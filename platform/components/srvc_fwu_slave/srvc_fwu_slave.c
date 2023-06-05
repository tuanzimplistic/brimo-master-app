/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_fwu_slave.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Dec 22
**  @brief      : Implementation of Srvc_Fwu_Slave module
**  @namespace  : FWUSLV
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Fwu_Slave
** @brief       Provides helper APIs to update main application firmware and bootloader firmware of slave board
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_fwu_slave.h"             /* Public header of this module */
#include "srvc_master_commander.h"      /* Use ESP-IDF's OTA firmware update APIs */
#include "mbzpl_req_m.h"                /* Use Modbus communication */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/task.h"              /* Use FreeRTOS task */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  ID of the CPU that the task running Bootloader protocol stack runs on */
#define FWUSLV_TASK_CPU_ID              1

/** @brief  Stack size (in bytes) of the task running Bootloader protocol stack */
#define FWUSLV_TASK_STACK_SIZE          4096

/** @brief  Priority of the task running Bootloader protocol stack */
#define FWUSLV_TASK_PRIORITY            (tskIDLE_PRIORITY + 1)

/** @brief  FreeRTOS event fired when Bootloader protocol stack is needed */
#define FWUSLV_BL_REQUIRED              0x00000001

/** @brief  States of firmware update process */
typedef enum
{
    FWUSLV_STATE_IDLE,                  //!< No firmware update of slave board is currently performed
    FWUSLV_STATE_READY,                 //!< Ready for a new firmware update of slave board
    FWUSLV_STATE_STARTED,               //!< Firmware update of slave board has been started and is in progress
} FWUSLV_state_t;

/** @brief  Project ID of slave firmware */
#define FWUSLV_SLAVE_PROJECT_ID         0x0001

/** @brief  Address and maximum size of slave's Bootloader firmware and Application firmware */
#define FWUSLV_BL_START_ADDR            0x08000000
#define FWUSLV_BL_MAX_SIZE              (64 * 1024)
#define FWUSLV_APP_START_ADDR           0x08010000
#define FWUSLV_APP_MAX_SIZE             (512 * 1024)

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Fwu_Slave";

/** @brief  Structure that will hold the TCB of the task being created */
static StaticTask_t g_x_task_buffer;

/** @brief  Buffer that the task being created will use as its stack */
static StackType_t g_x_task_stack [FWUSLV_TASK_STACK_SIZE];

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Firmware update state */
static FWUSLV_state_t g_enm_state = FWUSLV_STATE_IDLE;

/** @brief  Size in bytes of the firmware to update */
static uint32_t g_u32_fw_size = 0;

/** @brief  Number of bytes have been programmed onto flash of slave board */
static uint32_t g_u32_bytes_flashed = 0;

/** @brief  Instance of Master commander of Bootloader protocol */
static MCMD_inst_t g_x_cmd_inst;

/** @brief  State of slave board while it's in Bootloader */
static MCMD_fwu_state_t g_enm_bl_state = MCMD_STATE_RESERVED;

/** @brief  Indicates if Bootloader protocol is currently used */
static bool g_b_bootloader_used = false;

/** @brief  Handle of the task running Bootloader protocol stack */
static TaskHandle_t g_x_bl_task;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static void v_FWUSLV_Bl_Comm_Task (void * pv_param);
static void v_FWUSLV_Enable_Bootloader_Protocol (bool b_enabled);
static MCMD_fwu_state_t enm_FWUSLV_Get_Bl_State (uint32_t u32_timeout);
static void v_FWUSLV_Master_Cmd_Cb (MCMD_inst_t x_inst, MCMD_evt_t enm_evt, const void * pv_data, uint16_t u16_len);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_Fwu_Slave module
**
** @return
**      @arg    FWUSLV_OK
**      @arg    FWUSLV_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUSLV_Init (void)
{
    LOGD ("Initializing Srvc_Fwu_Slave module");

    /* If this module has been initialized, don't need to do that again */
    if (g_b_initialized)
    {
        return FWUSLV_OK;
    }

    /* Get instance of Master commander (Bootloader protocol) */
    if (s8_MCMD_Get_Inst (&g_x_cmd_inst) != MCMD_OK)
    {
        LOGE ("Failed to get instance of Master commander (Bootloader protocol)");
        return FWUSLV_ERR;
    }

    /* Register callack function to a Master commander */
    if (s8_MCMD_Register_Cb (g_x_cmd_inst, v_FWUSLV_Master_Cmd_Cb) != MCMD_OK)
    {
        LOGE ("Failed to register callack function to a Master commander");
        return FWUSLV_ERR;
    }

    /* Create task running Bootloader protocol stack */
    g_x_bl_task =
        xTaskCreateStaticPinnedToCore ( v_FWUSLV_Bl_Comm_Task,      /* Function that implements the task */
                                        "Srvc_Fwu_Slave",           /* Text name for the task */
                                        FWUSLV_TASK_STACK_SIZE,     /* Stack size in bytes, not words */
                                        NULL,                       /* Parameter passed into the task */
                                        FWUSLV_TASK_PRIORITY,       /* Priority at which the task is created */
                                        g_x_task_stack,             /* Array to use as the task's stack */
                                        &g_x_task_buffer,           /* Variable to hold the task's data structure */
                                        FWUSLV_TASK_CPU_ID);        /* ID of the CPU that the task runs on */
    if (g_x_bl_task == NULL)
    {
        LOGE ("Failed to create task performing the OTA update");
        return FWUSLV_ERR;
    }

    /* Done */
    LOGD ("Initialization of Srvc_Fwu_Slave module is done");
    g_b_initialized = true;
    return FWUSLV_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets current mode of slave board
**
** @param [out]
**      penm_mode: Current slave mode
**
** @return
**      @arg    FWUSLV_OK
**      @arg    FWUSLV_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUSLV_Get_Mode (FWUSLV_slave_mode_t * penm_mode)
{
    ASSERT_PARAM (g_b_initialized && (penm_mode != NULL));

    /* If slave board is in Application mode, it will respond the request asking for slave firmware version */
    v_FWUSLV_Enable_Bootloader_Protocol (false);
    if (mbzpl_MasterSendReq01 (SLAVE_ADDR, 100) == MB_MRE_NO_ERR)
    {
        *penm_mode = FWUSLV_MODE_APP;
        return FWUSLV_OK;
    }

    /* Probably slave board is in Bootloader mode */
    v_FWUSLV_Enable_Bootloader_Protocol (true);
    if (enm_FWUSLV_Get_Bl_State (100) != MCMD_STATE_RESERVED)
    {
        *penm_mode = FWUSLV_MODE_BL;
        return FWUSLV_OK;
    }

    /* Unable to determine mode */
    v_FWUSLV_Enable_Bootloader_Protocol (false);
    *penm_mode = FWUSLV_MODE_UNKNOWN;
    return FWUSLV_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets version of current running application slave firmware
**
** @param [out]
**      pu8_major: Major number
**
** @param [out]
**      pu8_minor: Minor number
**
** @param [out]
**      pu8_patch: Patch number
**
** @return
**      @arg    FWUSLV_OK
**      @arg    FWUSLV_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUSLV_Get_App_Version (uint8_t * pu8_major, uint8_t * pu8_minor, uint8_t * pu8_patch)
{
    ASSERT_PARAM (g_b_initialized && (pu8_major != NULL) && (pu8_minor != NULL) && (pu8_patch != NULL));

    /* Get revision of slave firmware via Modbus protocol */
    v_FWUSLV_Enable_Bootloader_Protocol (false);
    if (mbzpl_MasterSendReq01 (SLAVE_ADDR, 100) == MB_MRE_NO_ERR)
    {
        if (Req01_GetSlaveContext () == SLAVE_APPL_CONTEXT)
        {
            *pu8_major = Req01_GetMajorVersion ();
            *pu8_minor = Req01_GetMinorVersion ();
            *pu8_patch = Req01_GetPatchVersion ();
            return FWUSLV_OK;
        }
    }

    return FWUSLV_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Validates if slave firmware is valid
**
** @param [in]
**      pstru_fw_desc: Pointer to firmware descriptor
**
** @return
**      @arg    FWUSLV_OK
**      @arg    FWUSLV_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUSLV_Validate_Firmware_Info (const FWUSLV_desc_t * pstru_fw_desc)
{
    ASSERT_PARAM (g_b_initialized && (pstru_fw_desc != NULL));

    /* Validate some fields in firmware descriptor */
    if ((pstru_fw_desc->u32_recognizer != 0xAA55CC33) ||
        (pstru_fw_desc->u8_descriptor_rev != 1) ||
        ((pstru_fw_desc->u8_fw_type != FWUSLV_TYPE_BL) && (pstru_fw_desc->u8_fw_type != FWUSLV_TYPE_APP)))
    {
        return FWUSLV_ERR;
    }

    /* Check project ID */
    if ((pstru_fw_desc->u16_project_id != FWUSLV_SLAVE_PROJECT_ID) &&
        (pstru_fw_desc->u16_project_id != 0xFFFF))
    {
        return FWUSLV_ERR;
    }

    /* Check start address and size */
    bool b_bl_valid = (pstru_fw_desc->u32_start_addr == FWUSLV_BL_START_ADDR) &&
                      (pstru_fw_desc->u32_size <= FWUSLV_BL_MAX_SIZE);
    bool b_app_valid = (pstru_fw_desc->u32_start_addr == FWUSLV_APP_START_ADDR) &&
                       (pstru_fw_desc->u32_size <= FWUSLV_APP_MAX_SIZE);
    if ((b_bl_valid == false) && (b_app_valid == false))
    {
        return FWUSLV_ERR;
    }

    return FWUSLV_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Requests slave board to enter Bootloader mode
**
** @return
**      @arg    FWUSLV_OK
**      @arg    FWUSLV_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUSLV_Enter_Bootloader (void)
{
    ASSERT_PARAM (g_b_initialized);

    /* Via Modbus protocol, request slave board to enter Bootloader mode */
    for (uint8_t u8_retry = 0; u8_retry < 3; u8_retry++)
    {
        v_FWUSLV_Enable_Bootloader_Protocol (false);
        mbzpl_MasterSendReq02 (SLAVE_ADDR, 100);

        /* Wait for slave board to be ready in Bootloader mode */
        vTaskDelay (pdMS_TO_TICKS (250));
        v_FWUSLV_Enable_Bootloader_Protocol (true);
        if (enm_FWUSLV_Get_Bl_State (200) != MCMD_STATE_RESERVED)
        {
            /* Slave board is now in Bootloader mode */
            return FWUSLV_OK;
        }
        LOGW ("Retry entering Bootloader");
    }

    /* Failed to enter Bootloader mode */
    v_FWUSLV_Enable_Bootloader_Protocol (false);
    return FWUSLV_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Requests slave board to exit Bootloader mode and enter Application mode
**
** @return
**      @arg    FWUSLV_OK
**      @arg    FWUSLV_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUSLV_Exit_Bootloader (void)
{
    ASSERT_PARAM (g_b_initialized);

    /* Request slave board to exit Bootloader mode and enter Application mode */
    v_FWUSLV_Enable_Bootloader_Protocol (true);
    s8_MCMD_Reset (g_x_cmd_inst, false);

    /* Wait for slave board to be ready in Application mode */
    v_FWUSLV_Enable_Bootloader_Protocol (false);
    for (uint8_t u8_idx = 0; u8_idx < 10; u8_idx++)
    {
        if (mbzpl_MasterSendReq01 (SLAVE_ADDR, 100) == MB_MRE_NO_ERR)
        {
            /* Slave board is now in Application mode */
            return FWUSLV_OK;
        }
    }

    return FWUSLV_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Prepares a new firmware update (slave board must be in Bootloader mode)
**
** @param [in]
**      pstru_fw_desc: Pointer to firmware descriptor
**
** @param [out]
**      penm_result: Result of the preparation
**      @arg    FWUSLV_RESULT_OK
**      @arg    FWUSLV_RESULT_WARN_FW_OLDER_VER
**      @arg    FWUSLV_RESULT_WARN_FW_SAME_VER
**      @arg    FWUSLV_RESULT_WARN_FW_VAR_MISMATCH
**      @arg    FWUSLV_RESULT_WARN_FW_ALREADY_EXIST
**      @arg    FWUSLV_RESULT_ERR_UNKNOWN
**      @arg    FWUSLV_RESULT_ERR_FW_NOT_COMPATIBLE
**      @arg    FWUSLV_RESULT_ERR_FW_SIZE_TOO_BIG
**      @arg    FWUSLV_RESULT_ERR_FW_UPDATE_NOT_DONE
**
** @return
**      @arg    FWUSLV_OK
**      @arg    FWUSLV_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUSLV_Prepare_Update (const FWUSLV_desc_t * pstru_fw_desc, FWUSLV_result_t * penm_result)
{
    ASSERT_PARAM (g_b_initialized && (pstru_fw_desc != NULL) && (penm_result != NULL));

    /* A firmware update can only be started if the previous one (if any) has been done */
    if (g_enm_state == FWUSLV_STATE_STARTED)
    {
        *penm_result = FWUSLV_RESULT_ERR_FW_UPDATE_NOT_DONE;
        return FWUSLV_ERR;
    }

    /* Display firmware information */
    LOGI ("Received a request to update firmware:");
    LOGI ("+ Firmware name: %s", pstru_fw_desc->stri_desc);
    LOGI ("+ Firmware revision: %d.%d.%d",
          pstru_fw_desc->u8_major_rev, pstru_fw_desc->u8_minor_rev, pstru_fw_desc->u8_patch_rev);
    LOGI ("+ Firmware size: %d bytes", pstru_fw_desc->u32_size);

    /* Information of slave firmware to be updated */
    MCMD_fw_info_t stru_fw_info =
    {
        .u8_fw_type     = pstru_fw_desc->u8_fw_type,
        .u8_major_rev   = pstru_fw_desc->u8_major_rev,
        .u8_minor_rev   = pstru_fw_desc->u8_minor_rev,
        .u8_patch_rev   = pstru_fw_desc->u8_patch_rev,
        .u16_project_id = pstru_fw_desc->u16_project_id,
        .u16_variant_id = pstru_fw_desc->u16_variant_id,
        .u32_size       = pstru_fw_desc->u32_size,
        .u32_crc32      = pstru_fw_desc->u32_crc,
    };

    /* Prepare slave board for firmware update */
    MCMD_result_code_t enm_cmd_result = MCMD_RESULT_ERR_UNKNOWN;
    if (s8_MCMD_Prepare_Update (g_x_cmd_inst, &stru_fw_info, &enm_cmd_result) != MCMD_OK)
    {
        LOGE ("Failed to prepare slave board for firmware update");
        *penm_result = FWUSLV_RESULT_ERR_UNKNOWN;
        return FWUSLV_ERR;
    }

    /* Check result of the preparation */
    switch (enm_cmd_result)
    {
        case MCMD_RESULT_OK:
            *penm_result = FWUSLV_RESULT_OK;
            break;

        case MCMD_RESULT_WARN_FW_OLDER_VER:
            *penm_result = FWUSLV_RESULT_WARN_FW_OLDER_VER;
            break;

        case MCMD_RESULT_WARN_FW_SAME_VER:
            *penm_result = FWUSLV_RESULT_WARN_FW_SAME_VER;
            break;

        case MCMD_RESULT_WARN_FW_VAR_MISMATCH:
            *penm_result = FWUSLV_RESULT_WARN_FW_VAR_MISMATCH;
            break;

        case MCMD_RESULT_WARN_FW_ALREADY_EXIST:
            *penm_result = FWUSLV_RESULT_WARN_FW_ALREADY_EXIST;
            break;

        case MCMD_RESULT_ERR_FW_NOT_COMPATIBLE:
            *penm_result = FWUSLV_RESULT_ERR_FW_NOT_COMPATIBLE;
            break;

        case MCMD_RESULT_ERR_FW_SIZE_TOO_BIG:
            *penm_result = FWUSLV_RESULT_ERR_FW_SIZE_TOO_BIG;
            break;

        case MCMD_RESULT_ERR_FW_UPDATE_NOT_DONE:
            *penm_result = FWUSLV_RESULT_ERR_FW_UPDATE_NOT_DONE;
            break;

        default:
            *penm_result = FWUSLV_RESULT_ERR_UNKNOWN;
            break;
    }

    /* If slave board accepts the firmware */
    if (enm_cmd_result < MCMD_RESULT_ERR_UNKNOWN)
    {
        g_enm_state = FWUSLV_STATE_READY;
        g_u32_fw_size = pstru_fw_desc->u32_size;
        return FWUSLV_OK;
    }

    return FWUSLV_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Starts firmware update process (slave board must be in Bootloader mode)
**
** @note
**      Once this function is called, s8_FWUSLV_Finalize_Update() must be called to finish the firmware update process.
**
** @param [out]
**      penm_result: Result of the operation
**      @arg    FWUSLV_RESULT_OK
**      @arg    FWUSLV_RESULT_ERR_UNKNOWN
**      @arg    FWUSLV_RESULT_ERR_FW_UPDATE_NOT_STARTED
**      @arg    FWUSLV_RESULT_ERR_FW_REJECTED
**      @arg    FWUSLV_RESULT_ERR_ERASING_FAILED
**
** @return
**      @arg    FWUSLV_OK
**      @arg    FWUSLV_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUSLV_Start_Update (FWUSLV_result_t * penm_result)
{
    ASSERT_PARAM (g_b_initialized && (penm_result != NULL));

    /* Check if we are ready to start the firmware update */
    if (g_enm_state != FWUSLV_STATE_READY)
    {
        *penm_result = FWUSLV_RESULT_ERR_FW_UPDATE_NOT_STARTED;
        return FWUSLV_ERR;
    }

    /* Start firmware update on slave board */
    MCMD_result_code_t enm_cmd_result = MCMD_RESULT_ERR_UNKNOWN;
    if (s8_MCMD_Start_Update (g_x_cmd_inst, &enm_cmd_result) != MCMD_OK)
    {
        LOGE ("Failed to start firmware update on slave board");
        *penm_result = FWUSLV_RESULT_ERR_UNKNOWN;
        return FWUSLV_ERR;
    }

    /* Check result */
    switch (enm_cmd_result)
    {
        case MCMD_RESULT_OK:
            *penm_result = FWUSLV_RESULT_OK;
            g_enm_state = FWUSLV_STATE_STARTED;
            g_u32_bytes_flashed = 0;
            LOGI ("Firmware update started");
            break;

        case MCMD_RESULT_ERR_FW_REJECTED:
            *penm_result = FWUSLV_RESULT_ERR_FW_REJECTED;
            break;

        case MCMD_RESULT_ERR_ERASING_FAILED:
            *penm_result = FWUSLV_RESULT_ERR_ERASING_FAILED;
            break;

        default:
            *penm_result = FWUSLV_RESULT_ERR_UNKNOWN;
            break;
    }

    return (enm_cmd_result < MCMD_RESULT_ERR_UNKNOWN) ? FWUSLV_OK : FWUSLV_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Programs each chunk of a firmware data onto flash of slave board (slave board must be in Bootloader mode)
**
** @param [in]
**      pstru_fw_data: A chunk of firmware data to program
**
** @param [out]
**      penm_result: Result of the operation
**      @arg    FWUSLV_RESULT_OK
**      @arg    FWUSLV_RESULT_ERR_UNKNOWN
**      @arg    FWUSLV_RESULT_ERR_FW_UPDATE_NOT_STARTED
**      @arg    FWUSLV_RESULT_ERR_INVALID_DATA
**      @arg    FWUSLV_RESULT_ERR_FW_DOWNLOAD_TIMEOUT
**      @arg    FWUSLV_RESULT_ERR_WRITING_FAILED
**
** @return
**      @arg    FWUSLV_OK
**      @arg    FWUSLV_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUSLV_Program_Firmware (const FWUSLV_data_chunk_t * pstru_fw_data, FWUSLV_result_t * penm_result)
{
    ASSERT_PARAM (g_b_initialized && (pstru_fw_data != NULL) && (penm_result != NULL));

    /* Check if firmware update process has been started */
    if (g_enm_state != FWUSLV_STATE_STARTED)
    {
        *penm_result = FWUSLV_RESULT_ERR_FW_UPDATE_NOT_STARTED;
        return FWUSLV_ERR;
    }

    /* Firmware data chunk */
    MCMD_fw_data_chunk_t stru_fw_data =
    {
        .u32_offset     = pstru_fw_data->u32_offset,
        .u16_data_len   = pstru_fw_data->u16_data_len,
        .pu8_firmware   = pstru_fw_data->pu8_firmware,
    };

    /* Downloads the firmware data chunk to Slave board */
    MCMD_result_code_t enm_cmd_result = MCMD_RESULT_ERR_UNKNOWN;
    if (s8_MCMD_Download_Firmware (g_x_cmd_inst, &stru_fw_data, &enm_cmd_result) != MCMD_OK)
    {
        LOGE ("Failed to download firmware data chunk to Slave board");
        *penm_result = FWUSLV_RESULT_ERR_UNKNOWN;
        return FWUSLV_ERR;
    }

    /* Check result */
    switch (enm_cmd_result)
    {
        case MCMD_RESULT_OK:
            *penm_result = FWUSLV_RESULT_OK;
            g_u32_bytes_flashed += pstru_fw_data->u16_data_len;
            // LOGI ("Programming new firmware... %.1f%% (%d/%d bytes)",
                  // g_u32_bytes_flashed * 100.0 / g_u32_fw_size, g_u32_bytes_flashed, g_u32_fw_size);
            break;

        case MCMD_RESULT_ERR_INVALID_DATA:
            *penm_result = FWUSLV_RESULT_ERR_INVALID_DATA;
            break;

        case MCMD_RESULT_ERR_FW_DOWNLOAD_TIMEOUT:
            *penm_result = FWUSLV_RESULT_ERR_FW_DOWNLOAD_TIMEOUT;
            break;

        case MCMD_RESULT_ERR_WRITING_FAILED:
            *penm_result = FWUSLV_RESULT_ERR_WRITING_FAILED;
            break;

        default:
            *penm_result = FWUSLV_RESULT_ERR_UNKNOWN;
            break;
    }

    return (enm_cmd_result < MCMD_RESULT_ERR_UNKNOWN) ? FWUSLV_OK : FWUSLV_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Cancels or finalizes current firmware update process (slave board must be in Bootloader mode)
**
** @note
**      Once s8_FWUSLV_Start_Update() has been called, this function must be called to finish the firmware update
**      process.
**
** @param [in]
**      b_finalized
**      @arg    false: cancel firmware update process
**      @arg    true: finish and finalize firmware update process
**
** @param [out]
**      penm_result: Result of the operation
**      @arg    FWUSLV_RESULT_OK
**      @arg    FWUSLV_RESULT_ERR_UNKNOWN
**      @arg    FWUSLV_RESULT_ERR_FW_UPDATE_NOT_STARTED
**      @arg    FWUSLV_RESULT_ERR_VALIDATION_FAILED
**      @arg    FWUSLV_RESULT_ERR_FW_DOWNLOAD_TIMEOUT
**      @arg    FWUSLV_RESULT_ERR_INSTALL_BL_FAILED
**
** @return
**      @arg    FWUSLV_OK
**      @arg    FWUSLV_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUSLV_Finalize_Update (bool b_finalized, FWUSLV_result_t * penm_result)
{
    ASSERT_PARAM (g_b_initialized && (penm_result != NULL));

    /* Check if firmware update process has been started */
    if (g_enm_state != FWUSLV_STATE_STARTED)
    {
        LOGW ("Firmware update terminated");
        *penm_result = FWUSLV_RESULT_ERR_FW_UPDATE_NOT_STARTED;
        return FWUSLV_ERR;
    }

    /* Mark firmware update process as being done */
    g_enm_state = FWUSLV_STATE_IDLE;

    /* If firmware update process is canceled */
    if (!b_finalized)
    {
        MCMD_result_code_t enm_cmd_result = MCMD_RESULT_ERR_UNKNOWN;
        s8_MCMD_Finalize_Update (g_x_cmd_inst, true, &enm_cmd_result);
        LOGW ("Firmware update aborted");
        *penm_result = FWUSLV_RESULT_OK;
        return FWUSLV_OK;
    }

    /* Finalize slave firmware update process */
    MCMD_result_code_t enm_cmd_result = MCMD_RESULT_ERR_UNKNOWN;
    if (s8_MCMD_Finalize_Update (g_x_cmd_inst, false, &enm_cmd_result) != MCMD_OK)
    {
        LOGE ("Failed to finalize firmware update on slave board");
        *penm_result = FWUSLV_RESULT_ERR_UNKNOWN;
        return FWUSLV_ERR;
    }

    /* Check result */
    switch (enm_cmd_result)
    {
        case MCMD_RESULT_OK:
            *penm_result = FWUSLV_RESULT_OK;
            LOGI ("Firmware update is done successfully");
            break;

        case MCMD_RESULT_ERR_VALIDATION_FAILED:
            *penm_result = FWUSLV_RESULT_ERR_VALIDATION_FAILED;
            break;

        case MCMD_RESULT_ERR_FW_DOWNLOAD_TIMEOUT:
            *penm_result = FWUSLV_RESULT_ERR_FW_DOWNLOAD_TIMEOUT;
            break;

        case MCMD_RESULT_ERR_INSTALL_BL_FAILED:
            *penm_result = FWUSLV_RESULT_ERR_INSTALL_BL_FAILED;
            break;

        default:
            *penm_result = FWUSLV_RESULT_ERR_UNKNOWN;
            break;
    }

    return (enm_cmd_result < MCMD_RESULT_ERR_UNKNOWN) ? FWUSLV_OK : FWUSLV_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Task running Bootloader protocol stack
**
** @param [in]
**      pv_param: Parameter passed into the task
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_FWUSLV_Bl_Comm_Task (void * pv_param)
{
    const uint32_t  u32_bits_to_clear_on_entry = 0x00000000;
    const uint32_t  u32_bits_to_clear_on_exit = 0xFFFFFFFF;
    uint32_t        u32_notify_value;

    /* Endless loop of the task */
    while (true)
    {
        /* Wait until Bootloader protocol is required */
        xTaskNotifyWait (u32_bits_to_clear_on_entry, u32_bits_to_clear_on_exit, &u32_notify_value, portMAX_DELAY);
        if (u32_notify_value & FWUSLV_BL_REQUIRED)
        {
            /* Run Bootloader protocol stack until it is not required */
            while (g_b_bootloader_used)
            {
                s8_MCMD_Run_Inst (g_x_cmd_inst);
                vTaskDelay (pdMS_TO_TICKS (10));
            }
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Enable bootloader protocol in communication with slave board
**
** @param [in]
**      b_enabled
**      @arg    true: communicate with slave board via Bootloader protocol
**      @arg    false: communicate with slave board via Modbus protocol
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_FWUSLV_Enable_Bootloader_Protocol (bool b_enabled)
{
    /* Select the protocol to be used */
    if (b_enabled != g_b_bootloader_used)
    {
        g_b_bootloader_used = b_enabled;
        if (b_enabled)
        {
            vMBMasterPortSerialEnable (false, false);
            xTaskNotify (g_x_bl_task, FWUSLV_BL_REQUIRED, eSetBits);
        }
        else
        {
            vMBMasterPortSerialEnable (true, true);

            /* Wait for all UART leftover is processed completely by Modbus protocol */
            vTaskDelay (pdMS_TO_TICKS (100));
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets current state of slave board's Bootloader
**
** @param [in]
**      u32_timeout: Timeout in milliseconds waiting for the response from slave board
**
** @return
**      @arg    MCMD_STATE_RESERVED: Failed to get Bootloader state from slave board
**      @arg    Otherwise: Bootloader state of slave board
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static MCMD_fwu_state_t enm_FWUSLV_Get_Bl_State (uint32_t u32_timeout)
{
    /* Flush all responses from slave */
    s8_MCMD_Run_Inst (g_x_cmd_inst);

    /* Get Bootloader state from slave board and wait for response */
    g_enm_bl_state = MCMD_STATE_RESERVED;
    for (uint32_t u32_time = 0; (u32_time < u32_timeout) && (g_enm_bl_state == MCMD_STATE_RESERVED); u32_time += 10)
    {
        if ((u32_time % 100) == 0)
        {
            s8_MCMD_Check_Bootloader_State (g_x_cmd_inst);
        }
        vTaskDelay (pdMS_TO_TICKS (10));
    }

    return g_enm_bl_state;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Callback invoked when an event of Master commander (Bootloader protocol) occurs
**
** @param [in]
**      x_inst: Instance of Master commander
**
** @param [in]
**      enm_evt: Event which occurred
**
** @param [in]
**      pv_data: Pointer to the event data
**
** @param [in]
**      u16_len: Length in byte of the data pointed by pv_data
**
** @return
**      @arg    FWUSLV_OK
**      @arg    FWUSLV_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_FWUSLV_Master_Cmd_Cb (MCMD_inst_t x_inst, MCMD_evt_t enm_evt, const void * pv_data, uint16_t u16_len)
{
    if (enm_evt == MCMD_EVT_SLAVE_IN_BOOTLOADER)
    {
        /* Get state of slave board in Bootloader mode */
        g_enm_bl_state = *((MCMD_fwu_state_t *)pv_data);
    }
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
