/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_fwu_esp32.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Aug 27
**  @brief      : Implementation of Srvc_Fwu_Esp32 module
**  @namespace  : FWUESP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Fwu_Esp32
** @brief       Provides helper APIs to update main application firmware of ESP32
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_fwu_esp32.h"         /* Public header of this module */

#include "esp_ota_ops.h"            /* Use ESP-IDF's OTA firmware update APIs */
#include <string.h>                 /* Use strncmp() */
#include <stdio.h>                  /* Use sscanf() */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  States of firmware update process */
typedef enum
{
    FWUESP_STATE_IDLE,              //!< No firmware update is currently performed
    FWUESP_STATE_READY,             //!< Ready for a new firmware update
    FWUESP_STATE_STARTED,           //!< Firmware update has been started and is in progress
} FWUESP_state_t;

/** @brief  Maximum size in bytes of firmware data chunk */
#define FWUESP_MAX_FW_DATA_CHUNK    8192

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Fwu_Esp32";

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Firmware update state */
static FWUESP_state_t g_enm_state = FWUESP_STATE_IDLE;

/** @brief  Firmware update handle */
static esp_ota_handle_t g_x_update_handle;

/** @brief  Size in bytes of the firmware to update */
static uint32_t g_u32_fw_size;

/** @brief  Number of bytes have been programmed onto flash */
static uint32_t g_u32_bytes_flashed = 0;

/** @brief  The flash partition where the new firmware is to be written to */
static const esp_partition_t * g_px_buf_part = NULL;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_Fwu_Esp32 module
**
** @note
**      This function should be the last one to be called during device initialization process because it will
**      confirm the proper operation of newly programmed firmware.
**
** @param [out]
**      b_first_run: Indicates if this is the first time this firmware run after being updated
**
** @return
**      @arg    FWUESP_OK
**      @arg    FWUESP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUESP_Init (bool * pb_first_run)
{
    ASSERT_PARAM (pb_first_run != NULL);

    LOGD ("Initializing Srvc_Fwu_Esp32 module");

    /* Initialization */
    *pb_first_run = false;

    /* If this module has been initialized, don't need to do that again */
    if (g_b_initialized)
    {
        return FWUESP_OK;
    }

    /* Get partition of the current running firmware */
    const esp_partition_t * px_app_part = esp_ota_get_running_partition ();
    if (px_app_part == NULL)
    {
        LOGE ("Failed to get partition of the current running firmware");
        return FWUESP_ERR;
    }

    /* Check if this is the first time this firmware runs after being programmed */
    esp_ota_img_states_t enm_fw_state;
    if (esp_ota_get_state_partition (px_app_part, &enm_fw_state) == ESP_OK)
    {
        if (enm_fw_state == ESP_OTA_IMG_PENDING_VERIFY)
        {
            /* On first run, mark this firmware as valid so that rollback process is not triggered */
            ESP_ERROR_CHECK (esp_ota_mark_app_valid_cancel_rollback ());
            *pb_first_run = true;
        }
    }

    /* Done */
    LOGD ("Initialization of Srvc_Fwu_Esp32 module is done");
    g_b_initialized = true;
    return FWUESP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets descriptor of current running firmware
**
** @param [out]
**      pstru_desc: Pointer to the buffer containing the descriptor.
**
** @return
**      @arg    FWUESP_OK
**      @arg    FWUESP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUESP_Get_Fw_Descriptor (FWUESP_fw_desc_t * pstru_desc)
{
    ASSERT_PARAM (pstru_desc != NULL);

    /* Get description structure of current running firmware */
    const esp_app_desc_t * pstru_app_desc = esp_ota_get_app_description ();
    if (pstru_app_desc == NULL)
    {
        LOGE ("Failed to get firmware descriptor");
        return FWUESP_ERR;
    }

    /* Date and time compiling the firmware */
    static char stri_time[32];
    snprintf (stri_time, sizeof (stri_time), "%s %s", pstru_app_desc->date, pstru_app_desc->time);
    stri_time[sizeof (stri_time) - 1] = '\0';

    /* Firmware descriptor */
    pstru_desc->pstri_name = pstru_app_desc->project_name;
    pstru_desc->pstri_ver = pstru_app_desc->version;
    pstru_desc->pstri_time = stri_time;

    return FWUESP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Prepares a new firmware update
**
** @param [in]
**      pstru_fw_info: Information of the firmware to update
**
** @param [out]
**      penm_result: Result of the preparation
**      @arg    FWUESP_RESULT_OK
**      @arg    FWUESP_RESULT_WARN_FW_OLDER
**      @arg    FWUESP_RESULT_WARN_FW_SAME
**      @arg    FWUESP_RESULT_ERR
**      @arg    FWUESP_RESULT_ERR_PRJ_MISMATCH
**      @arg    FWUESP_RESULT_ERR_FW_TOO_BIG
**      @arg    FWUESP_RESULT_ERR_NOT_FINALIZED
**
** @return
**      @arg    FWUESP_OK
**      @arg    FWUESP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUESP_Prepare_Update (const FWUESP_fw_info_t * pstru_fw_info, FWUESP_result_t * penm_result)
{
    ASSERT_PARAM (g_b_initialized && (pstru_fw_info != NULL) && (penm_result != NULL));

    /* Init */
    *penm_result = FWUESP_RESULT_OK;

    /* A firmware update can only be started if the previous one (if any) has been done */
    if (g_enm_state == FWUESP_STATE_STARTED)
    {
        *penm_result = FWUESP_RESULT_ERR_NOT_FINALIZED;
        return FWUESP_ERR;
    }

    /* Get partition of the current running firmware */
    const esp_partition_t * px_app_part = esp_ota_get_running_partition ();
    if (px_app_part == NULL)
    {
        *penm_result = FWUESP_RESULT_ERR;
        return FWUESP_ERR;
    }

    /* Get description structure of current running firmware */
    const esp_app_desc_t * pstru_app_desc = esp_ota_get_app_description ();
    if (pstru_app_desc == NULL)
    {
        *penm_result = FWUESP_RESULT_ERR;
        return FWUESP_ERR;
    }

    /* Check project name */
    if (strncmp (pstru_fw_info->stri_name, pstru_app_desc->project_name, sizeof (pstru_fw_info->stri_name)))
    {
        /* Project name of the given firmware doesn't match with the current running firmware */
        *penm_result = FWUESP_RESULT_ERR_PRJ_MISMATCH;
        return FWUESP_ERR;
    }

    /* The flash partition where the new firmware is to be written to */
    g_px_buf_part = esp_ota_get_next_update_partition (px_app_part);
    if (g_px_buf_part == NULL)
    {
        *penm_result = FWUESP_RESULT_ERR;
        return FWUESP_ERR;
    }

    /* Check firmware size */
    if (pstru_fw_info->u32_size > g_px_buf_part->size)
    {
        *penm_result = FWUESP_RESULT_ERR_FW_TOO_BIG;
        return FWUESP_ERR;
    }

    /* Check firmware revision */
    int32_t s32_major;
    int32_t s32_minor;
    int32_t s32_patch;
    if (sscanf (pstru_app_desc->version, "%d.%d.%d", &s32_major, &s32_minor, &s32_patch) != 3)
    {
        /* Format of version string is incorrect */
        *penm_result = FWUESP_RESULT_ERR;
        return FWUESP_ERR;
    }
    uint32_t u32_current_rev = (((uint32_t)(s32_major & 0xFF)) << 16) |
                               (((uint32_t)(s32_minor & 0xFF)) <<  8) |
                               (((uint32_t)(s32_patch & 0xFF)) <<  0);
    uint32_t u32_new_rev = ((uint32_t)pstru_fw_info->u8_major_rev << 16) |
                           ((uint32_t)pstru_fw_info->u8_minor_rev <<  8) |
                           ((uint32_t)pstru_fw_info->u8_patch_rev <<  0);
    if (u32_new_rev == u32_current_rev)
    {
        *penm_result = FWUESP_RESULT_WARN_FW_SAME;
    }
    else if (u32_new_rev < u32_current_rev)
    {
        *penm_result = FWUESP_RESULT_WARN_FW_OLDER;
    }

    /* The new firmware seems to be okay */
    LOGI ("Received a request to update firmware:");
    LOGI ("+ Firmware name: %s", pstru_fw_info->stri_name);
    LOGI ("+ Firmware revision: %d.%d.%d",
          pstru_fw_info->u8_major_rev, pstru_fw_info->u8_minor_rev, pstru_fw_info->u8_patch_rev);
    LOGI ("+ Firmware size: %d bytes", pstru_fw_info->u32_size);
    g_enm_state = FWUESP_STATE_READY;
    g_u32_fw_size = pstru_fw_info->u32_size;
    return FWUESP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Starts firmware update process
**
** @note
**      Once this function is called, s8_FWUESP_Finalize_Update() must be called to finish the firmware update process.
**
** @param [out]
**      penm_result: Result code
**      @arg    FWUESP_RESULT_OK
**      @arg    FWUESP_RESULT_ERR
**      @arg    FWUESP_RESULT_ERR_NOT_PREPARED
**
** @return
**      @arg    FWUESP_OK
**      @arg    FWUESP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUESP_Start_Update (FWUESP_result_t * penm_result)
{
    ASSERT_PARAM (g_b_initialized && (penm_result != NULL));

    /* Init */
    *penm_result = FWUESP_RESULT_OK;

    /* Check if we are ready to start the firmware update */
    if (g_enm_state != FWUESP_STATE_READY)
    {
        *penm_result = FWUESP_RESULT_ERR_NOT_PREPARED;
        return FWUESP_ERR;
    }

    /* Start the firmware update, this will erase the destination flash to make room for the new firmware */
    esp_err_t x_err = esp_ota_begin (g_px_buf_part, g_u32_fw_size, &g_x_update_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to start firmware update process (%s)", esp_err_to_name (x_err));
        *penm_result = FWUESP_RESULT_ERR;
        return FWUESP_ERR;
    }

    /* Firmware update process has been started */
    LOGI ("Firmware update started");
    g_enm_state = FWUESP_STATE_STARTED;
    g_u32_bytes_flashed = 0;
    return FWUESP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Programs each chunk of a firmware data onto flash
**
** @param [in]
**      pstru_fw_data: A chunk of firmware data to program
**
** @param [out]
**      penm_result: Result of the preparation
**      @arg    FWUESP_RESULT_OK
**      @arg    FWUESP_RESULT_ERR
**      @arg    FWUESP_RESULT_ERR_NOT_STARTED
**      @arg    FWUESP_RESULT_ERR_DATA_INVALID
**
** @return
**      @arg    FWUESP_OK
**      @arg    FWUESP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUESP_Program_Firmware (const FWUESP_data_chunk_t * pstru_fw_data, FWUESP_result_t * penm_result)
{
    ASSERT_PARAM (g_b_initialized && (pstru_fw_data != NULL) && (penm_result != NULL));
    ASSERT_PARAM (pstru_fw_data->u16_unpacked_len <= FWUESP_MAX_FW_DATA_CHUNK);

    /* Init */
    *penm_result = FWUESP_RESULT_OK;

    /* Check if firmware update process has been started */
    if (g_enm_state != FWUESP_STATE_STARTED)
    {
        *penm_result = FWUESP_RESULT_ERR_NOT_STARTED;
        return FWUESP_ERR;
    }

    /* Ensure that the firmware data chunk doesn't exceed the provided firmware data size */
    uint16_t u16_chunk_len = pstru_fw_data->u16_unpacked_len ? pstru_fw_data->u16_unpacked_len :
                                                               pstru_fw_data->u16_data_len;
    if (pstru_fw_data->u32_offset + u16_chunk_len > g_u32_fw_size)
    {
        *penm_result = FWUESP_RESULT_ERR_DATA_INVALID;
        return FWUESP_ERR;
    }

    /* Program the firmware data chunk onto destination partition */
    esp_err_t x_err = esp_ota_write_with_offset (g_x_update_handle, pstru_fw_data->pu8_firmware,
                                                 pstru_fw_data->u16_data_len, pstru_fw_data->u32_offset);
    g_u32_bytes_flashed += pstru_fw_data->u16_data_len;

    /* Check result */
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to write firmware data onto flash (%s)", esp_err_to_name (x_err));
        *penm_result = FWUESP_RESULT_ERR;
        return FWUESP_ERR;
    }

    // LOGI ("Programming new firmware... %.1f%% (%d/%d bytes)", g_u32_bytes_flashed * 100.0 / g_u32_fw_size,
                                                              // g_u32_bytes_flashed, g_u32_fw_size);
    return FWUESP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Cancels or finalizes current firmware update process
**
** @note
**      Once s8_FWUESP_Start_Update() has been called, this function must be called to finish the firmware update
**      process.
**
** @param [in]
**      b_finalized:
**      @arg    false: cancel firmware update process
**      @arg    true: finish and finalize firmware update process
**
** @param [out]
**      penm_result: Result of the preparation
**      @arg    FWUESP_RESULT_OK
**      @arg    FWUESP_RESULT_ERR
**      @arg    FWUESP_RESULT_ERR_NOT_STARTED
**      @arg    FWUESP_RESULT_ERR_FW_INVALID
**
** @return
**      @arg    FWUESP_OK
**      @arg    FWUESP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_FWUESP_Finalize_Update (bool b_finalized, FWUESP_result_t * penm_result)
{
    ASSERT_PARAM (g_b_initialized && (penm_result != NULL));

    /* Init */
    *penm_result = FWUESP_RESULT_OK;

    /* Check if firmware update process has been started */
    if (g_enm_state != FWUESP_STATE_STARTED)
    {
        *penm_result = FWUESP_RESULT_ERR_NOT_STARTED;
        LOGW ("Firmware update terminated");
        return FWUESP_ERR;
    }

    /* Mark firmware update process as being done */
    g_enm_state = FWUESP_STATE_IDLE;

    /* If firmware update process is canceled */
    if (!b_finalized)
    {
        esp_ota_abort (g_x_update_handle);
        LOGW ("Firmware update aborted");
    }
    else
    {
        /* Finish firmware update process and validate newly written app image */
        esp_err_t x_err = esp_ota_end (g_x_update_handle);
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to finalize firmware update process (%s)", esp_err_to_name (x_err));
            *penm_result = (x_err == ESP_ERR_OTA_VALIDATE_FAILED) ? FWUESP_RESULT_ERR_FW_INVALID : FWUESP_RESULT_ERR;
            return FWUESP_ERR;
        }

        /* Activate the new firmware for next bootup */
        x_err = esp_ota_set_boot_partition (g_px_buf_part);
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to activate new firmwre (%s)", esp_err_to_name (x_err));
            *penm_result = FWUESP_RESULT_ERR;
            return FWUESP_ERR;
        }
        LOGI ("Firmware update is done successfully");
    }

    return FWUESP_OK;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
