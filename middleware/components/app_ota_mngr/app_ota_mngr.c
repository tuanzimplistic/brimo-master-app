/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : app_ota_mngr.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Aug 28
**  @brief      : Implementation of App_Ota_Mngr module
**  @namespace  : OTAMN
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Ota_Mngr
** @brief       Performs Over-The-Air update of different components such as master board's firmware, slave board's
**              firmware, etc.
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "app_ota_mngr.h"               /* Public header of this module */
#include "app_mqtt_mngr.h"              /* Send notification over MQTT interface */
#include "app_gui_mngr.h"               /* Display message box on GUI while updating */
#include "srvc_fwu_esp32.h"             /* Use ESP32 firmware update service */
#include "srvc_fwu_slave.h"             /* Use slave firmware update service */

#include "esp_system.h"                 /* Use esp_restart() */
#include "esp_http_client.h"            /* Use HTTP client of IDF framework */
#include "esp_ota_ops.h"                /* Use structures in ESP32 firmware header */
#include "esp32/rom/crc.h"              /* Use ESP-IDF's CRC API */
#include <string.h>                     /* Use strncpy(), memcpy(), sprintf(), etc. */
#include <stdio.h>                      /* Use sscanf() */

#include "esp_ota_ops.h"                /* Use ESP-IDF's OTA firmware update APIs */
#include "esp_partition.h"              /* Use ESP-IDF partition API */
#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/task.h"              /* Use FreeRTOS task */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  ID of the CPU that OTA update tasks run on */
#define OTAMN_TASK_CPU_ID               1

/** @brief  Stack size (in bytes) of OTA update tasks */
#define OTAMN_TASK_STACK_SIZE           4096

/** @brief  Priority of OTA update tasks */
#define OTAMN_TASK_PRIORITY             (tskIDLE_PRIORITY + 1)

/**
** @brief   Size in bytes of each download data chunk from OTA source
** @note    This size must be large enough to contain firmware descriptor in the first chunk. It must be < 65535
*/
#define OTAMN_DOWNLOAD_CHUNK_SIZE       2048

/** @brief  Macros notifying OTA download progress, install progrss, and overall status over MQTT interface */
#ifdef CONFIG_OTA_NOTIFY_OVER_MQTT
#   define OTAMN_NOTIFY_DOWNLOAD_PROGRESS_MQTT(u8_percents)     s8_MQTTMN_Notify_Ota_Download_Progress (u8_percents)
#   define OTAMN_NOTIFY_INSTALL_PROGRESS_MQTT(u8_percents)      s8_MQTTMN_Notify_Ota_Install_Progress (u8_percents)
#   define OTAMN_NOTIFY_STATUS_MQTT(b_ok, pstri_error)          s8_MQTTMN_Notify_Ota_Status (b_ok, pstri_error)
#else
#   define OTAMN_NOTIFY_DOWNLOAD_PROGRESS_MQTT(u8_percents)     ;
#   define OTAMN_NOTIFY_INSTALL_PROGRESS_MQTT(u8_percents)      ;
#   define OTAMN_NOTIFY_STATUS_MQTT(b_ok, pstri_error)          ;
#endif

/** @brief  Name of the temporary file created while OTA updating file */
#define OTAMN_TEMP_FILE                 "./~temp.tmp"

/** @brief  States of OTA firmware update */
typedef enum
{
    OTAMN_STATE_DOWNLOAD,               //!< The firmware is being downloaded OTA
    OTAMN_STATE_INSTALL,                //!< The firmware is being installed
    OTAMN_STATE_RESTART,                //!< The target component is being restarted

} OTAMN_state_t;

/** @brief  Size in byte of a slave firmware data chunk */
#define OTAMN_SLAVE_FW_CHUNK_SIZE       196

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static void v_OTAMN_Update_Master_Firmware_Task (void * pv_param);
static int8_t s8_OTAMN_Update_Master_Firmware (OTAMN_config_t * pstru_config, const char * pstri_ca_cert);
static void v_OTAMN_Update_Slave_Firmware_Task (void * pv_param);
static int8_t s8_OTAMN_Download_Slave_Firmware (OTAMN_config_t * pstru_config, const char * pstri_ca_cert);
static int8_t s8_OTAMN_Install_Slave_Firmware (OTAMN_config_t * pstru_config);
static void v_OTAMN_Update_Master_File_Task (void * pv_param);
static int8_t s8_OTAMN_Update_Master_File (OTAMN_config_t * pstru_config, const char * pstri_ca_cert);
static void v_OTAMN_Create_Folder (const char * pstri_path);
static const char * pstri_OTAMN_Get_File_Name (const char * pstri_path);
static void v_OTAMN_Cleanup (OTAMN_config_t * pstru_config);
static void v_OTAMN_Notify_Progress_Gui (OTAMN_target_t enm_target, OTAMN_state_t enm_state, uint8_t u8_percents);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "App_Ota_Mngr";

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Indicates if OTA Manager is busy */
static bool g_b_busy = false;

/** @brief  Request the ongoing OTA update process (if any) to be cancelled */
static bool g_b_cancelled = false;

/** @brief  Certificate of the server storing update components */
extern const char g_stri_ca_cert[] asm("_binary_ca_cert_aws_s3_pem_start");

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes App_Ota_Mngr module
**
** @note
**      This function should be the last one to be called during device initialization process because it will
**      confirm the proper operation of newly programmed firmware.
**
** @return
**      @arg    OTAMN_OK
**      @arg    OTAMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_OTAMN_Init (void)
{
    /* Do nothing if this module has been initialized */
    if (g_b_initialized)
    {
        return OTAMN_OK;
    }

    LOGD ("Initializing App_Ota_Mngr module");

    /* Initialize ESP32 firmware update service */
    bool b_first_run = false;
    if (s8_FWUESP_Init (&b_first_run) != FWUESP_OK)
    {
        LOGE ("Failed to initialize ESP32 firmware update service");
        return OTAMN_ERR;
    }

    /* Initialize slave firmware update service */
    if (s8_FWUSLV_Init () != FWUSLV_OK)
    {
        LOGE ("Failed to initialize slave firmware update service");
        return OTAMN_ERR;
    }

    /* If this is the first time this firmware runs after being updated */
    if (b_first_run)
    {
        /* Notify user */
        FWUESP_fw_desc_t stru_fw_desc;
        s8_FWUESP_Get_Fw_Descriptor (&stru_fw_desc);
        LOGI ("*** ESP32 firmware v%s has been running successfully ***", stru_fw_desc.pstri_ver);
        GUI_notify_t stru_notify =
        {
            .enm_type       = GUI_MSG_INFO,
            .pstri_brief    = "OTA firmware update",
            .pstri_detail   = "The new firmware has been installed and run successfully on master board.",
            .u32_wait_time  = 0
        };
        s8_GUI_Notify (&stru_notify);
    }

    /* Done */
    LOGD ("Initialization of App_Ota_Mngr module is done");
    g_b_initialized = true;
    return OTAMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Starts OTA update of a component
**
** @note
**      This function returns immediately, the update is performed asynchronously in the background
**
** @param [in]
**      pstru_config: Configuration of the OTA update
**
** @return
**      @arg    OTAMN_OK
**      @arg    OTAMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_OTAMN_Start (const OTAMN_config_t * pstru_config)
{
    static OTAMN_config_t stru_config;;

    /* Validate arguments */
    ASSERT_PARAM (g_b_initialized && (pstru_config != NULL) &&
                  (pstru_config->enm_target < OTAMN_NUM_TARGETS) &&
                  (pstru_config->pstri_url != NULL));

    /* Check if OTA Manager is busy */
    if (g_b_busy)
    {
        LOGE ("OTA Manager is busy and cannot perform the OTA request");
        return OTAMN_ERR;
    }

    /* Start the OTA update */
    g_b_busy = true;
    g_b_cancelled = false;

    /* Store the OTA configuration */
    stru_config = *pstru_config;

    /* Allocate buffer to store URL of the OTA source */
    uint16_t u16_url_len = strlen (pstru_config->pstri_url) + 1;
    stru_config.pstri_url = malloc (u16_url_len);
    if (stru_config.pstri_url == NULL)
    {
        LOGE ("Failed to allocate memory to store URL for OTA");
        v_OTAMN_Cleanup (&stru_config);
        return OTAMN_ERR;
    }
    memcpy (stru_config.pstri_url, pstru_config->pstri_url, u16_url_len);

    /* Allocate buffer to store installation directory if existing */
    if (pstru_config->pstri_inst_dir != NULL)
    {
        uint16_t u16_path_len = strlen (pstru_config->pstri_inst_dir) + 1;
        stru_config.pstri_inst_dir = malloc (u16_path_len);
        if (stru_config.pstri_inst_dir == NULL)
        {
            LOGE ("Failed to allocate memory to store installation directory for OTA");
            v_OTAMN_Cleanup (&stru_config);
            return OTAMN_ERR;
        }
        memcpy (stru_config.pstri_inst_dir, pstru_config->pstri_inst_dir, u16_path_len);
    }

    /* Determine the task to perform the OTA update */
    TaskFunction_t pfnc_task = (stru_config.enm_target == OTAMN_MASTER_FW)   ? v_OTAMN_Update_Master_Firmware_Task  :
                               (stru_config.enm_target == OTAMN_SLAVE_FW)    ? v_OTAMN_Update_Slave_Firmware_Task   :
                               (stru_config.enm_target == OTAMN_MASTER_FILE) ? v_OTAMN_Update_Master_File_Task      :
                               NULL;

    /* Create task performing the OTA update, this task will be deleted when OTA update is done */
    BaseType_t x_result =
        xTaskCreatePinnedToCore (pfnc_task,                 /* Function that implements the task */
                                 "App_Ota_Mngr",            /* Text name for the task */
                                 OTAMN_TASK_STACK_SIZE,     /* Stack size in bytes, not words */
                                 &stru_config,              /* Parameter passed into the task */
                                 OTAMN_TASK_PRIORITY,       /* Priority at which the task is created */
                                 NULL,                      /* Handle of the created task */
                                 OTAMN_TASK_CPU_ID);        /* ID of the CPU that the task runs on */
    if (x_result != pdPASS)
    {
        LOGE ("Failed to create task performing the OTA update");
        v_OTAMN_Cleanup (&stru_config);
        return OTAMN_ERR;
    }

    /* OTA update of the target component will be performed in the background */
    return OTAMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Cancels the ongoing OTA update (if any)
**
** @return
**      @arg    OTAMN_OK
**      @arg    OTAMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_OTAMN_Cancel (void)
{
    ASSERT_PARAM (g_b_initialized);

    /* Request ongoing OTA update process (if any) to cancel */
    g_b_cancelled = true;

    return OTAMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Task performing OTA update for firmware of Master board
**
** @param [in]
**      pv_param: Parameter passed into the task
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_OTAMN_Update_Master_Firmware_Task (void * pv_param)
{
    /* Validation */
    ASSERT_PARAM (g_b_busy && (pv_param != NULL));
    LOGI ("OTA firmware update for Master board starts");

    /* OTA configuration */
    OTAMN_config_t * pstru_config = (OTAMN_config_t *)pv_param;

    /* Perform the update, retry if update fails */
    int8_t s8_result;
    for (uint8_t u8_retry = 0; u8_retry < 3; u8_retry++)
    {
        if (u8_retry != 0)
        {
            LOGE ("OTA update failed. Retrying %d...", u8_retry);
            vTaskDelay (pdMS_TO_TICKS (1000));
        }

        s8_result = s8_OTAMN_Update_Master_Firmware (pstru_config, g_stri_ca_cert);
        if (s8_result != OTAMN_ERR)
        {
            break;
        }
    }

    /* Check result */
    if (s8_result == OTAMN_OK)
    {
        LOGI ("Restarting...");
        v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FW, OTAMN_STATE_RESTART, 0);

        /* Wait 1 second for all notification to be sent out */
        vTaskDelay (pdMS_TO_TICKS (1000));

        /* Restart to use new firmware. This function never returns */
        esp_restart ();
    }
    else if (s8_result == OTAMN_CANCELLED)
    {
        /* Hide progress from GUI */
        v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FW, OTAMN_STATE_RESTART, 0xFF);

        /* Notify user */
        GUI_notify_t stru_notify =
        {
            .enm_type       = GUI_MSG_WARNING,
            .pstri_brief    = "OTA firmware update",
            .pstri_detail   = "OTA firmware update of master board has been cancelled.",
            .u32_wait_time  = 0
        };
        s8_GUI_Notify (&stru_notify);
    }
    else if (s8_result == OTAMN_IGNORED)
    {
        /* Do nothing */
        LOGI ("Ignored the OTA update of master board's firmware");
    }
    else
    {
        /* Hide progress from GUI */
        v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FW, OTAMN_STATE_RESTART, 0xFF);

        /* Notify user */
        GUI_notify_t stru_notify =
        {
            .enm_type       = GUI_MSG_ERROR,
            .pstri_brief    = "OTA firmware update",
            .pstri_detail   = "Failed to update firmware of master board.",
            .u32_wait_time  = 0
        };
        s8_GUI_Notify (&stru_notify);
    }

    /* Done */
    v_OTAMN_Cleanup (pstru_config);
    vTaskDelete (NULL);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Downloads firmware of master board from the corresponding HTTPs server, validates it, and install it onto
**      internal flash of ESP32.
**
** @param [in]
**      pstru_config: OTA configuration
**
** @param [in]
**      pstri_ca_cert: Certificate (NULL-terminated string) of the HTTPs server storing the firmware
**
** @return
**      @arg    OTAMN_OK
**      @arg    OTAMN_ERR
**      @arg    OTAMN_CANCELLED
**      @arg    OTAMN_IGNORED
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_OTAMN_Update_Master_Firmware (OTAMN_config_t * pstru_config, const char * pstri_ca_cert)
{
    esp_http_client_handle_t    x_https_client  = NULL;
    int8_t                      s8_result       = OTAMN_OK;
    bool                        b_http_open     = false;
    uint8_t *                   pu8_chunk_data  = NULL;
    int32_t                     s32_total_size  = 0;
    uint32_t                    u32_done_size   = 0;
    uint8_t                     u8_percents     = 0;
    FWUESP_result_t             enm_result_code = FWUESP_RESULT_OK;

    /* Configuration of HTTP client */
    esp_http_client_config_t stru_http_client_cfg =
    {
        .url                = pstru_config->pstri_url,
        .cert_pem           = pstri_ca_cert,
        .timeout_ms         = 10000,
        .keep_alive_enable  = true,
        .buffer_size        = 2048,
        .buffer_size_tx     = 1024,
    };

    /* Start HTTP session. Note that esp_http_client_cleanup() must be called when HTTP session is done */
    if (s8_result == OTAMN_OK)
    {
        x_https_client = esp_http_client_init (&stru_http_client_cfg);
        if (x_https_client == NULL)
        {
            LOGE ("Failed to initialise HTTPs connection");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to initialise HTTPs connection");
            s8_result = OTAMN_ERR;
        }
    }

    /* Open the HTTP connection for reading */
    if (s8_result == OTAMN_OK)
    {
        esp_err_t x_err = esp_http_client_open (x_https_client, 0);
        if (x_err == ESP_OK)
        {
            b_http_open = true;
        }
        else
        {
            LOGE ("Failed to open HTTPs connection: %s", esp_err_to_name (x_err));
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to open HTTPs connection");
            s8_result = OTAMN_ERR;
        }
    }

    /* Read from HTTP stream and process all response headers to get size of the firmware to download */
    if (s8_result == OTAMN_OK)
    {
        s32_total_size = esp_http_client_fetch_headers (x_https_client);
        if (s32_total_size < 0)
        {
            LOGE ("Failed to process HTTPs response headers");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to process HTTPs response headers");
            s8_result = OTAMN_ERR;
        }
        else if (s32_total_size == 0)
        {
            LOGE ("Failed to reach the firmware file to download");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to reach the firmware file to download");
            s8_result = OTAMN_ERR;
        }
        else if (s32_total_size < 256 * 1024)
        {
            LOGE ("Firmware size of %d bytes is invalid", s32_total_size);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Firmware size is invalid");
            s8_result = OTAMN_ERR;
        }
    }

    /* Allocate buffer for data chunk downloaded */
    if (s8_result == OTAMN_OK)
    {
        pu8_chunk_data = malloc (OTAMN_DOWNLOAD_CHUNK_SIZE);
        if (pu8_chunk_data == NULL)
        {
            LOGE ("Failed to allocate memory of %d bytes for download data chunk", OTAMN_DOWNLOAD_CHUNK_SIZE);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Not enough memory");
            s8_result = OTAMN_ERR;
        }
    }

    /* Download firmware from the HTTPs server */
    while (s8_result == OTAMN_OK)
    {
        /* Get one data chunk */
        int32_t s32_data_len = esp_http_client_read (x_https_client, (char *)pu8_chunk_data, OTAMN_DOWNLOAD_CHUNK_SIZE);
        if (s32_data_len < 0)
        {
            LOGE ("Failed to download firmware data chunk (offset %d bytes) from the server", u32_done_size);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to download firmware data chunk from the server");
            s8_result = OTAMN_ERR;
            break;
        }
        else if (s32_data_len == 0)
        {
            if (esp_http_client_is_complete_data_received (x_https_client))
            {
                LOGI ("Downloading completed");
                OTAMN_NOTIFY_DOWNLOAD_PROGRESS_MQTT (100);
                v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FW, OTAMN_STATE_DOWNLOAD, 100);

                /* Finalize ESP32 firmware update process */
                if (s8_FWUESP_Finalize_Update (true, &enm_result_code) == FWUESP_OK)
                {
                    LOGI ("New firmware for ESP32 has been installed successfully.");
                    OTAMN_NOTIFY_INSTALL_PROGRESS_MQTT (100);
                    v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FW, OTAMN_STATE_INSTALL, 100);

                    vTaskDelay (pdMS_TO_TICKS (100));
                    OTAMN_NOTIFY_STATUS_MQTT (true, NULL);
                }
                else
                {
                    if (enm_result_code == FWUESP_RESULT_ERR_FW_INVALID)
                    {
                        LOGE ("Firmware validation failed");
                        OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Firmware validation failed");
                    }
                    else
                    {
                        LOGE ("Failed to finalize firmware update process");
                        OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to finalize firmware update process");
                    }
                    s8_result = OTAMN_ERR;
                }
            }
            else
            {
                LOGE ("Connection closed");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Connection closed");
                s8_result = OTAMN_ERR;
            }
            break;
        }

        /* If this is the first data chunk, obtain and validate firmware descriptor */
        if (u32_done_size == 0)
        {
            const uint16_t u16_fw_desc_offset = sizeof (esp_image_header_t) + sizeof (esp_image_segment_header_t);
            if (s32_data_len < u16_fw_desc_offset + sizeof (esp_app_desc_t))
            {
                LOGE ("Failed to get firmware descriptor");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to get firmware descriptor");
                s8_result = OTAMN_ERR;
                break;
            }

            /* Firmware descriptor */
            esp_app_desc_t * pstru_desc = (esp_app_desc_t *)&pu8_chunk_data [u16_fw_desc_offset];

            /* Validate the descriptor */
            if (pstru_desc->magic_word != ESP_APP_DESC_MAGIC_WORD)
            {
                LOGE ("Invalid firmware descriptor");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Invalid firmware descriptor");
                s8_result = OTAMN_ERR;
                break;
            }

            /* Version of the new firmware */
            int32_t s32_major;
            int32_t s32_minor;
            int32_t s32_patch;
            if (sscanf (pstru_desc->version, "%d.%d.%d", &s32_major, &s32_minor, &s32_patch) != 3)
            {
                /* Format of version string is incorrect */
                LOGE ("Format of version string is incorrect");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Format of version string is incorrect");
                s8_result = OTAMN_ERR;
                break;
            }

            /* Prepare ESP32 firmware update process (skip validating firmware version) */
            FWUESP_fw_info_t stru_fw_info =
            {
                .u8_major_rev   = (uint8_t)s32_major,
                .u8_minor_rev   = (uint8_t)s32_minor,
                .u8_patch_rev   = (uint8_t)s32_patch,
                .u32_size       = s32_total_size,
            };
            strncpy (stru_fw_info.stri_name, pstru_desc->project_name, sizeof (stru_fw_info.stri_name));
            stru_fw_info.stri_name[sizeof (stru_fw_info.stri_name) - 1] = '\0';

            if (s8_FWUESP_Prepare_Update (&stru_fw_info, &enm_result_code) == FWUESP_OK)
            {
                if ((enm_result_code == FWUESP_RESULT_WARN_FW_OLDER) ||
                    (enm_result_code == FWUESP_RESULT_WARN_FW_SAME))
                {
                    LOGW ("The new firmware is NOT newer than the current running firmware");
                    if (pstru_config->b_check_newer)
                    {
                        OTAMN_NOTIFY_STATUS_MQTT (false, "Error: The new firmware is NOT newer than the current firmware");
                        s8_result = OTAMN_IGNORED;
                        break;
                    }
                }
            }
            else
            {
                if (enm_result_code == FWUESP_RESULT_ERR_PRJ_MISMATCH)
                {
                    LOGE ("Not a firmware for Master board");
                    OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Not a firmware for Master board");
                }
                else if (enm_result_code == FWUESP_RESULT_ERR_FW_TOO_BIG)
                {
                    LOGE ("Firmware size is too big");
                    OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Firmware size is too big");
                }
                else
                {
                    LOGE ("Failed to prepare firmware update process");
                    OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to prepare firmware update process");
                }
                s8_result = OTAMN_ERR;
                break;
            }

            /* Start ESP32 firmware update process */
            if (s8_FWUESP_Start_Update (&enm_result_code) != FWUESP_OK)
            {
                LOGE ("Failed to start ESP32 firmware update process");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to start ESP32 firmware update process");
                s8_result = OTAMN_ERR;
                break;
            }
        }

        /* Notify download progress */
        uint8_t u8_new_percents = u32_done_size * 100 / s32_total_size;
        if ((u32_done_size == 0) || (u8_new_percents != u8_percents))
        {
            u8_percents = u8_new_percents;
            LOGI ("Downloading master firmware... %d%%", u8_percents);
            OTAMN_NOTIFY_DOWNLOAD_PROGRESS_MQTT (u8_percents);
            v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FW, OTAMN_STATE_DOWNLOAD, u8_percents);
        }

        /* Program the firmware data chunk onto flash */
        FWUESP_data_chunk_t stru_chunk =
        {
            .u32_offset         = u32_done_size,
            .u16_data_len       = s32_data_len,
            .u16_unpacked_len   = 0,
            .pu8_firmware       = pu8_chunk_data,
        };
        if (s8_FWUESP_Program_Firmware (&stru_chunk, &enm_result_code) != FWUESP_OK)
        {
            LOGE ("Failed to program firmware data chunk at offset %d", u32_done_size);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to program firmware data chunk");
            s8_result = OTAMN_ERR;
            break;
        }

        /* This data chunk has been flashed successfully */
        u32_done_size += s32_data_len;

        /* Check if user want to cancel OTA update process */
        if (g_b_cancelled)
        {
            s8_FWUESP_Finalize_Update (false, &enm_result_code);

            LOGW ("Firmware update process has been cancelled");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Firmware update process is cancelled");
            s8_result = OTAMN_CANCELLED;
            break;
        }
    }

    /* Cleanup */
    if (pu8_chunk_data != NULL)
    {
        free (pu8_chunk_data);
    }
    if (b_http_open)
    {
        esp_http_client_close (x_https_client);
    }
    if (x_https_client != NULL)
    {
        esp_http_client_cleanup (x_https_client);
    }

    /* Check result */
    if (s8_result == OTAMN_ERR)
    {
        /* Cancel firmware update */
        s8_FWUESP_Finalize_Update (false, &enm_result_code);
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Task performing OTA update for firmware of Slave board
**
** @param [in]
**      pv_param: Parameter passed into the task
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_OTAMN_Update_Slave_Firmware_Task (void * pv_param)
{
    int8_t s8_result = OTAMN_OK;

    /* Validation */
    ASSERT_PARAM (g_b_busy && (pv_param != NULL));
    LOGI ("OTA firmware update for Slave board starts");

    /* OTA configuration */
    OTAMN_config_t * pstru_config = (OTAMN_config_t *)pv_param;

    /* Download slave firmware and store in OTA buffer, retry if downloading fails */
    for (uint8_t u8_retry = 0; u8_retry < 3; u8_retry++)
    {
        if (u8_retry != 0)
        {
            LOGE ("Failed to download slave firmware. Retrying %d...", u8_retry);
            vTaskDelay (pdMS_TO_TICKS (1000));
        }

        LOGI ("Start downloading slave firmware from cloud server");
        s8_result = s8_OTAMN_Download_Slave_Firmware (pstru_config, g_stri_ca_cert);
        if (s8_result != OTAMN_ERR)
        {
            break;
        }
    }

    /* Install the downloaded slave firmware onto slave board, retry if installation fails */
    if (s8_result == OTAMN_OK)
    {
        for (uint8_t u8_retry = 0; u8_retry < 3; u8_retry++)
        {
            if (u8_retry != 0)
            {
                LOGE ("Failed to install slave firmware. Retrying %d...", u8_retry);
                vTaskDelay (pdMS_TO_TICKS (1000));
            }

            LOGI ("Start flashing firmware onto slave board");
            s8_result = s8_OTAMN_Install_Slave_Firmware (pstru_config);
            if (s8_result != OTAMN_ERR)
            {
                break;
            }
        }
    }

    /* Check result */
    if (s8_result == OTAMN_OK)
    {
        /* Hide progress from GUI */
        v_OTAMN_Notify_Progress_Gui (OTAMN_SLAVE_FW, OTAMN_STATE_INSTALL, 0xFF);

        /* Notify user */
        GUI_notify_t stru_notify =
        {
            .enm_type       = GUI_MSG_INFO,
            .pstri_brief    = "OTA firmware update",
            .pstri_detail   = "Firmware of slave board has been updated successfully.",
            .u32_wait_time  = 0
        };
        s8_GUI_Notify (&stru_notify);
    }
    else if (s8_result == OTAMN_CANCELLED)
    {
        /* Hide progress from GUI */
        v_OTAMN_Notify_Progress_Gui (OTAMN_SLAVE_FW, OTAMN_STATE_INSTALL, 0xFF);

        /* Notify user */
        GUI_notify_t stru_notify =
        {
            .enm_type       = GUI_MSG_WARNING,
            .pstri_brief    = "OTA firmware update",
            .pstri_detail   = "OTA firmware update of slave board has been cancelled.",
            .u32_wait_time  = 0
        };
        s8_GUI_Notify (&stru_notify);
    }
    else if (s8_result == OTAMN_IGNORED)
    {
        /* Do nothing */
        LOGI ("Ignored the OTA update of slave board's firmware");
    }
    else
    {
        /* Hide progress from GUI */
        v_OTAMN_Notify_Progress_Gui (OTAMN_SLAVE_FW, OTAMN_STATE_INSTALL, 0xFF);

        /* Notify user */
        GUI_notify_t stru_notify =
        {
            .enm_type       = GUI_MSG_ERROR,
            .pstri_brief    = "OTA firmware update",
            .pstri_detail   = "Failed to update firmware of slave board.",
            .u32_wait_time  = 0
        };
        s8_GUI_Notify (&stru_notify);
    }

    /* Done */
    v_OTAMN_Cleanup (pstru_config);
    vTaskDelete (NULL);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Downloads firmware of slave board from the corresponding HTTPs server, validates and stores it into OTA buffer
**
** @param [in]
**      pstru_config: OTA configuration
**
** @param [in]
**      pstri_ca_cert: Certificate (NULL-terminated string) of the HTTPs server storing the firmware
**
** @return
**      @arg    OTAMN_OK
**      @arg    OTAMN_ERR
**      @arg    OTAMN_CANCELLED
**      @arg    OTAMN_IGNORED
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_OTAMN_Download_Slave_Firmware (OTAMN_config_t * pstru_config, const char * pstri_ca_cert)
{
    esp_http_client_handle_t    x_https_client  = NULL;
    int8_t                      s8_result       = OTAMN_OK;
    bool                        b_http_open     = false;
    uint8_t *                   pu8_chunk_data  = NULL;
    int32_t                     s32_total_size  = 0;
    uint32_t                    u32_done_size   = 0;
    uint8_t                     u8_percents     = 0;
    const esp_partition_t *     px_buf_part     = NULL;
    uint32_t                    u32_fw_crc      = 0;
    uint32_t                    u32_calc_crc    = 0;

    /* Configuration of HTTP client */
    esp_http_client_config_t stru_http_client_cfg =
    {
        .url                = pstru_config->pstri_url,
        .cert_pem           = pstri_ca_cert,
        .timeout_ms         = 10000,
        .keep_alive_enable  = true,
        .buffer_size        = 2048,
        .buffer_size_tx     = 1024,
    };

    /* Start HTTP session. Note that esp_http_client_cleanup() must be called when HTTP session is done */
    if (s8_result == OTAMN_OK)
    {
        x_https_client = esp_http_client_init (&stru_http_client_cfg);
        if (x_https_client == NULL)
        {
            LOGE ("Failed to initialise HTTPs connection");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to initialise HTTPs connection");
            s8_result = OTAMN_ERR;
        }
    }

    /* Open the HTTP connection for reading */
    if (s8_result == OTAMN_OK)
    {
        esp_err_t x_err = esp_http_client_open (x_https_client, 0);
        if (x_err == ESP_OK)
        {
            b_http_open = true;
        }
        else
        {
            LOGE ("Failed to open HTTPs connection: %s", esp_err_to_name (x_err));
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to open HTTPs connection");
            s8_result = OTAMN_ERR;
        }
    }

    /* Read from HTTP stream and process all response headers to get size of the firmware to download */
    if (s8_result == OTAMN_OK)
    {
        s32_total_size = esp_http_client_fetch_headers (x_https_client);
        if (s32_total_size < 0)
        {
            LOGE ("Failed to process HTTPs response headers");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to process HTTPs response headers");
            s8_result = OTAMN_ERR;
        }
        else if (s32_total_size == 0)
        {
            LOGE ("Failed to reach the firmware file to download");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to reach the firmware file to download");
            s8_result = OTAMN_ERR;
        }
        else if ((s32_total_size < 8 * 1024) || (s32_total_size > 512 * 1024))
        {
            LOGE ("Firmware size of %d bytes is invalid", s32_total_size);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Firmware size is invalid");
            s8_result = OTAMN_ERR;
        }
    }

    /* Allocate buffer for data chunk downloaded */
    if (s8_result == OTAMN_OK)
    {
        pu8_chunk_data = malloc (OTAMN_DOWNLOAD_CHUNK_SIZE);
        if (pu8_chunk_data == NULL)
        {
            LOGE ("Failed to allocate memory of %d bytes for download data chunk", OTAMN_DOWNLOAD_CHUNK_SIZE);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Not enough memory");
            s8_result = OTAMN_ERR;
        }
    }

    /* Get the OTA partition */
    if (s8_result == OTAMN_OK)
    {
        px_buf_part = esp_ota_get_next_update_partition (NULL);
        if (px_buf_part == NULL)
        {
            LOGE ("Failed to access OTA partition");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to access OTA partition");
            s8_result = OTAMN_ERR;
        }
    }

    /* Download firmware from the HTTPs server */
    while (s8_result == OTAMN_OK)
    {
        /* Get one data chunk */
        int32_t s32_data_len = esp_http_client_read (x_https_client, (char *)pu8_chunk_data, OTAMN_DOWNLOAD_CHUNK_SIZE);
        if (s32_data_len < 0)
        {
            LOGE ("Failed to download firmware data chunk (offset %d bytes) from the server", u32_done_size);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to download firmware data chunk from the server");
            s8_result = OTAMN_ERR;
            break;
        }
        else if (s32_data_len == 0)
        {
            if (esp_http_client_is_complete_data_received (x_https_client))
            {
                LOGI ("Downloading completed");
                OTAMN_NOTIFY_DOWNLOAD_PROGRESS_MQTT (100);
                v_OTAMN_Notify_Progress_Gui (OTAMN_SLAVE_FW, OTAMN_STATE_DOWNLOAD, 100);

                /* Verify firmware checksum */
                if (u32_fw_crc != u32_calc_crc)
                {
                    LOGE ("Firmware checksum validation failed");
                    OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Firmware checksum validation failed");
                    s8_result = OTAMN_ERR;
                }
            }
            else
            {
                LOGE ("Connection closed");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Connection closed");
                s8_result = OTAMN_ERR;
            }
            break;
        }

        /* If this is the first data chunk, obtain and validate firmware descriptor */
        if (u32_done_size == 0)
        {
            if (s32_data_len < FWUSLV_DESC_OFFSET + sizeof (FWUSLV_desc_t))
            {
                LOGE ("Failed to get firmware descriptor");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to get firmware descriptor");
                s8_result = OTAMN_ERR;
                break;
            }

            /* Firmware descriptor */
            FWUSLV_desc_t * pstru_desc = (FWUSLV_desc_t *)&pu8_chunk_data [FWUSLV_DESC_OFFSET];

            /* Validate the descriptor */
            if (s8_FWUSLV_Validate_Firmware_Info (pstru_desc) != FWUSLV_OK)
            {
                LOGE ("Invalid firmware descriptor");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Invalid firmware descriptor");
                s8_result = OTAMN_ERR;
                break;
            }

            /* If the firmware to be updated is application firmware, check its version */
            if (pstru_config->b_check_newer && (pstru_desc->u8_fw_type == FWUSLV_TYPE_APP))
            {
                uint8_t u8_major;
                uint8_t u8_minor;
                uint8_t u8_patch;
                if (s8_FWUSLV_Get_App_Version (&u8_major, &u8_minor, &u8_patch) == FWUSLV_OK)
                {
                    uint32_t u32_current_rev = ((uint32_t)u8_major << 16) |
                                               ((uint32_t)u8_minor <<  8) |
                                               ((uint32_t)u8_patch <<  0);
                    uint32_t u32_new_rev = ((uint32_t)pstru_desc->u8_major_rev << 16) |
                                           ((uint32_t)pstru_desc->u8_minor_rev <<  8) |
                                           ((uint32_t)pstru_desc->u8_patch_rev <<  0);
                    if (u32_new_rev <= u32_current_rev)
                    {
                        LOGW ("The new firmware is NOT newer than the current running firmware");
                        OTAMN_NOTIFY_STATUS_MQTT (false, "Error: The new firmware is NOT newer than the current firmware");
                        s8_result = OTAMN_IGNORED;
                        break;
                    }
                }
            }

            /* Store firmware checksum */
            u32_fw_crc = pstru_desc->u32_crc;

            /*
            ** Calculate firmware checksum of the first data chunk (skip CRC field itself)
            ** Note that crc32_le() has a `~` at the beginning and the end of the function
            */
            uint32_t u32_crc_offset = (uint32_t)(&pstru_desc->u32_crc) - (uint32_t)pu8_chunk_data;
            u32_calc_crc = crc32_le (0x00000000, pu8_chunk_data, u32_crc_offset);
            u32_crc_offset += 4;
            u32_calc_crc = crc32_le (u32_calc_crc, &pu8_chunk_data[u32_crc_offset], s32_data_len - u32_crc_offset);

            /* Erase the OTA partitition. Note that the size to erase must be divisible by 4 kilobytes. */
            if (esp_partition_erase_range (px_buf_part, 0, (s32_total_size + 0xFFF) & 0xFFFFF000) != ESP_OK)
            {
                LOGE ("Failed to erase the OTA buffer partition");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to erase the OTA buffer partition");
                s8_result = OTAMN_ERR;
                break;
            }
        }
        else
        {
            /* Calculate firmware checksum */
            u32_calc_crc = crc32_le (u32_calc_crc, pu8_chunk_data, s32_data_len);
        }

        /* Notify download progress */
        uint8_t u8_new_percents = u32_done_size * 100 / s32_total_size;
        if ((u32_done_size == 0) || (u8_new_percents != u8_percents))
        {
            u8_percents = u8_new_percents;
            LOGI ("Downloading slave firmware... %d%%", u8_percents);
            OTAMN_NOTIFY_DOWNLOAD_PROGRESS_MQTT (u8_percents);
            v_OTAMN_Notify_Progress_Gui (OTAMN_SLAVE_FW, OTAMN_STATE_DOWNLOAD, u8_percents);
        }

        /* Program the firmware data chunk onto OTA buffer partition */
        if (esp_partition_write (px_buf_part, u32_done_size, pu8_chunk_data, s32_data_len) != ESP_OK)
        {
            LOGE ("Failed to program firmware data chunk at offset %d", u32_done_size);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to program firmware data chunk");
            s8_result = OTAMN_ERR;
            break;
        }

        /* This data chunk has been flashed successfully */
        u32_done_size += s32_data_len;

        /* Check if user want to cancel OTA update process */
        if (g_b_cancelled)
        {
            LOGW ("Firmware update process has been cancelled");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Firmware update process is cancelled");
            s8_result = OTAMN_CANCELLED;
            break;
        }
    }

    /* Cleanup */
    if (pu8_chunk_data != NULL)
    {
        free (pu8_chunk_data);
    }
    if (b_http_open)
    {
        esp_http_client_close (x_https_client);
    }
    if (x_https_client != NULL)
    {
        esp_http_client_cleanup (x_https_client);
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends slave firmware to slave board for installation
**
** @param [in]
**      pstru_config: OTA configuration
**
** @return
**      @arg    OTAMN_OK
**      @arg    OTAMN_ERR
**      @arg    OTAMN_CANCELLED
**      @arg    OTAMN_IGNORED
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_OTAMN_Install_Slave_Firmware (OTAMN_config_t * pstru_config)
{
    int8_t                  s8_result       = OTAMN_OK;
    const esp_partition_t * px_buf_part     = NULL;
    FWUSLV_result_t         enm_result_code = FWUSLV_RESULT_OK;
    uint8_t                 u8_percents     = 0;
    FWUSLV_desc_t           stru_desc;

    /* Request slave board to enter Bootloader mode */
    if (s8_FWUSLV_Enter_Bootloader () != FWUSLV_OK)
    {
        LOGE ("Slave board failed to enter Bootloader mode");
        OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Slave board failed to enter Bootloader mode");
        s8_result = OTAMN_ERR;
    }

    /* Get the OTA partition */
    if (s8_result == OTAMN_OK)
    {
        px_buf_part = esp_ota_get_next_update_partition (NULL);
        if (px_buf_part == NULL)
        {
            LOGE ("Failed to access OTA partition");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to access OTA partition");
            s8_result = OTAMN_ERR;
        }
    }

    /* Read firmware descriptor of slave firmware from OTA partition */
    if (s8_result == OTAMN_OK)
    {
        if (esp_partition_read (px_buf_part, FWUSLV_DESC_OFFSET, &stru_desc, sizeof (stru_desc)) != ESP_OK)
        {
            LOGE ("Failed to read firmware descriptor from OTA partition");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to read firmware descriptor from OTA partition");
            s8_result = OTAMN_ERR;
        }
    }

    /* Prepare slave board for firmware update */
    if (s8_result == OTAMN_OK)
    {
        if (s8_FWUSLV_Prepare_Update (&stru_desc, &enm_result_code) == FWUSLV_OK)
        {
            if ((enm_result_code == FWUSLV_RESULT_WARN_FW_OLDER_VER) ||
                (enm_result_code == FWUSLV_RESULT_WARN_FW_SAME_VER) ||
                (enm_result_code == FWUSLV_RESULT_WARN_FW_ALREADY_EXIST))
            {
                LOGW ("The new firmware is NOT newer than the current running firmware");
                if (pstru_config->b_check_newer)
                {
                    OTAMN_NOTIFY_STATUS_MQTT (false, "Error: The new firmware is NOT newer than the current firmware");
                    s8_result = OTAMN_IGNORED;
                }
            }
            else if (enm_result_code == FWUSLV_RESULT_WARN_FW_VAR_MISMATCH)
            {
                LOGW ("Variant ID of the new firmware does not match with that of current running firmware");
            }
        }
        else
        {
            if (enm_result_code == FWUSLV_RESULT_ERR_FW_NOT_COMPATIBLE)
            {
                LOGE ("Not a firmware for Slave board");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Not a firmware for Slave board");
            }
            else if (enm_result_code == FWUSLV_RESULT_ERR_FW_SIZE_TOO_BIG)
            {
                LOGE ("Firmware size is too big");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Firmware size is too big");
            }
            else
            {
                LOGE ("Failed to prepare firmware update process");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to prepare firmware update process");
            }
            s8_result = OTAMN_ERR;
        }
    }

    /* Start firmware update process on slave board */
    if (s8_result == OTAMN_OK)
    {
        if (s8_FWUSLV_Start_Update (&enm_result_code) != FWUSLV_OK)
        {
            LOGE ("Failed to start slave firmware update process");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to start slave firmware update process");
            s8_result = OTAMN_ERR;
        }
    }

    /* Flash firmware data from OTA buffer onto slave board */
    if (s8_result == OTAMN_OK)
    {
        uint8_t * pu8_firmware = malloc (OTAMN_SLAVE_FW_CHUNK_SIZE);
        if (pu8_firmware == NULL)
        {
            LOGE ("Failed to allocate buffer for firmware flashing");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to allocate buffer for firmware flashing");
            s8_result = OTAMN_ERR;
        }
        else
        {
            uint32_t u32_num_flashed = 0;
            while ((u32_num_flashed < stru_desc.u32_size) && (s8_result == OTAMN_OK))
            {
                /* Get firmware data chunk from OTA buffer */
                uint16_t u16_chunk_len = (stru_desc.u32_size - u32_num_flashed > OTAMN_SLAVE_FW_CHUNK_SIZE) ?
                                          OTAMN_SLAVE_FW_CHUNK_SIZE : stru_desc.u32_size - u32_num_flashed;
                if (esp_partition_read (px_buf_part, u32_num_flashed, pu8_firmware, u16_chunk_len) != ESP_OK)
                {
                    LOGE ("Failed to read firmware data from OTA partition");
                    OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to read firmware data from OTA partition");
                    s8_result = OTAMN_ERR;
                    break;
                }

                /* Program firmware data onto slave board */
                FWUSLV_data_chunk_t stru_data_chunk =
                {
                    .u32_offset     = u32_num_flashed,
                    .u16_data_len   = u16_chunk_len,
                    .pu8_firmware   = pu8_firmware,
                };
                if (s8_FWUSLV_Program_Firmware (&stru_data_chunk, &enm_result_code) != FWUSLV_OK)
                {
                    LOGE ("Failed to program firmware data onto slave board");
                    OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to program firmware data onto slave board");
                    s8_result = OTAMN_ERR;
                    break;
                }

                /* Notify installation progress */
                uint8_t u8_new_percents = u32_num_flashed * 100 / stru_desc.u32_size;
                if ((u32_num_flashed == 0) || (u8_new_percents != u8_percents))
                {
                    u8_percents = u8_new_percents;
                    LOGI ("Installing slave firmware... %d%%", u8_percents);
                    OTAMN_NOTIFY_INSTALL_PROGRESS_MQTT (u8_percents);
                    v_OTAMN_Notify_Progress_Gui (OTAMN_SLAVE_FW, OTAMN_STATE_INSTALL, u8_percents);
                }

                /* This data chunk has been flashed successfully */
                u32_num_flashed += u16_chunk_len;

                /* Check if user want to cancel OTA update process */
                if (g_b_cancelled)
                {
                    LOGW ("Firmware update process has been cancelled");
                    OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Firmware update process is cancelled");
                    s8_result = OTAMN_CANCELLED;
                    break;
                }
            }

            /* Cleanup */
            free (pu8_firmware);
        }
    }

    /* Finalize slave firmware update process */
    if (s8_result == OTAMN_OK)
    {
        if (s8_FWUSLV_Finalize_Update (true, &enm_result_code) == FWUSLV_OK)
        {
            LOGI ("New firmware for slave board has been installed successfully.");
            OTAMN_NOTIFY_INSTALL_PROGRESS_MQTT (100);
            v_OTAMN_Notify_Progress_Gui (OTAMN_SLAVE_FW, OTAMN_STATE_INSTALL, 100);

            vTaskDelay (pdMS_TO_TICKS (100));
            OTAMN_NOTIFY_STATUS_MQTT (true, NULL);
        }
        else
        {
            if (enm_result_code == FWUSLV_RESULT_ERR_VALIDATION_FAILED)
            {
                LOGE ("Firmware validation failed");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Firmware validation failed");
            }
            else
            {
                LOGE ("Failed to finalize firmware update process");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to finalize firmware update process");
            }
            s8_result = OTAMN_ERR;
        }
    }
    else
    {
        /* Cancel firmware update */
        s8_FWUSLV_Finalize_Update (false, &enm_result_code);
    }

    /* Request slave board to exit Bootloader mode */
    if (s8_FWUSLV_Exit_Bootloader () != FWUSLV_OK)
    {
        s8_result = OTAMN_ERR;
        LOGE ("Slave board failed to exit Bootloader mode");
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Task performing OTA update for a file in filesystem of Master board
**
** @param [in]
**      pv_param: Parameter passed into the task
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_OTAMN_Update_Master_File_Task (void * pv_param)
{
    /* Validation */
    ASSERT_PARAM (g_b_busy && (pv_param != NULL));
    LOGI ("OTA update for file in Master board starts");

    /* OTA configuration */
    OTAMN_config_t * pstru_config = (OTAMN_config_t *)pv_param;

    /* Perform the update, retry if update fails */
    int8_t s8_result;
    for (uint8_t u8_retry = 0; u8_retry < 3; u8_retry++)
    {
        if (u8_retry != 0)
        {
            LOGE ("OTA update failed. Retrying %d...", u8_retry);
            vTaskDelay (pdMS_TO_TICKS (1000));
        }

        s8_result = s8_OTAMN_Update_Master_File (pstru_config, g_stri_ca_cert);
        if (s8_result != OTAMN_ERR)
        {
            break;
        }
    }

    /* Check result */
    if (s8_result == OTAMN_OK)
    {
        /* Hide progress from GUI */
        v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FILE, OTAMN_STATE_RESTART, 0xFF);

        /* Notify user */
        GUI_notify_t stru_notify =
        {
            .enm_type       = GUI_MSG_INFO,
            .pstri_brief    = "OTA data update",
            .pstri_detail   = "A file on filesystem of master board has been updated successfully.",
            .u32_wait_time  = 0
        };
        s8_GUI_Notify (&stru_notify);
    }
    else if (s8_result == OTAMN_CANCELLED)
    {
        /* Hide progress from GUI */
        v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FILE, OTAMN_STATE_RESTART, 0xFF);

        /* Notify user */
        GUI_notify_t stru_notify =
        {
            .enm_type       = GUI_MSG_WARNING,
            .pstri_brief    = "OTA data update",
            .pstri_detail   = "OTA data update of master board has been cancelled.",
            .u32_wait_time  = 0
        };
        s8_GUI_Notify (&stru_notify);
    }
    else if (s8_result == OTAMN_IGNORED)
    {
        /* Do nothing */
        LOGI ("Ignored the OTA update of master board's file");
    }
    else
    {
        /* Hide progress from GUI */
        v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FILE, OTAMN_STATE_RESTART, 0xFF);

        /* Notify user */
        GUI_notify_t stru_notify =
        {
            .enm_type       = GUI_MSG_ERROR,
            .pstri_brief    = "OTA data update",
            .pstri_detail   = "Failed to update data of master board.",
            .u32_wait_time  = 0
        };
        s8_GUI_Notify (&stru_notify);
    }

    /* Done */
    v_OTAMN_Cleanup (pstru_config);
    vTaskDelete (NULL);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Downloads a file from the corresponding HTTPs server and puts it onto the root folder of Master board's
**      filesystem, overwrites existing file if any.
**
** @param [in]
**      pstru_config: OTA configuration
**
** @param [in]
**      pstri_ca_cert: Certificate (NULL-terminated string) of the HTTPs server storing the file
**
** @return
**      @arg    OTAMN_OK
**      @arg    OTAMN_ERR
**      @arg    OTAMN_CANCELLED
**      @arg    OTAMN_IGNORED
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_OTAMN_Update_Master_File (OTAMN_config_t * pstru_config, const char * pstri_ca_cert)
{
    esp_http_client_handle_t    x_https_client  = NULL;
    int8_t                      s8_result       = OTAMN_OK;
    bool                        b_http_open     = false;
    uint8_t *                   pu8_chunk_data  = NULL;
    int32_t                     s32_total_size  = 0;
    uint32_t                    u32_done_size   = 0;
    uint8_t                     u8_percents     = 0;
    bool                        b_tmp_file_open = false;
    lfs2_file_t                 x_tmp_file;

    /* Configuration of HTTP client */
    esp_http_client_config_t stru_http_client_cfg =
    {
        .url                = pstru_config->pstri_url,
        .cert_pem           = pstri_ca_cert,
        .timeout_ms         = 10000,
        .keep_alive_enable  = true,
        .buffer_size        = 2048,
        .buffer_size_tx     = 1024,
    };

    /* Check if file name part exists in installation path */
    if ((pstru_config->pstri_inst_dir == NULL) ||
        (pstri_OTAMN_Get_File_Name (pstru_config->pstri_inst_dir) == NULL))
    {
        LOGE ("Failed to extract file name");
        OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to extract file name from installation path");
        s8_result = OTAMN_ERR;
    }

    /* Start HTTP session. Note that esp_http_client_cleanup() must be called when HTTP session is done */
    if (s8_result == OTAMN_OK)
    {
        x_https_client = esp_http_client_init (&stru_http_client_cfg);
        if (x_https_client == NULL)
        {
            LOGE ("Failed to initialise HTTPs connection");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to initialise HTTPs connection");
            s8_result = OTAMN_ERR;
        }
    }

    /* Open the HTTP connection for reading */
    if (s8_result == OTAMN_OK)
    {
        esp_err_t x_err = esp_http_client_open (x_https_client, 0);
        if (x_err == ESP_OK)
        {
            b_http_open = true;
        }
        else
        {
            LOGE ("Failed to open HTTPs connection: %s", esp_err_to_name (x_err));
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to open HTTPs connection");
            s8_result = OTAMN_ERR;
        }
    }

    /* Read from HTTP stream and process all response headers to get size of the file to download */
    if (s8_result == OTAMN_OK)
    {
        s32_total_size = esp_http_client_fetch_headers (x_https_client);
        if (s32_total_size < 0)
        {
            LOGE ("Failed to process HTTPs response headers");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to process HTTPs response headers");
            s8_result = OTAMN_ERR;
        }
        else if (s32_total_size == 0)
        {
            LOGE ("Failed to reach the file to download");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to reach the file to download");
            s8_result = OTAMN_ERR;
        }
        else
        {
            LOGI ("Download file size = %d bytes", s32_total_size);
        }
    }

    /* Check if size of the file to download can be fit inside the filesystem */
    if (s8_result == OTAMN_OK)
    {
        uint32_t u32_free_space;
        if (s8_MQTTMN_Get_Storage_Space (NULL, &u32_free_space) != MQTTMN_OK)
        {
            LOGE ("Failed to get storage space");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to get storage space");
            s8_result = OTAMN_ERR;
        }
        else if (u32_free_space < s32_total_size)
        {
            LOGE ("Size of the file to download is greater than the remaining storage (%d bytes)", u32_free_space);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: The remaining storage is not sufficient for the file to download");
            s8_result = OTAMN_ERR;
        }
    }

    /* Open a temporary file to store the data */
    if (s8_result == OTAMN_OK)
    {
        if (lfs2_file_open (g_px_lfs2, &x_tmp_file, OTAMN_TEMP_FILE, LFS2_O_WRONLY | LFS2_O_CREAT | LFS2_O_TRUNC) < 0)
        {
            LOGE ("Failed to open file " OTAMN_TEMP_FILE " for writing");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to open file " OTAMN_TEMP_FILE " for writing");
            s8_result = OTAMN_ERR;
        }
        else
        {
            b_tmp_file_open = true;
        }
    }

    /* Allocate buffer for data chunk downloaded */
    if (s8_result == OTAMN_OK)
    {
        pu8_chunk_data = malloc (OTAMN_DOWNLOAD_CHUNK_SIZE);
        if (pu8_chunk_data == NULL)
        {
            LOGE ("Failed to allocate memory of %d bytes for download data chunk", OTAMN_DOWNLOAD_CHUNK_SIZE);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Not enough memory");
            s8_result = OTAMN_ERR;
        }
    }

    /* Download the file from the HTTPs server */
    while (s8_result == OTAMN_OK)
    {
        /* Get one data chunk */
        int32_t s32_data_len = esp_http_client_read (x_https_client, (char *)pu8_chunk_data, OTAMN_DOWNLOAD_CHUNK_SIZE);
        if (s32_data_len < 0)
        {
            LOGE ("Failed to download file data chunk (offset %d bytes) from the server", u32_done_size);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to download file data chunk from the server");
            s8_result = OTAMN_ERR;
            break;
        }
        else if (s32_data_len == 0)
        {
            if (esp_http_client_is_complete_data_received (x_https_client))
            {
                LOGI ("Downloading completed");
                OTAMN_NOTIFY_DOWNLOAD_PROGRESS_MQTT (100);
                v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FILE, OTAMN_STATE_DOWNLOAD, 100);
                lfs2_file_close (g_px_lfs2, &x_tmp_file);
                b_tmp_file_open = false;

                /* Create destination folder(s) */
                v_OTAMN_Create_Folder (pstru_config->pstri_inst_dir);

                /* Move and rename the temporary file into the desired location and name */
                if (lfs2_rename (g_px_lfs2, OTAMN_TEMP_FILE, pstru_config->pstri_inst_dir) < 0)
                {
                    lfs2_remove (g_px_lfs2, OTAMN_TEMP_FILE);
                    LOGE ("Failed to rename the downloaded file");
                    OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to rename the downloaded file");
                    s8_result = OTAMN_ERR;
                }
                else
                {
                    LOGI ("File %s has been installed successfully.", pstru_config->pstri_inst_dir);
                    OTAMN_NOTIFY_INSTALL_PROGRESS_MQTT (100);
                    v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FILE, OTAMN_STATE_INSTALL, 100);

                    vTaskDelay (pdMS_TO_TICKS (100));
                    OTAMN_NOTIFY_STATUS_MQTT (true, NULL);
                }
            }
            else
            {
                LOGE ("Connection closed");
                OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Connection closed");
                s8_result = OTAMN_ERR;
            }
            break;
        }

        /* Notify download progress */
        uint8_t u8_new_percents = u32_done_size * 100 / s32_total_size;
        if ((u32_done_size == 0) || (u8_new_percents != u8_percents))
        {
            u8_percents = u8_new_percents;
            LOGI ("Downloading file... %d%%", u8_percents);
            OTAMN_NOTIFY_DOWNLOAD_PROGRESS_MQTT (u8_percents);
            v_OTAMN_Notify_Progress_Gui (OTAMN_MASTER_FILE, OTAMN_STATE_DOWNLOAD, u8_percents);
        }

        /* Store the file data chunk into the temporary file */
        if (lfs2_file_write (g_px_lfs2, &x_tmp_file, pu8_chunk_data, s32_data_len) != s32_data_len)
        {
            LOGE ("Failed to program file data chunk at offset %d", u32_done_size);
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: Failed to program file data chunk");
            s8_result = OTAMN_ERR;
            break;
        }

        /* This data chunk has been flashed successfully */
        u32_done_size += s32_data_len;

        /* Check if user want to cancel OTA update process */
        if (g_b_cancelled)
        {
            LOGW ("File update process has been cancelled");
            OTAMN_NOTIFY_STATUS_MQTT (false, "Error: File update process is cancelled");
            s8_result = OTAMN_CANCELLED;
            break;
        }
    }

    /* Cleanup */
    if (b_tmp_file_open)
    {
        lfs2_file_close (g_px_lfs2, &x_tmp_file);
        lfs2_remove (g_px_lfs2, OTAMN_TEMP_FILE);
    }
    if (pu8_chunk_data != NULL)
    {
        free (pu8_chunk_data);
    }
    if (b_http_open)
    {
        esp_http_client_close (x_https_client);
    }
    if (x_https_client != NULL)
    {
        esp_http_client_cleanup (x_https_client);
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Creates a folder and all intermediate folders given an absolute path
**
** @details
**      For example, if path is:
**      + "/a/b/c/d"  : folder "/a", "/a/b", and "/a/b/c" shall be created (if not existing), "d" is regarded as a file
**      + "/a/b/c/d/" : folder "/a", "/a/b", "/a/b/c", and "/a/b/c/d" shall be created (if not existing)
**      + "a/b/c/d/"  : folder "/a", "/a/b", "/a/b/c", and "/a/b/c/d" shall be created (if not existing)
**
** @param [in]
**      pstri_path: Path of the folder to create
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_OTAMN_Create_Folder (const char * pstri_path)
{
    /* Allocate temporary memory for path string */
    uint16_t u16_path_len = strlen (pstri_path) + 1;
    char * pstri_path_tmp = malloc (u16_path_len);
    if (pstri_path_tmp == NULL)
    {
        LOGE ("Failed to allocate memory for path string");
        return;
    }
    memcpy (pstri_path_tmp, pstri_path, u16_path_len);

    /* Create folder(s) */
    for (uint16_t u16_idx = 0; u16_idx < u16_path_len; u16_idx++)
    {
        if ((pstri_path_tmp[u16_idx] == '/') && (u16_idx != 0))
        {
            pstri_path_tmp[u16_idx] = '\0';
            lfs2_mkdir (g_px_lfs2, pstri_path_tmp);
            pstri_path_tmp[u16_idx] = '/';
        }
    }

    /* Cleanup */
    free (pstri_path_tmp);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Extracts the file name part from a path
**
** @details
**      File name is the part after the last '/' in the path. For example, if path is:
**      + "/a/b/c/d"  : returns pointer to "d"
**      + "/a/b/c/d/" : returns NULL
**
** @param [in]
**      pstri_path: Path to extract file name
**
** @return
**      Pointer to the file name part in pstri_path
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static const char * pstri_OTAMN_Get_File_Name (const char * pstri_path)
{
    const char * pstri_file_name = NULL;
    while (*pstri_path != '\0')
    {
        if (*pstri_path == '/')
        {
            pstri_path++;
            pstri_file_name = pstri_path;
        }
        else
        {
            pstri_path++;
        }
    }
    if (pstri_file_name[0] == '\0')
    {
        pstri_file_name = NULL;
    }

    return pstri_file_name;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Perform cleaning up when OTA update is done
**
** @param [in]
**      pstru_config: OTA configuration
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_OTAMN_Cleanup (OTAMN_config_t * pstru_config)
{
    /* Free the allocated buffers */
    if (pstru_config->pstri_url != NULL)
    {
        free (pstru_config->pstri_url);
        pstru_config->pstri_url = NULL;
    }
    if (pstru_config->pstri_inst_dir != NULL)
    {
        free (pstru_config->pstri_inst_dir);
        pstru_config->pstri_inst_dir = NULL;
    }

    /* OTA is done now */
    g_b_busy = false;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Displays update progress on GUI
**
** @param [in]
**      enm_target: Component being updated
**
** @param [in]
**      enm_state: Current OTA update state
**
** @param [in]
**      u8_percents: Percents already done
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_OTAMN_Notify_Progress_Gui (OTAMN_target_t enm_target, OTAMN_state_t enm_state, uint8_t u8_percents)
{
    static char stri_status [sizeof ("Downloading... 100%") + 10];
    static GUI_progress_t stru_progress =
    {
        .enm_type       = GUI_JOB_SYSTEM,
        .pstri_brief    = "OTA firmware update",
        .pstri_status   = stri_status,
        .s32_min        = 0,
        .s32_max        = 100,
    };

    /* Detailed description of the progress */
    if (enm_target == OTAMN_MASTER_FW)
    {
        stru_progress.pstri_detail = "Firmware of Master board is being updated over-the-air.\n\n"
                                     "Do NOT disconnect the power or restart Rotimatic until the update is done.";
    }
    else if (enm_target == OTAMN_SLAVE_FW)
    {
        stru_progress.pstri_detail = "Firmware of Slave board is being updated over-the-air.\n\n"
                                     "Do NOT disconnect the power or restart Rotimatic until the update is done.";
    }
    else
    {
        stru_progress.pstri_brief = "OTA data update";
        stru_progress.pstri_detail = "Data of Master board is being updated over-the-air.\n\n"
                                     "Do NOT disconnect the power or restart Rotimatic until the update is done.";
    }

    /* Status and percents of the progress */
    stru_progress.s32_progress = u8_percents;
    if (enm_state == OTAMN_STATE_DOWNLOAD)
    {
        sprintf (stri_status, "Downloading... %d%%", u8_percents);
    }
    else if (enm_state == OTAMN_STATE_INSTALL)
    {
        sprintf (stri_status, "Installing... %d%%", u8_percents);
    }
    else if (enm_state == OTAMN_STATE_RESTART)
    {
        sprintf (stri_status, "Restarting...");
    }

    /* Display the progress to GUI */
    s8_GUI_Progress (&stru_progress);
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
