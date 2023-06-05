/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : app_mqtt_mngr.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 May 24
**  @brief      : Implementation of App_Mqtt_Mngr module
**  @namespace  : MQTTMN
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Mqtt_Mngr
** @brief       Manages MQTT connection and handles messages received from MQTT network.
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "app_mqtt_mngr.h"              /* Public header of this module */
#include "srvc_mqtt.h"                  /* Use MQTT service */
#include "srvc_param.h"                 /* Use Parameter service */
#include "srvc_wifi.h"                  /* Use MAC address of Wifi interface */

#include <string.h>                     /* Use strlen(), strcmp(), etc. */
#include <stdio.h>                      /* Use snprintf(), sscanf(), etc. */
#include "cJSON.h"                      /* Use ESP-IDF's JSON component */
#include "esp_partition.h"              /* Access partition information */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/task.h"              /* Use FreeRTOS task */
#include "freertos/event_groups.h"      /* Use FreeRTOS event group */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Table of all request and post commands received from back-office node */
#define MQTTMN_RX_CMD_TABLE(X)                                                             \
/*---------------------------------------------------------------------------------------*/\
/*  Command                         Description                                          */\
/*---------------------------------------------------------------------------------------*/\
                                                                                           \
X(  scanPost                        /* Checks aliveness of the device */                  )\
X(  devResetPost                    /* Resets the device */                               )\
X(  webReplRunPost                  /* Starts WebREPL interface of the device */          )\
X(  otaUpdateCancelPost             /* Cancels an on-going OTA update process */          )\
                                                                                           \
X(  paramReadRequest                /* Reads value of non-volatile settings */            )\
X(  paramWriteRequest               /* Writes value of non-volatile settings */           )\
X(  fileListReadRequest             /* Gets list of all files in root directory */        )\
X(  fileUploadWriteRequest          /* Uploads a file to the device */                    )\
X(  fileDownloadReadRequest         /* Downloads an existing file from the device */      )\
X(  fileDeleteWriteRequest          /* Deletes an existing file of the device */          )\
X(  fileRunWriteRequest             /* Runs an existing Python script in the device */    )\
X(  otaUpdateWriteRequest           /* Triggers OTA update process */                     )\
                                                                                           \
/*---------------------------------------------------------------------------------------*/

/** @brief  Macro expanding an entry in request and post command table as structure initialization */
#define EXPAND_RX_TABLE_AS_STRUCT_INIT(COMMAND)                 \
    {                                                           \
        .pstri_command          = #COMMAND,                     \
        .b_is_request           = false,                        \
        .v_pfnc_cmd_handler     = v_MQTTMN_##COMMAND##_Handler, \
    },

/** @brief  Macro expanding an entry in request and post command table as prototype of commnad handler */
#define EXPAND_RX_TABLE_AS_HANDLER_PROTOTYPE(COMMAND)           \
    static void v_MQTTMN_##COMMAND##_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_command);

/** @brief  Index of request and post commands */
#define EXPAND_RX_TABLE_AS_CMD_IDX(COMMAND, ...)                COMMAND,
enum
{
    MQTTMN_RX_CMD_TABLE (EXPAND_RX_TABLE_AS_CMD_IDX)
    MQTTMN_NUM_RX_CMD
};

/** @brief  Number of concurrent communication sessions with back-office nodes */
#define NUM_COMM_SESSIONS                   5

/** @brief  Maximum length in bytes of a file */
#define MQTT_MAX_FILE_SIZE                  (256 * 1024)

/** @brief  ID of the CPU that App_Mqtt_Mngr task runs on */
#define MQTTMN_TASK_CPU_ID                  1

/** @brief  Stack size (in bytes) of App_Mqtt_Mngr task */
#define MQTTMN_TASK_STACK_SIZE              6144

/** @brief  Priority of App_Mqtt_Mngr task */
#define MQTTMN_TASK_PRIORITY                (tskIDLE_PRIORITY + 0)

/** @brief  Cycle in milliseconds of App_Mqtt_Mngr task */
#define MQTTMN_TASK_PERIOD_MS               50

/** @brief  Timeout in milliseconds to close an inactivity communication session */
#define SESSION_INACT_TIMEOUT               300000

/** @brief  FreeRTOS events */
enum
{
    MQTTMN_FILE_DOWNLOAD_STARTED_EVT        = BIT0,     //!< Back-office node starts to download a file
    MQTTMN_OTA_DOWNLOAD_PROGRESS_EVT        = BIT1,     //!< Send notify on OTA firmware download progress
    MQTTMN_OTA_INSTALL_PROGRESS_EVT         = BIT2,     //!< Send notify on OTA firmware install progress
    MQTTMN_OTA_OVERALL_STATUS_EVT           = BIT3,     //!< Send notify on overall status of OTA firmware update
};

/** @brief  Structure encapsulating a connection session with a back-office node */
typedef struct
{
    bool        b_active;                               //!< Indicates if the session is being used
    TickType_t  x_inact_timer;                          //!< Timer tracking session inactive time
    uint32_t    u32_master_node_id;                     //!< Master node ID
    char        stri_response_topic[96];                //!< MQTT topic to send response to the back-office node
    char        stri_data_topic[96];                    //!< MQTT topic to send data to the back-office node
    uint32_t    u32_request_eid;                        //!< Exchange ID of current request command
    uint32_t    u32_post_eid;                           //!< Exchange ID of current post command

} MQTTMN_session_t;

/** @brief  Structure encapsulating each command in request and post command table */
typedef struct
{
    /** @brief  Command string */
    const char * pstri_command;

    /** @brief  Whether this command is a request (or a post) */
    bool b_is_request;

    /** @brief  Pointer to command handler */
    void (*v_pfnc_cmd_handler) (MQTTMN_session_t * pstru_session, const cJSON * px_command);

} MQTTMN_rx_cmd_t;

/** @brief  Common JSON keys used in commands */
#define JSON_KEY_CMD                        "command"
#define JSON_KEY_EID                        "eid"

/** @brief  Types of statuses for statusNotify command */
#define NOTIFY_FILE_UPLOAD_STATUS           "fileUploadStatus"
#define NOTIFY_FILE_DOWNLOAD_STATUS         "fileDownloadStatus"
#define NOTIFY_OTA_DOWNLOAD_PROGRESS        "otaDownloadProgress"
#define NOTIFY_OTA_INSTALL_PROGRESS         "otaInstallProgress"
#define NOTIFY_OTA_UPDATE_STATUS            "otaUpdateStatus"

/** @brief  Values of common statuses for responses and statusNotify command */
#define STATUS_OK                           "ok"
#define STATUS_CANCELLED                    "cancelled"
#define STATUS_ERR                          "error"
#define STATUS_ERR_NOT_SUPPORTED            "errorNotSupported"
#define STATUS_ERR_INVALID_DATA             "errorInvalidData"
#define STATUS_ERR_BUSY                     "errorBusy"
#define STATUS_ERR_STATE_NOT_ALLOWED        "errorStateNotAllowed"
#define STATUS_ERR_INVALID_ACCESS           "errorInvalidAccess"

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

MQTTMN_RX_CMD_TABLE (EXPAND_RX_TABLE_AS_HANDLER_PROTOTYPE)

static void v_MQTTMN_Main_Task (void * pv_param);
static void v_MQTTMN_Event_Handler (MQTT_evt_data_t * pstru_evt_data);
static void v_MQTTMN_Process_Rx_Message (const char * pstri_topic, uint16_t u16_topic_len, const void * pv_data,
                                          uint32_t u32_data_len, uint32_t u32_offset, uint32_t u32_total_len);
static bool b_MQTTMN_Parse_Topic (const char * pstri_topic, uint16_t u16_topic_len,
                                  uint32_t * pu32_master_node_id, bool * pb_is_command);
static MQTTMN_session_t * pstru_MQTTMN_Get_Session (uint32_t u32_master_node_id);
static void v_MQTTMN_Process_Command (MQTTMN_session_t * pstru_session, const void * pv_data, uint32_t u32_len);
static void v_MQTTMN_Process_Data (MQTTMN_session_t * pstru_session, const void * pv_data,
                                   uint32_t u32_len, uint32_t u32_offset, uint32_t u32_total_len);
static void v_MQTTMN_Data2Hex (const uint8_t * pu8_data, uint8_t u8_len, char ** ppstri_hex);
static void v_MQTTMN_Hex2Data (const char * pstri_hex, uint8_t ** ppu8_data, uint8_t * pu8_len);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "App_Mqtt_Mngr";

/** @brief  Structure that will hold the TCB of the task being created */
static StaticTask_t g_x_task_buffer;

/** @brief  Buffer that the task being created will use as its stack */
static StackType_t g_x_task_stack [MQTTMN_TASK_STACK_SIZE];

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Instance of the associated MQTT client */
static MQTT_inst_t g_x_mqtt = NULL;

/** @brief  Indicate if the MQTT broker is being connected */
static bool g_b_mqtt_connected = false;

/** @brief  FreeRTOS event group */
static EventGroupHandle_t g_x_event_group;

/** @brief  Array of communication sessions with back-office nodes */
static MQTTMN_session_t g_astru_sessions[NUM_COMM_SESSIONS];

/** @brief  Group ID of this node */
static char * g_pstri_group_id = NULL;

/** @brief  Slave node ID of this node (will be constructed from the last 4 bytes of MAC address) */
static uint32_t g_u32_slave_node_id = 0;

/** @brief  Exchange ID for notify messages */
static uint32_t g_u32_notify_eid = 0;

/** @brief  Buffer storing uploading file path, if there is no file being uploaded, the buffer contains empty string */
static char g_stri_upload_file [MAX_FILE_PATH_LEN];

/** @brief  Buffer storing downloading file path, if there is no file being downloaded, the buffer contains empty string */
static char g_stri_download_file [MAX_FILE_PATH_LEN];

/** @brief  Array of all request and post commands */
static MQTTMN_rx_cmd_t g_astru_rx_commands [MQTTMN_NUM_RX_CMD] =
{
    MQTTMN_RX_CMD_TABLE (EXPAND_RX_TABLE_AS_STRUCT_INIT)
};

/** @brief  Context data for FreeRTOS events */
static struct
{
    uint8_t      u8_ota_download_percent;   //!< Download progress in percents (0 -> 100)
    uint8_t      u8_ota_install_percent;    //!< Install progress in percents (0 -> 100)
    bool         b_ota_ok;                  //!< Indicates if OTA firmware update has been done successfully for not
    const char * pstri_ota_error_desc;      //!< Description about OTA firmware update error

} g_stru_evt_context;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Response and notify commands */
#include "tx_messages.c"

/* Request and post command handlers */
#include "rx_messages.c"

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes App_Mqtt_Mngr module
**
** @note
**      To reduce time connecting to MQTT brokers, this function should be invoked after network connection has been
**      established
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MQTTMN_Init (void)
{
    /* Do nothing if this module has been initialized */
    if (g_b_initialized)
    {
        return MQTTMN_OK;
    }

    LOGD ("Initializing App_Mqtt_Mngr module");

    /* Ensure that LittleFS storage is ready */
    if (g_px_lfs2 == NULL)
    {
        LOGE ("LittleFS storage is not ready yet");
        return MQTTMN_ERR;
    }

    /* Display information of the LittleFS storage */
    uint32_t u32_total_space;
    uint32_t u32_free_space;
    if (s8_MQTTMN_Get_Storage_Space (&u32_total_space, &u32_free_space) != MQTTMN_OK)
    {
        LOGE ("Failed to get information of LittleFS storage");
        return MQTTMN_ERR;
    }
    LOGI ("LittleFS storage: total space = %d bytes, free space = %d bytes", u32_total_space, u32_free_space);

    /* Get instance of the MQTT client */
    g_x_mqtt = x_MQTT_Get_Inst (MQTT_ESP32_CLIENT);
    if (g_x_mqtt == NULL)
    {
        LOGE ("Failed to get MQTT instance");
        return MQTTMN_ERR;
    }

    /* Initialize communication sessions */
    for (uint8_t u8_idx = 0; u8_idx < NUM_COMM_SESSIONS; u8_idx++)
    {
        g_astru_sessions[u8_idx].b_active = false;
    }

    /* Determine message type of receive commands basing on their name */
    for (uint8_t u8_idx = 0; u8_idx < MQTTMN_NUM_RX_CMD; u8_idx++)
    {
        MQTTMN_rx_cmd_t * pstru_cmd = &g_astru_rx_commands[u8_idx];
        uint8_t u8_cmd_len = strlen (pstru_cmd->pstri_command);
        pstru_cmd->b_is_request = (strcmp ("Request", &pstru_cmd->pstri_command[u8_cmd_len - 7]) == 0);
    }

    /* Get group ID string that this node belongs to */
    if (s8_PARAM_Get_String (PARAM_MQTT_GROUP_ID, &g_pstri_group_id) != PARAM_OK)
    {
        LOGE ("Failed to read MQTT group ID");
        return MQTTMN_ERR;
    }

    /* Construct slave node ID from MAC address */
    uint8_t au8_mac[8];
    s8_WIFI_Get_Mac (au8_mac);
    g_u32_slave_node_id = ENDIAN_GET32_BE (&au8_mac[2]);
    LOGI ("ESP32 node ID = %08X", g_u32_slave_node_id);

    /* MQTT topic receiving unicast messages from back-office nodes */
    static char stri_rx_unicast_topic[64];
    snprintf (stri_rx_unicast_topic, sizeof (stri_rx_unicast_topic),
              "itor3/m2s/%s/%08X/#", g_pstri_group_id, g_u32_slave_node_id);
    v_MQTT_Set_Subscribe_Topic (g_x_mqtt, MQTT_M2S_UNICAST, stri_rx_unicast_topic);

    /* MQTT topic receiving multicast (group broadcast) messages from back-office nodes */
    static char stri_rx_multicast_topic[64];
    snprintf (stri_rx_multicast_topic, sizeof (stri_rx_multicast_topic),
              "itor3/m2s/%s/_broadcast_/#", g_pstri_group_id);
    v_MQTT_Set_Subscribe_Topic (g_x_mqtt, MQTT_M2S_MULTICAST, stri_rx_multicast_topic);

    /* MQTT topic for sending notify commands */
    static char stri_notify_topic[64];
    snprintf (stri_notify_topic, sizeof (stri_notify_topic),
              "itor3/s2m/%s/%08X/notify", g_pstri_group_id, g_u32_slave_node_id);
    v_MQTT_Set_Publish_Topic (g_x_mqtt, MQTT_S2M_NOTIFY, stri_notify_topic);

    /* Listen to MQTT events */
    v_MQTT_Register_Callback (g_x_mqtt, v_MQTTMN_Event_Handler, NULL);

    /* Start the MQTT client */
    if (enm_MQTT_Start_Inst (g_x_mqtt) != MQTT_OK)
    {
        LOGE ("Failed to start the MQTT client");
        return MQTTMN_ERR;
    }

    /* Create FreeRTOS event group */
    g_x_event_group = xEventGroupCreate ();

    /* Create task running this module */
    xTaskCreateStaticPinnedToCore ( v_MQTTMN_Main_Task,         /* Function that implements the task */
                                    "App_Mqtt_Mngr",            /* Text name for the task */
                                    MQTTMN_TASK_STACK_SIZE,     /* Stack size in bytes, not words */
                                    NULL,                       /* Parameter passed into the task */
                                    MQTTMN_TASK_PRIORITY,       /* Priority at which the task is created */
                                    g_x_task_stack,             /* Array to use as the task's stack */
                                    &g_x_task_buffer,           /* Variable to hold the task's data structure */
                                    MQTTMN_TASK_CPU_ID);        /* ID of the CPU that App_Gui_Mngr task runs on */

    /* Done */
    LOGD ("Initialization of App_Mqtt_Mngr module is done");
    g_b_initialized = true;
    return MQTTMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a notify message via MQTT interface about firmware download progress of OTA firmware update
**
** @param [in]
**      u8_percents: Firmware download progress in percents (0->100)
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MQTTMN_Notify_Ota_Download_Progress (uint8_t u8_percents)
{
    ASSERT_PARAM (u8_percents <= 100);

    /* The notify will be sent by App_Mqtt_Mngr task */
    g_stru_evt_context.u8_ota_download_percent = u8_percents;
    xEventGroupSetBits (g_x_event_group, MQTTMN_OTA_DOWNLOAD_PROGRESS_EVT);
    vTaskDelay (pdMS_TO_TICKS (10));

    return MQTTMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a notify message via MQTT interface about firmware install progress of OTA firmware update
**
** @param [in]
**      u8_percents: Firmware install progress in percents (0->100)
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MQTTMN_Notify_Ota_Install_Progress (uint8_t u8_percents)
{
    ASSERT_PARAM (u8_percents <= 100);

    /* The notify will be sent by App_Mqtt_Mngr task */
    g_stru_evt_context.u8_ota_install_percent = u8_percents;
    xEventGroupSetBits (g_x_event_group, MQTTMN_OTA_INSTALL_PROGRESS_EVT);
    vTaskDelay (pdMS_TO_TICKS (10));

    return MQTTMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a notify message via MQTT interface about overall status of OTA firmware update when done
**
** @param [in]
**      b_ok: Indicates if OTA firmware update has been done successfully for not
**
** @param [in]
**      pstri_error_desc: Description about error in case b_ok is false
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MQTTMN_Notify_Ota_Status (bool b_ok, const char * pstri_error_desc)
{
    /* The notify will be sent by App_Mqtt_Mngr task */
    g_stru_evt_context.b_ota_ok = b_ok;
    g_stru_evt_context.pstri_ota_error_desc = (pstri_error_desc != NULL) ? pstri_error_desc : "";
    xEventGroupSetBits (g_x_event_group, MQTTMN_OTA_OVERALL_STATUS_EVT);
    vTaskDelay (pdMS_TO_TICKS (10));

    return MQTTMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets total size and free space of the LittleFS storage
**
** @param [out]
**      pu32_total_space: Total space in bytes, can be NULL
**
** @param [out]
**      pu32_free_space: Free space in bytes, can be NULL
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MQTTMN_Get_Storage_Space (uint32_t * pu32_total_space, uint32_t * pu32_free_space)
{
    /* Get size of the partition containing LittleFS filesystem */
    const esp_partition_t * pstru_part = esp_partition_find_first (ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "vfs");
    if (pstru_part == NULL)
    {
        LOGE ("Failed to get size LittleFS partition information");
        return MQTTMN_ERR;
    }

    /* Get used space */
    lfs2_ssize_t x_used_blocks = lfs2_fs_size (g_px_lfs2);
    if (x_used_blocks < 0)
    {
        LOGE ("Failed to get number of blocks used from LittleFS storage");
        return MQTTMN_ERR;
    }

    /* Get result */
    if (pu32_total_space != NULL)
    {
        *pu32_total_space = pstru_part->size;
    }
    if (pu32_free_space != NULL)
    {
        *pu32_free_space = pstru_part->size - (x_used_blocks * g_px_lfs2->cfg->block_size);
    }

    return MQTTMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Task running App_Mqtt_Mngr module
**
** @param [in]
**      pv_param: Parameter passed into the task
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_Main_Task (void * pv_param)
{
    LOGD ("App_Mqtt_Mngr task started");

    /* Endless loop of the task */
    while (true)
    {
        /* Waiting for task tick or a FreeRTOS event occurs */
        EventBits_t x_event_bits =
            xEventGroupWaitBits (g_x_event_group,       /* The event group to test the bits */
                                 MQTTMN_FILE_DOWNLOAD_STARTED_EVT |
                                 MQTTMN_OTA_DOWNLOAD_PROGRESS_EVT |
                                 MQTTMN_OTA_INSTALL_PROGRESS_EVT  |
                                 MQTTMN_OTA_OVERALL_STATUS_EVT,
                                 pdTRUE,                /* Whether the tested bits are automatically cleared on exit */
                                 pdFALSE,               /* Whether to wait for all test bits to be set */
                                 pdMS_TO_TICKS (MQTTMN_TASK_PERIOD_MS));

        /* If a file needs to be sent to back-office node */
        if (x_event_bits & MQTTMN_FILE_DOWNLOAD_STARTED_EVT)
        {
            /* Publish content of the downloading file */
            s8_MQTTMN_Publish_Downloading_File ();
        }

        /* If a notify about firmware download progress of OTA firmware update needs to be sent */
        if (x_event_bits & MQTTMN_OTA_DOWNLOAD_PROGRESS_EVT)
        {
            /* Percent string */
            char stri_percent[] = "100";
            snprintf (stri_percent, sizeof (stri_percent), "%d", g_stru_evt_context.u8_ota_download_percent);

            /* Publish the notify message */
            s8_MQTTMN_Send_statusNotify (NOTIFY_OTA_DOWNLOAD_PROGRESS, stri_percent, "");
        }

        /* If a notify about firmware install progress of OTA firmware update needs to be sent */
        if (x_event_bits & MQTTMN_OTA_INSTALL_PROGRESS_EVT)
        {
            /* Percent string */
            char stri_percent[] = "100";
            snprintf (stri_percent, sizeof (stri_percent), "%d", g_stru_evt_context.u8_ota_install_percent);

            /* Publish the notify message */
            s8_MQTTMN_Send_statusNotify (NOTIFY_OTA_INSTALL_PROGRESS, stri_percent, "");
       }

        /* If a notify about overall status of OTA firmware update needs to be sent */
        if (x_event_bits & MQTTMN_OTA_OVERALL_STATUS_EVT)
        {
            if (g_stru_evt_context.b_ota_ok)
            {
                s8_MQTTMN_Send_statusNotify (NOTIFY_OTA_UPDATE_STATUS, STATUS_OK, "");
            }
            else
            {
                s8_MQTTMN_Send_statusNotify (NOTIFY_OTA_UPDATE_STATUS, STATUS_ERR,
                                             g_stru_evt_context.pstri_ota_error_desc);
            }
        }

        /* Close sessions that is inactive for too long */
        for (uint8_t u8_idx = 0; u8_idx < NUM_COMM_SESSIONS; u8_idx++)
        {
            MQTTMN_session_t * pstru_session = &g_astru_sessions[u8_idx];
            if (pstru_session->b_active &&
                (TIMER_ELAPSED (pstru_session->x_inact_timer) >= pdMS_TO_TICKS (SESSION_INACT_TIMEOUT)))
            {
                pstru_session->b_active = false;
            }
        }

        /* Display remaining stack space every 30s */
        // PRINT_STACK_USAGE (30000);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of events from MQTT service
**
** @param [in]
**      pstru_evt_data: Pointer to the data associated with the event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_Event_Handler (MQTT_evt_data_t * pstru_evt_data)
{
    /* Process MQTT events */
    switch (pstru_evt_data->enm_evt)
    {
        /* Connected to the MQTT broker */
        case MQTT_EVT_CONNECTED:
        {
            if (!g_b_mqtt_connected)
            {
                g_b_mqtt_connected = true;
                LOGI ("Connected with MQTT broker");

                /* Send a scanNotify command to notify about the presence of this node */
                s8_MQTTMN_Send_scanNotify ();
            }
            break;
        }

        /* Disconnected from the MQTT broker*/
        case MQTT_EVT_DISCONNECTED:
        {
            if (g_b_mqtt_connected)
            {
                g_b_mqtt_connected = false;
                ESP_LOGW (TAG, "Disconnected with MQTT broker");
            }
            break;
        }

        /* A data is received from a subscribed topic */
        case MQTT_EVT_DATA_RECEIVED:
        {
            v_MQTTMN_Process_Rx_Message (pstru_evt_data->stru_receive.pstri_topic,
                                         pstru_evt_data->stru_receive.u16_topic_len,
                                         pstru_evt_data->stru_receive.pv_data,
                                         pstru_evt_data->stru_receive.u32_data_len,
                                         pstru_evt_data->stru_receive.u32_offset,
                                         pstru_evt_data->stru_receive.u32_totlen);
            break;
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Processes message received from a subscribed topic
**
** @note
**      For message that exceeds the internal buffer, the message shall be split into multiple fragments. This function
**      shall be invoked multiple times, each time for one fragment. u32_offset and u32_total_len are used to keep
**      track of the fragmented data then.
**
** @param [in]
**      pstri_topic: The topic that the message comes from
**
** @param [in]
**      u16_topic_len: Length in bytes of the topic that the message comes from
**
** @param [in]
**      pv_data: Pointer to fragment's data
**
** @param [in]
**      u32_data_len: Length in bytes of the fragment's data
**
** @param [in]
**      u32_offset: Offset of this fragment in the whole message data
**
** @param [in]
**      u32_total_len: Total length (in bytes) of the whole message data
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_Process_Rx_Message (const char * pstri_topic, uint16_t u16_topic_len, const void * pv_data,
                                         uint32_t u32_data_len, uint32_t u32_offset, uint32_t u32_total_len)
{
    /* Validate arguments */
    ASSERT_PARAM ((pstri_topic != NULL) && (u16_topic_len < 256) && (pv_data != NULL));

    /* Parse the topic where we received the message from */
    uint32_t u32_master_node_id;
    bool b_is_command = false;
    if (b_MQTTMN_Parse_Topic (pstri_topic, u16_topic_len, &u32_master_node_id, &b_is_command) == false)
    {
        LOGE ("Topic of the received message is invalid");
        return;
    }

    /* Get the corresponding session with the master node */
    MQTTMN_session_t * pstru_session = pstru_MQTTMN_Get_Session (u32_master_node_id);
    if (pstru_session == NULL)
    {
        LOGE ("No session to communicate with master node %d", u32_master_node_id);
        return;
    }

    /* Process the message received */
    if (b_is_command)
    {
        /* We don't expect multiple fragments for commands */
        ASSERT_PARAM ((u32_offset == 0) && (u32_data_len == u32_total_len));
        v_MQTTMN_Process_Command (pstru_session, pv_data, u32_data_len);
    }
    else
    {
        v_MQTTMN_Process_Data (pstru_session, pv_data, u32_data_len, u32_offset, u32_total_len);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Parses an MQTT topic text for some important information
**
** @details
**      The MQTT client receives data from the following topics:
**          itor3/m2s/_broadcast_/<master_node_id>/command
**          itor3/m2s/_broadcast_/<master_node_id>/data
**          itor3/m2s/<group_id>/_broadcast_/<master_node_id>/command
**          itor3/m2s/<group_id>/_broadcast_/<master_node_id>/data
**          itor3/m2s/<group_id>/<slave_node_id>/<master_node_id>/command
**          itor3/m2s/<group_id>/<slave_node_id>/<master_node_id>/data
**
** @param [in]
**      pstri_topic: The topic to parse
**
** @param [in]
**      u16_topic_len: Length in bytes of the topic that the data comes from
**
** @param [out]
**      pu32_master_node_id: Master node ID obtained from the given topic
**
** @param [out]
**      pb_is_command: Whether the given topic is a command topic or data topic
**
** @return
**      @arg    true: Parsing is successful
**      @arg    false: Failed to parse the topic
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static bool b_MQTTMN_Parse_Topic (const char * pstri_topic, uint16_t u16_topic_len,
                                  uint32_t * pu32_master_node_id, bool * pb_is_command)
{
    uint8_t u8_num_slashes = 0;
    uint8_t au8_slash_pos[5];

    /* Determine number of forward slash and their position in the given topic */
    for (uint16_t u16_idx = 0; u16_idx < u16_topic_len; u16_idx++)
    {
        if (pstri_topic[u16_idx] == '/')
        {
            u8_num_slashes++;
            if (u8_num_slashes > 5)
            {
                LOGE ("Topic %s is invalid", pstri_topic);
                return false;
            }
            else
            {
                au8_slash_pos[u8_num_slashes - 1] = u16_idx;
            }
        }
    }

    /* Get master node ID and determine whether the topic is a command topic or data topic */
    if ((u8_num_slashes == 4) || (u8_num_slashes == 5))
    {
        /* Master node ID */
        uint16_t u16_node_id_len = au8_slash_pos[u8_num_slashes - 1] - au8_slash_pos[u8_num_slashes - 2] - 1;
        if (u16_node_id_len != 8)
        {
            LOGE ("Master node ID length of topic %s is invalid", pstri_topic);
            return false;
        }
        else
        {
            if (sscanf (&pstri_topic[au8_slash_pos[u8_num_slashes - 2] + 1], "%08X", pu32_master_node_id) != 1)
            {
                LOGE ("Master node ID string of topic %s is invalid", pstri_topic);
                return false;
            }
        }

        /* Topic type */
        uint16_t u16_type_len = u16_topic_len - au8_slash_pos[u8_num_slashes - 1] - 1;
        if (u16_type_len == sizeof ("command") - 1)
        {
            *pb_is_command = true;
        }
        else if (u16_type_len == sizeof ("data") - 1)
        {
            *pb_is_command = false;
        }
        else
        {
            LOGE ("Type of topic %s is invalid", pstri_topic);
            return false;
        }
    }
    else
    {
        LOGE ("Topic %s is invalid", pstri_topic);
        return false;
    }

    /* Done */
    return true;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Returns the session corresponding with a given master node ID. If it does not exist, establish a new session.
**
** @param [in]
**      u32_master_node_id: Master node ID of the session
**
** @return
**      @arg    NULL: failed to establish a new session for the given master node ID
**      @arg    otheriwse: Session corresponding with the given master node ID
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static MQTTMN_session_t * pstru_MQTTMN_Get_Session (uint32_t u32_master_node_id)
{
    MQTTMN_session_t * pstru_session = NULL;
    MQTTMN_session_t * pstru_candidate_session = NULL;

    /*
    ** If the communication session with the master node already exists, use it.
    ** Otherwise, create a new one if possible
    */
    for (uint8_t u8_idx = 0; u8_idx < NUM_COMM_SESSIONS; u8_idx++)
    {
        MQTTMN_session_t * pstru_temp_session = &g_astru_sessions[u8_idx];
        if (pstru_temp_session->b_active)
        {
            if (pstru_temp_session->u32_master_node_id == u32_master_node_id)
            {
                pstru_session = pstru_temp_session;
                TIMER_RESET (pstru_session->x_inact_timer);
                break;
            }
        }
        else
        {
            pstru_candidate_session = pstru_temp_session;
        }
    }

    if (pstru_session == NULL)
    {
        LOGI ("The session with master node ID 0x%08X doesn't exist yet", u32_master_node_id);
        if (pstru_candidate_session != NULL)
        {
            LOGI ("Establish a new session");
            pstru_session = pstru_candidate_session;
            pstru_session->b_active = true;
            TIMER_RESET (pstru_session->x_inact_timer);
            pstru_session->u32_master_node_id = u32_master_node_id;
            snprintf (pstru_session->stri_response_topic, sizeof (pstru_session->stri_response_topic),
                      "itor3/s2m/%s/%08X/%08X/response", g_pstri_group_id, g_u32_slave_node_id, u32_master_node_id);
            snprintf (pstru_session->stri_data_topic, sizeof (pstru_session->stri_data_topic),
                      "itor3/s2m/%s/%08X/%08X/data", g_pstri_group_id, g_u32_slave_node_id, u32_master_node_id);
            pstru_session->u32_request_eid = 0;
            pstru_session->u32_post_eid = 0;
        }
        else
        {
            LOGE ("No resource is available for communication session with master node ID 0x%08X", u32_master_node_id);
        }
    }

    /* Done */
    return pstru_session;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Processes received command
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      pv_data: Pointer to command data
**
** @param [in]
**      u32_len: Length in bytes of the command data
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_Process_Command (MQTTMN_session_t * pstru_session, const void * pv_data, uint32_t u32_len)
{
    cJSON *             px_json_root = NULL;
    cJSON *             px_json_node = NULL;
    char *              pstri_command = NULL;
    uint32_t            u32_eid = 0;
    MQTTMN_rx_cmd_t *   pstru_cmd = NULL;
    bool                b_success = true;

    /* Parse the command (JSON format) into JSON object */
    px_json_root = cJSON_ParseWithLength (pv_data, u32_len);
    if (px_json_root == NULL)
    {
        LOGE ("Failed to parse received command: %.*s", u32_len, (char *)pv_data);
        b_success = false;
    }

    /* Get command name */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, JSON_KEY_CMD);
        if (px_json_node == NULL)
        {
            LOGE ("Invalid command received: No " JSON_KEY_CMD " key");
            b_success = false;
        }
        else
        {
            pstri_command = px_json_node->valuestring;
        }
    }

    /* Get exchange ID */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, JSON_KEY_EID);
        if (px_json_node == NULL)
        {
            LOGE ("Invalid request command received: No " JSON_KEY_EID " key");
            b_success = false;
        }
        else
        {
            u32_eid = px_json_node->valueint;
        }
    }

    /* Determine which command was received */
    if (b_success)
    {
        b_success = false;
        for (uint8_t u8_idx = 0; u8_idx < MQTTMN_NUM_RX_CMD; u8_idx++)
        {
            pstru_cmd = &g_astru_rx_commands[u8_idx];
            if (strcmp (pstru_cmd->pstri_command, pstri_command) == 0)
            {
                b_success = true;
                break;
            }
        }

        if (!b_success)
        {
            LOGE ("Received unsupported command: %s", pstri_command);
        }
    }

    /* Handle the received command */
    if (b_success)
    {
        /* Discard repeated commands */
        if ((u32_eid != 0) &&
            (((pstru_cmd->b_is_request) && (u32_eid == pstru_session->u32_request_eid)) ||
             ((!pstru_cmd->b_is_request) && (u32_eid == pstru_session->u32_post_eid))))
        {
            LOGW ("Receive repeated command %s. Discard it", pstri_command);
        }
        else
        {
            /* Update session */
            if (pstru_cmd->b_is_request)
            {
                pstru_session->u32_request_eid = u32_eid;
            }
            else
            {
                pstru_session->u32_post_eid = u32_eid;
            }

            /* Invoke command handler */
            LOGI ("Command %s received", pstri_command);
            pstru_cmd->v_pfnc_cmd_handler (pstru_session, px_json_root);
        }
    }

    /* Clean up */
    if (px_json_root != NULL)
    {
        cJSON_Delete (px_json_root);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Processes received data message
**
** @note
**      For data that exceeds the internal buffer, the data shall be split into multiple fragments. This function
**      shall be invoked multiple times, each time for one fragment. u32_offset and u32_total_len are used to keep
**      track of the fragmented data then.
**
** @param [in]
**      pstru_session: the session through which the data message was received
**
** @param [in]
**      pv_data: Pointer to fragment's data
**
** @param [in]
**      u32_len: Length in bytes of the fragment's data
**
** @param [in]
**      u32_offset: Offset of this fragment in the whole topic data
**
** @param [in]
**      u32_total_len: Total length (in bytes) of the whole topic data
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_Process_Data (MQTTMN_session_t * pstru_session, const void * pv_data,
                                   uint32_t u32_len, uint32_t u32_offset, uint32_t u32_total_len)
{
    static lfs2_file_t x_file;

    /* Check if a file is being uploaded (its file path is not an empty string) */
    if (g_stri_upload_file[0] == 0)
    {
        LOGW ("Ignored received data, no file is being uploaded");
        return;
    }

    /* Cancel uploading if received data is invalid */
    uint32_t u32_rx_count = u32_offset + u32_len;
    if ((u32_rx_count > u32_total_len) || (u32_total_len > MQTT_MAX_FILE_SIZE))
    {
        LOGE ("Received data of the uploaded file is invalid (offset = %d, length = %d, total length = %d",
                  u32_offset, u32_len, u32_total_len);
        g_stri_upload_file[0] = 0;
        s8_MQTTMN_Send_statusNotify (NOTIFY_FILE_UPLOAD_STATUS, STATUS_ERR, "Invalid data");
        return;
    }

    /* Open the destination file to write the data */
    if (u32_offset == 0)
    {
        if (lfs2_file_open (g_px_lfs2, &x_file, g_stri_upload_file, LFS2_O_WRONLY | LFS2_O_CREAT | LFS2_O_TRUNC) < 0)
        {
            LOGE ("Failed to open file %s for writing", g_stri_upload_file);
            g_stri_upload_file[0] = 0;
            s8_MQTTMN_Send_statusNotify (NOTIFY_FILE_UPLOAD_STATUS, STATUS_ERR, "Failed to open file for writing");
            return;
        }
    }

    /* Store the received data to file */
    if (lfs2_file_write (g_px_lfs2, &x_file, pv_data, u32_len) != u32_len)
    {
        LOGE ("Failed to write data to file %s", g_stri_upload_file);
        lfs2_file_close (g_px_lfs2, &x_file);
        lfs2_remove (g_px_lfs2, g_stri_upload_file);
        g_stri_upload_file[0] = 0;
        s8_MQTTMN_Send_statusNotify (NOTIFY_FILE_UPLOAD_STATUS, STATUS_ERR, "Failed to write data to file");
        return;
    }

    /* Display progress every 20% of the file has been stored */
    if (u32_rx_count % (u32_total_len / 5) < u32_len)
    {
        LOGI ("%d/%d bytes of file %s has been received", u32_rx_count, u32_total_len, g_stri_upload_file);
    }

    /* If all data of this file has been written */
    if (u32_rx_count == u32_total_len)
    {
        LOGI ("%d bytes of file %s has been received completely", u32_total_len, g_stri_upload_file);

        /* Close and save file */
        if (lfs2_file_close (g_px_lfs2, &x_file) < 0)
        {
            LOGE ("Failed to save file %s", g_stri_upload_file);
            g_stri_upload_file[0] = 0;
            s8_MQTTMN_Send_statusNotify (NOTIFY_FILE_UPLOAD_STATUS, STATUS_ERR, "Failed to save file");
            return;
        }

        /* File uploading done */
        g_stri_upload_file[0] = 0;
        s8_MQTTMN_Send_statusNotify (NOTIFY_FILE_UPLOAD_STATUS, STATUS_OK, "");
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Converts a block of data into hex string (NULL-terminated string)
**
** @note
**      The memory allocated for the converted hexa string must be free after being used
**
** @details
**      Example: Input  = 0x12-0x34-0x56-0x78-0x9A-0xBC
**               Output = "12-34-56-78-9A-BC"
**
** @param [in]
**      pu8_data: Pointer to the data
**
** @param [in]
**      u8_len: Length in bytes of the data
**
** @param [out]
**      ppstri_hex: The allocated buffer containing hexa string (NULL-terminated) representing the data, NULL if failed
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_Data2Hex (const uint8_t * pu8_data, uint8_t u8_len, char ** ppstri_hex)
{
    /* Initialization */
    *ppstri_hex = NULL;

    /* Allocate memory for the converted string */
    char * pstri_buf = malloc (u8_len * 3);
    if (pstri_buf == NULL)
    {
        LOGE ("Failed to allocate memory for hexa converted string");
        return;
    }

    /* Conver the data into hexa string */
    for (uint8_t u8_idx = 0; u8_idx < u8_len; u8_idx++)
    {
        uint8_t u8_low_nibble = pu8_data[u8_idx] & 0x0F;
        uint8_t u8_high_nibble = (pu8_data[u8_idx] >> 4) & 0x0F;
        pstri_buf[u8_idx * 3 + 0] = (u8_high_nibble <= 9) ? ('0' + u8_high_nibble) : ('A' + (u8_high_nibble - 10));
        pstri_buf[u8_idx * 3 + 1] = (u8_low_nibble <= 9) ? ('0' + u8_low_nibble) : ('A' + (u8_low_nibble - 10));
        pstri_buf[u8_idx * 3 + 2] = '-';
    }
    pstri_buf[u8_len * 3 - 1] = 0;

    /* Done */
    *ppstri_hex = pstri_buf;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Converts a hex string (NULL-terminated string) into block of data
**
** @note
**      The memory allocated for the converted data must be free after being used
**
** @details
**      Example 1:  Input  = "12-34-56-78-9A-BC"
**                  Output = 0x12-0x34-0x56-0x78-0x9A-0xBC
**      Example 2:  Input  = "12-34-56-78-9A-BC-D"
**                  Output = 0x12-0x34-0x56-0x78-0x9A-0xBC-0xD0
**
** @param [in]
**      pstri_hex: Hexa string (NULL-terminated) representing the data
**
** @param [out]
**      ppu8_data: Pointer to the allocated buffer containing the converted data, NULL if failed
**
** @param [out]
**      pu8_len: Length in bytes of the data, 0 if failed
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_Hex2Data (const char * pstri_hex, uint8_t ** ppu8_data, uint8_t * pu8_len)
{
    /* Initialization */
    *ppu8_data = NULL;
    *pu8_len = 0;

    /* Allocated memory for the converted data */
    uint8_t u8_nibbles = (uint8_t)(strlen (pstri_hex) + 1) * 2 / 3;
    uint8_t u8_len = (u8_nibbles + 1) >> 1;
    uint8_t * pu8_buf = malloc (u8_len);
    if (pu8_buf == NULL)
    {
        LOGE ("Failed to allocate memory for converted data");
        return;
    }

    /* Convert hexa string into data */
    for (uint8_t u8_idx = 0; u8_idx < u8_nibbles; u8_idx++)
    {
        uint8_t u8_hex = pstri_hex [u8_idx * 3 / 2];
        uint8_t u8_nibble = (u8_hex <= '9') ? (u8_hex - '0') :
                            (u8_hex <= 'F') ? (u8_hex - 'A' + 10) : (u8_hex - 'a' + 10);
        if (u8_idx & 0x01)
        {
            pu8_buf [u8_idx >> 1] |= u8_nibble;
        }
        else
        {
            pu8_buf [u8_idx >> 1] = (u8_nibble << 4);
        }
    }

    /* Done */
    *ppu8_data = pu8_buf;
    *pu8_len = u8_len;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
