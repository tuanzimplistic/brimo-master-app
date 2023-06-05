/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_master_commander.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Dec 18
**  @brief      : Implementation of Srvc_Master_Commander module
**  @namespace  : MCMD
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Master_Commander
** @brief       Abstracts application layer (client side) of Bootloader protocol
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_master_commander.h"      /* Public header of this module */
#include "srvc_master_transport.h"      /* Use transport layer of Bootloader protocol */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/event_groups.h"      /* Use FreeRTOS event group */
#include "freertos/semphr.h"            /* Use FreeRTOS semaphore */

#include <string.h>                     /* Use memcpy() */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Maximum number of callback functions */
#define MCMD_NUM_CB                     1

/** @brief  Maximum length in bytes of a Master application message */
#define MCMD_MAX_MSG_LEN                245

/** @brief  Structure wrapping data of a Master commander */
struct MCMD_obj
{
    bool                b_initialized;                  //!< Specifies whether the object has been initialized or not
    MTP_inst_t          x_transport_inst;               //!< Instance of the transport channel

    uint8_t             au8_buf [MCMD_MAX_MSG_LEN];     //!< Buffer storing command message to send
    SemaphoreHandle_t   x_sem_comm;                     //!< Semaphore ensuring that there is one command at a time
    MCMD_cb_t           apfnc_cb [MCMD_NUM_CB];         //!< Callback function invoked when an event occurs
};

/** @brief  Structure of Master application message */
typedef struct
{
    uint8_t             u8_cid;                         //!< Command ID
    uint8_t             u8_status;                      //!< Status
    uint8_t             au8_data[];                     //!< Command data

} MCMD_msg_t;

/** @brief  Default timeout (in milliseconds) for a request message */
#define MCMD_DEFAULT_TIMEOUT            200

/** @brief  Exchange status */
enum
{
    MCMD_STATUS_OK                      = 0x00,         //!< Okay, no error occurred
    MCMD_STATUS_ERR                     = 0x80,         //!< An error has occurred
    MCMD_STATUS_ERR_NOT_SUPPORTED       = 0x81,         //!< Command is not supported
    MCMD_STATUS_ERR_INVALID_DATA        = 0x82,         //!< Some data in command is not valid
    MCMD_STATUS_ERR_BUSY                = 0x83,         //!< Device is busy
    MCMD_STATUS_ERR_STATE_NOT_ALLOWED   = 0x84,         //!< Current device state does not allow the command
    MCMD_STATUS_ERR_INVALID_ACCESS      = 0x85,         //!< Invalid access
};

/** @brief  List of all command IDs */
enum
{
    /* Requests/Responses */
    MCMD_FW_PREPARE_WRITE_REQ           = 0x00,         //!< Prepares Slave board for firmware update
    MCMD_FW_START_WRITE_REQ             = 0x01,         //!< Start firmware update on Slave board
    MCMD_FW_DOWNLOAD_WRITE_REQ          = 0x02,         //!< Downloads each chunk of a firmware to Slave board
    MCMD_FW_FINALIZE_WRITE_REQ          = 0x03,         //!< Finalizes firmware update on Slave board

    /* Posts */
    MCMD_SCAN_POST                      = 0x80,         //!< Check and get state of Slave board in bootloader mode
    MCMD_DEV_RESET_POST                 = 0x81,         //!< Reset Slave board

    /* Notifications */
    MCMD_SCAN_NOTIFY                    = 0xC0,         //!< State of Slave board in bootloader mode
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Master_Commander";

/** @brief  Single instance of Master commander */
static struct MCMD_obj g_stru_mcmd_obj =
{
    .b_initialized      = false,
    .x_transport_inst   = NULL,
    .x_sem_comm         = NULL,
    .apfnc_cb           = { NULL },
};

/** @brief  Indicates if this module has been initialized or not */
static bool g_b_initialized = false;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifdef USE_MODULE_ASSERT
 static bool b_MCMD_Is_Valid_Inst (MCMD_inst_t x_inst);
#endif

static int8_t s8_MCMD_Init_Module (void);
static int8_t s8_MCMD_Init_Inst (MCMD_inst_t x_inst);
static void v_MCMD_Transport_Cb (MTP_inst_t x_transport_inst, MTP_evt_t enm_evt,
                                 const void * pv_data, uint16_t u16_len);
static void v_MCMD_Process_Notification (MCMD_inst_t x_inst, MCMD_msg_t * pstru_msg, uint16_t u16_msg_len);
static int8_t s8_MCMD_Send_Request (MCMD_inst_t x_inst, MCMD_msg_t * pstru_request, uint16_t u16_request_len,
                                    MCMD_msg_t ** ppstru_response, uint16_t * pu16_response_len, uint16_t u16_timeout);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets instance of Master commander
**
** @param [out]
**      px_inst: Container to store the retrieved instance
**
** @return
**      @arg    MCMD_OK
**      @arg    MCMD_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MCMD_Get_Inst (MCMD_inst_t * px_inst)
{
    MCMD_inst_t x_inst = NULL;
    int8_t      s8_result = MCMD_OK;

    /* Validation */
    ASSERT_PARAM (px_inst != NULL);

    /* Initialize */
    *px_inst = NULL;

    /* If this module has not been initialized, do that now */
    if (s8_result >= MCMD_OK)
    {
        if (!g_b_initialized)
        {
            s8_result = s8_MCMD_Init_Module ();
            if (s8_result >= MCMD_OK)
            {
                g_b_initialized = true;
            }
        }
    }

    /* If the retrieved instance has not been initialized yet, do that now */
    if (s8_result >= MCMD_OK)
    {
        x_inst = &g_stru_mcmd_obj;
        if (!x_inst->b_initialized)
        {
            s8_result = s8_MCMD_Init_Inst (x_inst);
            if (s8_result >= MCMD_OK)
            {
                x_inst->b_initialized = true;
            }
        }
    }

    /* Return instance of the Master commander */
    if (s8_result >= MCMD_OK)
    {
        *px_inst = x_inst;
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs Master commander
**
** @note
**      This function must be called periodically
**
** @param [in]
**      x_inst: Specific instance
**
** @return
**      @arg    MTP_OK
**      @arg    MTP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MCMD_Run_Inst (MCMD_inst_t x_inst)
{
    ASSERT_PARAM (b_MCMD_Is_Valid_Inst (x_inst));

    /* Run Master transport channel */
    int8_t s8_result = s8_MTP_Run_Inst (x_inst->x_transport_inst);

    /* Done */
    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Registers callack function to a Master commander
**
** @note
**      This function is not thread-safe
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pfnc_cb: Pointer of the callback function
**
** @return
**      @arg    MCMD_OK
**      @arg    MCMD_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MCMD_Register_Cb (MCMD_inst_t x_inst, MCMD_cb_t pfnc_cb)
{
    ASSERT_PARAM (b_MCMD_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized && (pfnc_cb != NULL));

    /* Store callback function pointer */
    for (uint8_t u8_idx = 0; u8_idx < MCMD_NUM_CB; u8_idx++)
    {
        if (x_inst->apfnc_cb [u8_idx] == NULL)
        {
            x_inst->apfnc_cb [u8_idx] = pfnc_cb;
            return MCMD_OK;
        }
    }

    LOGE ("Failed to register callback function");
    return MCMD_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets state of Slave board if it is in Bootloader mode. If Slave board is in Bootloader mode, its state will be
**      returned via context data of MCMD_EVT_SLAVE_IN_BOOTLOADER event.
**
** @param [in]
**      x_inst: Specific instance
**
** @return
**      @arg    MCMD_OK
**      @arg    MCMD_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MCMD_Check_Bootloader_State (MCMD_inst_t x_inst)
{
    ASSERT_PARAM (b_MCMD_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized);

    /* Take the Post exchange */
    xSemaphoreTakeRecursive (x_inst->x_sem_comm, portMAX_DELAY);

    /* Construct post message */
    MCMD_msg_t * pstru_post     = (MCMD_msg_t *)x_inst->au8_buf;
    pstru_post->u8_cid          = MCMD_SCAN_POST;
    pstru_post->u8_status       = MCMD_STATUS_OK;

    /* Send the post message */
    int8_t s8_result = s8_MTP_Send_Post (x_inst->x_transport_inst, pstru_post, sizeof (MCMD_msg_t));

    /* Release the Post exchange */
    xSemaphoreGiveRecursive (x_inst->x_sem_comm);

    return (s8_result < MTP_OK ? MCMD_ERR : MCMD_OK);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Resets Slave board
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      b_bootloader_mode: Desired operation mode of Slave board on bootup
**      @arg    true: Slave board should enter Bootloader mode on bootup
**      @arg    false: Slave board should enter Application mode on bootup
**
** @return
**      @arg    MCMD_OK
**      @arg    MCMD_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MCMD_Reset (MCMD_inst_t x_inst, bool b_bootloader_mode)
{
    ASSERT_PARAM (b_MCMD_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized);

    /* Take the Post exchange */
    xSemaphoreTakeRecursive (x_inst->x_sem_comm, portMAX_DELAY);

    /* Construct post message */
    MCMD_msg_t * pstru_post     = (MCMD_msg_t *)x_inst->au8_buf;
    pstru_post->u8_cid          = MCMD_DEV_RESET_POST;
    pstru_post->u8_status       = MCMD_STATUS_OK;
    pstru_post->au8_data[0]     = b_bootloader_mode ? 0x00 : 0x01;

    /* Send the post message */
    int8_t s8_result = s8_MTP_Send_Post (x_inst->x_transport_inst, pstru_post, sizeof (MCMD_msg_t) + 1);

    /* Release the Post exchange */
    xSemaphoreGiveRecursive (x_inst->x_sem_comm);

    return (s8_result < MTP_OK ? MCMD_ERR : MCMD_OK);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Prepares Slave board for firmware update
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pstru_fw_info: Information of Slave firmware
**
** @param [out]
**      penm_result: Result of the preparation
**      @arg    MCMD_RESULT_OK
**      @arg    MCMD_RESULT_WARN_FW_OLDER_VER
**      @arg    MCMD_RESULT_WARN_FW_SAME_VER
**      @arg    MCMD_RESULT_WARN_FW_VAR_MISMATCH
**      @arg    MCMD_RESULT_WARN_FW_ALREADY_EXIST
**      @arg    MCMD_RESULT_ERR_UNKNOWN
**      @arg    MCMD_RESULT_ERR_FW_NOT_COMPATIBLE
**      @arg    MCMD_RESULT_ERR_FW_SIZE_TOO_BIG
**      @arg    MCMD_RESULT_ERR_FW_UPDATE_NOT_DONE
**
** @return
**      @arg    MCMD_OK
**      @arg    MCMD_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MCMD_Prepare_Update (MCMD_inst_t x_inst, const MCMD_fw_info_t * pstru_fw_info,
                               MCMD_result_code_t * penm_result)
{
    ASSERT_PARAM (b_MCMD_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized && (pstru_fw_info != NULL) && (penm_result != NULL));

    /* Take the Request exchange */
    xSemaphoreTakeRecursive (x_inst->x_sem_comm, portMAX_DELAY);

    /* Construct request message */
    MCMD_msg_t * pstru_request  = (MCMD_msg_t *)x_inst->au8_buf;
    pstru_request->u8_cid       = MCMD_FW_PREPARE_WRITE_REQ;
    pstru_request->u8_status    = MCMD_STATUS_OK;

    /* Firmware type */
    uint8_t u8_offset = 0;
    pstru_request->au8_data[u8_offset] = pstru_fw_info->u8_fw_type;
    u8_offset += 1;

    /* Project ID */
    ENDIAN_PUT16 (&pstru_request->au8_data[u8_offset], pstru_fw_info->u16_project_id);
    u8_offset += 2;

    /* Variant ID */
    ENDIAN_PUT16 (&pstru_request->au8_data[u8_offset], pstru_fw_info->u16_variant_id);
    u8_offset += 2;

    /* Firmware version */
    pstru_request->au8_data[u8_offset] = pstru_fw_info->u8_major_rev;
    u8_offset += 1;
    pstru_request->au8_data[u8_offset] = pstru_fw_info->u8_minor_rev;
    u8_offset += 1;
    pstru_request->au8_data[u8_offset] = pstru_fw_info->u8_patch_rev;
    u8_offset += 1;

    /* Firmware size */
    ENDIAN_PUT32 (&pstru_request->au8_data[u8_offset], pstru_fw_info->u32_size);
    u8_offset += 4;

    /* Firmware checksum */
    ENDIAN_PUT32 (&pstru_request->au8_data[u8_offset], pstru_fw_info->u32_crc32);
    u8_offset += 4;

    /* Send the request message and wait for the response */
    MCMD_msg_t *    pstru_response;
    uint16_t        u16_response_len;
    int8_t s8_result = s8_MCMD_Send_Request (x_inst, pstru_request, u8_offset,
                                             &pstru_response, &u16_response_len, MCMD_DEFAULT_TIMEOUT);

    /* Check the response */
    if (s8_result >= MCMD_OK)
    {
        if (u16_response_len != 1)
        {
            s8_result = MCMD_ERR;
            LOGE ("Invalid response for request MCMD_FW_PREPARE_WRITE_REQ");
        }
        else
        {
            *penm_result = (MCMD_result_code_t) pstru_response->au8_data[0];;
        }
    }

    /* Release the Request exchange */
    xSemaphoreGiveRecursive (x_inst->x_sem_comm);

    return (s8_result);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Starts firmware update on Slave board
**
** @note
**      This function may take several seconds to complete
**
** @param [in]
**      x_inst: Specific instance
**
** @param [out]
**      penm_result: Result of the operation
**      @arg    MCMD_RESULT_OK
**      @arg    MCMD_RESULT_ERR_UNKNOWN
**      @arg    MCMD_RESULT_ERR_FW_REJECTED
**      @arg    MCMD_RESULT_ERR_ERASING_FAILED
**
** @return
**      @arg    MCMD_OK
**      @arg    MCMD_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MCMD_Start_Update (MCMD_inst_t x_inst, MCMD_result_code_t * penm_result)
{
    ASSERT_PARAM (b_MCMD_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized && (penm_result != NULL));

    /* Take the Request exchange */
    xSemaphoreTakeRecursive (x_inst->x_sem_comm, portMAX_DELAY);

    /* Construct request message */
    MCMD_msg_t * pstru_request  = (MCMD_msg_t *)x_inst->au8_buf;
    pstru_request->u8_cid       = MCMD_FW_START_WRITE_REQ;
    pstru_request->u8_status    = MCMD_STATUS_OK;

    /* Send the request message and wait for the response */
    MCMD_msg_t *    pstru_response;
    uint16_t        u16_response_len;
    int8_t s8_result = s8_MCMD_Send_Request (x_inst, pstru_request, 0, &pstru_response, &u16_response_len, 4000);

    /* Check the response */
    if (s8_result >= MCMD_OK)
    {
        if (u16_response_len != 1)
        {
            s8_result = MCMD_ERR;
            LOGE ("Invalid response for request MCMD_FW_START_WRITE_REQ");
        }
        else
        {
            *penm_result = (MCMD_result_code_t) pstru_response->au8_data[0];;
        }
    }

    /* Release the Request exchange */
    xSemaphoreGiveRecursive (x_inst->x_sem_comm);

    return (s8_result);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Downloads each chunk of a firmware to Slave board
**
** @note
**      This function may take several seconds to complete
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pstru_fw_data: A chunk of firmware data to flash onto Slave board
**
** @param [out]
**      penm_result: Result of the operation
**      @arg    MCMD_RESULT_OK
**      @arg    MCMD_RESULT_ERR_UNKNOWN
**      @arg    MCMD_RESULT_ERR_FW_UPDATE_NOT_STARTED
**      @arg    MCMD_RESULT_ERR_INVALID_DATA
**      @arg    MCMD_RESULT_ERR_FW_DOWNLOAD_TIMEOUT
**      @arg    MCMD_RESULT_ERR_WRITING_FAILED
**
** @return
**      @arg    MCMD_OK
**      @arg    MCMD_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MCMD_Download_Firmware (MCMD_inst_t x_inst, const MCMD_fw_data_chunk_t * pstru_fw_data,
                                  MCMD_result_code_t * penm_result)
{
    ASSERT_PARAM (b_MCMD_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized && (pstru_fw_data != NULL) && (penm_result != NULL));

    /* Take the Request exchange */
    xSemaphoreTakeRecursive (x_inst->x_sem_comm, portMAX_DELAY);

    /* Construct request message */
    MCMD_msg_t * pstru_request  = (MCMD_msg_t *)x_inst->au8_buf;
    pstru_request->u8_cid       = MCMD_FW_DOWNLOAD_WRITE_REQ;
    pstru_request->u8_status    = MCMD_STATUS_OK;

    /* Offset of the data chunk */
    uint8_t u8_offset = 0;
    ENDIAN_PUT32 (&pstru_request->au8_data[u8_offset], pstru_fw_data->u32_offset);
    u8_offset += 4;

    /* Size of the data chunk */
    ENDIAN_PUT16 (&pstru_request->au8_data[u8_offset], pstru_fw_data->u16_data_len);
    u8_offset += 2;

    /* Firmware data */
    memcpy (&pstru_request->au8_data[u8_offset], pstru_fw_data->pu8_firmware, pstru_fw_data->u16_data_len);
    u8_offset += pstru_fw_data->u16_data_len;

    /* Send the request message and wait for the response */
    MCMD_msg_t *    pstru_response;
    uint16_t        u16_response_len;
    int8_t s8_result = s8_MCMD_Send_Request (x_inst, pstru_request, u8_offset, &pstru_response, &u16_response_len, 1500);

    /* Check the response */
    if (s8_result >= MCMD_OK)
    {
        if (u16_response_len != 1)
        {
            s8_result = MCMD_ERR;
            LOGE ("Invalid response for request MCMD_FW_DOWNLOAD_WRITE_REQ");
        }
        else
        {
            *penm_result = (MCMD_result_code_t) pstru_response->au8_data[0];;
        }
    }

    /* Release the Request exchange */
    xSemaphoreGiveRecursive (x_inst->x_sem_comm);

    return (s8_result);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Finalizes firmware update on Slave board
**
** @note
**      Master board should use s8_MCMD_Check_Bootloader_State() to check if firmware finalizaton is done successfully.
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      b_canceled
**      @arg    true: Cancel the firmware update
**      @arg    true: Finalize the firmware update
**
** @param [out]
**      penm_result: Result of the operation
**      @arg    MCMD_RESULT_OK
**      @arg    MCMD_RESULT_ERR_UNKNOWN
**      @arg    MCMD_RESULT_ERR_FW_UPDATE_NOT_STARTED
**      @arg    MCMD_RESULT_ERR_VALIDATION_FAILED
**      @arg    MCMD_RESULT_ERR_FW_DOWNLOAD_TIMEOUT
**      @arg    MCMD_RESULT_ERR_INSTALL_BL_FAILED
**
** @return
**      @arg    MCMD_OK
**      @arg    MCMD_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MCMD_Finalize_Update (MCMD_inst_t x_inst, bool b_canceled, MCMD_result_code_t * penm_result)
{
    ASSERT_PARAM (b_MCMD_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized && (penm_result != NULL));

    /* Take the Request exchange */
    xSemaphoreTakeRecursive (x_inst->x_sem_comm, portMAX_DELAY);

    /* Construct request message */
    MCMD_msg_t * pstru_request  = (MCMD_msg_t *)x_inst->au8_buf;
    pstru_request->u8_cid       = MCMD_FW_FINALIZE_WRITE_REQ;
    pstru_request->u8_status    = MCMD_STATUS_OK;

    /* Cancel or finalize */
    uint8_t u8_offset = 0;
    pstru_request->au8_data[u8_offset] = b_canceled ? 0x00 : 0x01;
    u8_offset += 1;

    /* Send the request message and wait for the response */
    MCMD_msg_t *    pstru_response;
    uint16_t        u16_response_len;
    int8_t s8_result = s8_MCMD_Send_Request (x_inst, pstru_request, u8_offset,
                                             &pstru_response, &u16_response_len, MCMD_DEFAULT_TIMEOUT);

    /* Check the response */
    if (s8_result >= MCMD_OK)
    {
        if (u16_response_len != 1)
        {
            s8_result = MCMD_ERR;
            LOGE ("Invalid response for request MCMD_FW_FINALIZE_WRITE_REQ");
        }
        else
        {
            *penm_result = (MCMD_result_code_t) pstru_response->au8_data[0];;
        }
    }

    /* Release the Request exchange */
    xSemaphoreGiveRecursive (x_inst->x_sem_comm);

    return (s8_result);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_PnP_Cmd module
**
** @return
**      @arg    MCMD_OK
**      @arg    MCMD_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MCMD_Init_Module (void)
{
    /* Do nothing */
    return MCMD_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes a Master commander
**
** @param [in]
**      x_inst: Specific instance
**
** @return
**      @arg    MCMD_OK
**      @arg    MCMD_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MCMD_Init_Inst (MCMD_inst_t x_inst)
{
    /* Get instance of the associated transport channel */
    if (s8_MTP_Get_Inst (&x_inst->x_transport_inst) < MTP_OK)
    {
        LOGE ("Failed to get instance of transport channel");
        return MCMD_ERR;
    }

    /* Initialize callback function pointers */
    for (uint8_t u8_idx = 0; u8_idx < MCMD_NUM_CB; u8_idx++)
    {
        x_inst->apfnc_cb [u8_idx] = NULL;
    }

    /* Create mutex protecting exchanges */
    x_inst->x_sem_comm = xSemaphoreCreateRecursiveMutex ();
    if (x_inst->x_sem_comm == NULL)
    {
        return MCMD_ERR;
    }

    /* Register callback function to event from transport layer */
    if (s8_MTP_Register_Cb (x_inst->x_transport_inst, v_MCMD_Transport_Cb) < MTP_OK)
    {
        LOGE ("Failed to register callback function to transport channel");
        return MCMD_ERR;
    }

    return MCMD_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Callback invoked when an event from transport layer fires
**
** @param [in]
**      x_transport_inst: Specific instance of transport layer
**
** @param [in]
**      enm_evt: The event from transport layer fired
**
** @param [in]
**      pv_data: Context data of the event
**
** @param [in]
**      u16_len: Length in bytes of the data in pv_data buffer
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MCMD_Transport_Cb (MTP_inst_t x_transport_inst, MTP_evt_t enm_evt, const void * pv_data, uint16_t u16_len)
{
    MCMD_inst_t x_inst = &g_stru_mcmd_obj;

    /* Process the event of receiving notification message */
    if (enm_evt == MTP_EVT_NOTIFY)
    {
        v_MCMD_Process_Notification (x_inst, (MCMD_msg_t *)pv_data, u16_len);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Processes a notification message received from client
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pstru_msg: The notification message received
**
** @param [in]
**      u16_msg_len: Length in bytes of the whole message (including header)
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MCMD_Process_Notification (MCMD_inst_t x_inst, MCMD_msg_t * pstru_msg, uint16_t u16_msg_len)
{
    /* Process each notification */
    switch (pstru_msg->u8_cid)
    {
        /* Slave board is in Bootloader mode */
        case MCMD_SCAN_NOTIFY:
        {
            if (u16_msg_len - sizeof (MCMD_msg_t) != 1)
            {
                LOGE ("Invalid SCAN_NOTIFY message received");
                break;
            }
            for (uint8_t u8_idx = 0; u8_idx < MCMD_NUM_CB; u8_idx++)
            {
                if (x_inst->apfnc_cb [u8_idx] != NULL)
                {
                    x_inst->apfnc_cb [u8_idx] (x_inst, MCMD_EVT_SLAVE_IN_BOOTLOADER,
                                               pstru_msg->au8_data, u16_msg_len - sizeof (MCMD_msg_t));
                }
            }
            break;
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a request command to transport layer and waits for response
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pstru_request: Request message to send
**
** @param [in]
**      u16_request_len: Length in bytes of the request data (not including header), 0 if no data
**
** @param [out]
**      ppstru_response: Pointer to the response
**
** @param [out]
**      pu16_response_len: Length in bytes of the response data (not including header), 0 if no data
**
** @param [out]
**      u16_timeout: Timeout in milliseconds of request retry
**
** @return
**      @arg    MCMD_OK
**      @arg    MCMD_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MCMD_Send_Request (MCMD_inst_t x_inst, MCMD_msg_t * pstru_request, uint16_t u16_request_len,
                                    MCMD_msg_t ** ppstru_response, uint16_t * pu16_response_len, uint16_t u16_timeout)
{
    /* Send the request message and wait for the response */
    int8_t s8_result = s8_MTP_Send_Request (x_inst->x_transport_inst,
                                              pstru_request, sizeof (MCMD_msg_t) + u16_request_len,
                                              (uint8_t **)ppstru_response, pu16_response_len, u16_timeout);

    /* Check the response */
    if (s8_result < MTP_OK)
    {
        LOGE ("Failed to send request 0x%02X", pstru_request->u8_cid);
    }
    else
    {
        if ((*pu16_response_len < sizeof (MCMD_msg_t)) ||
            ((*ppstru_response)->u8_cid != pstru_request->u8_cid))
        {
            s8_result = MCMD_ERR;
            LOGE ("Received invalid response of request 0x%02X (response length = %d, CID = 0x%02X)",
                      pstru_request->u8_cid, *pu16_response_len, (*ppstru_response)->u8_cid);
        }
        else
        {
            *pu16_response_len = *pu16_response_len - sizeof (MCMD_msg_t);
            if ((*ppstru_response)->u8_status != MCMD_STATUS_OK)
            {
                s8_result = MCMD_ERR;
                LOGE ("Request 0x%02X failed. Error code: 0x%02X",
                          pstru_request->u8_cid, (*ppstru_response)->u8_status);
            }
        }
    }

    return (s8_result < MTP_OK ? MCMD_ERR : MCMD_OK);
}

#ifdef USE_MODULE_ASSERT

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Check if an instance is a vaild instance of this module
**
** @param [in]
**      x_inst: instance to check
**
** @return
**      Result
**      @arg    true: Valid instance
**      @arg    false: Invalid instance
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static bool b_MCMD_Is_Valid_Inst (MCMD_inst_t x_inst)
{
    if (x_inst == &g_stru_mcmd_obj)
    {
        return true;
    }

    LOGE ("Invalid instance");
    return false;
}

#endif

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
