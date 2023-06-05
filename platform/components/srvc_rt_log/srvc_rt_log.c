/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_rt_log.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Oct 25
**  @brief      : Implementation of Srvc_Rt_Log module
**  @namespace  : RTLOG
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Rt_Log
**
** @brief       Srvc_Rt_Log module processes realtime log messages received from Slave board over UART interface
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_rt_log.h"                /* Public header of this module */
#include "srvc_ws_server.h"             /* Send realtime log to Websocket server */
#include "srvc_recovery.h"              /* Backup operating data when power is interrupted */

#include "cJSON.h"                      /* Use ESP-IDF's JSON component */
#include "string.h"                     /* Use strlen() */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Message ID */
enum
{
    RTLOG_MSG_RT_MEAS               = 0x11,     //!< Realtime measurement message
    RTLOG_MSG_POWER_INTERRUPTED     = 0x22,     //!< Notification sent when power interruption is detected
};

/** @brief  ID of realtime measurement */
typedef enum
{
    RTLOG_TOP_HEATER_TEMP           = 0,        //!< Temperature of top heater in Celsius degrees (fix16_t)
    RTLOG_BTM_HEATER_TEMP           = 1,        //!< Temperature of bottom heater in Celsius degrees (fix16_t)

} RTLOG_meas_id_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Rt_Log";

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Instance of the Websocket server to send realtime log messages to */
static WSS_inst_t g_x_ws_server_inst = NULL;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static int32_t s32_RTLOG_Init_Module (void);
static void v_RTLOG_Process_Rt_Meas (uint32_t u32_timestamp, uint8_t * pu8_data, uint8_t u8_len);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Parses the raw data of realtime log message and processes it
**
** @param [in]
**      u32_timestamp: Timestamp in milliseconds of the log message
**
** @param [in]
**      u8_msg_id: Message ID help differentiate types of realtime logs
**
** @param [in]
**      pv_data: Raw data of the log
**
** @param [in]
**      u8_len: Length in bytes of the data stored in pv_data
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_RTLOG_Process_Log_Data (uint32_t u32_timestamp, uint8_t u8_msg_id, const void * pv_data, uint8_t u8_len)
{
    /* If this module has not initialized yet, initialize it */
    if (!g_b_initialized)
    {
        g_b_initialized = (s32_RTLOG_Init_Module () == STATUS_OK);
        if (!g_b_initialized)
        {
            LOGE ("Failed to initialize realtime logging module");
            return;
        }
    }

    /* Process the received message */
    if (u8_msg_id == RTLOG_MSG_RT_MEAS)
    {
        v_RTLOG_Process_Rt_Meas (u32_timestamp, (uint8_t *)pv_data, u8_len);
    }
    else if (u8_msg_id == RTLOG_MSG_POWER_INTERRUPTED)
    {
        /* This message should be processed once until next power on */
        static bool b_power_interrupted = false;

        /* Backup operating data of cooking script onto non-volatile memory */
        if (!b_power_interrupted)
        {
            b_power_interrupted = true;
            v_RCVR_Backup_Data ();
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_Rt_Log module
**
** @return
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int32_t s32_RTLOG_Init_Module (void)
{
    /* Get instance of the Websocket server used to send the realtime log messages to */
    g_x_ws_server_inst = x_WSS_Get_Inst (WSS_SLAVE_RTLOG);
    if (g_x_ws_server_inst == NULL)
    {
        LOGE ("Failed to get instance of Websocket server used to send the realtime log messages");
        return STATUS_ERR;
    }

    return STATUS_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Parses the raw data from a realtime measurement message (message type 0x11) into JSON message and sends it to
**      Websocket server
**
** @details
**      Structure of au8_data[] part of the message:
**      + u32_meas_mask: Measurement ID x is available in the following data if bit x in this mask is 1
**      + ... : data of measurement ID x if its bit is 1 in u32_meas_mask
**
** @param [in]
**      u32_timestamp: Timestamp in milliseconds of the log message
**
** @param [in]
**      u8_msg_id: Message ID help differentiate types of realtime logs
**
** @param [in]
**      pu8_data: Raw data of the log
**
** @param [in]
**      u8_len: Length in bytes of the data stored in pu8_data
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_RTLOG_Process_Rt_Meas (uint32_t u32_timestamp, uint8_t * pu8_data, uint8_t u8_len)
{
    uint32_t    u32_meas_mask = ENDIAN_GET32 (pu8_data);
    float       flt_top_temp = -1;
    float       flt_btm_temp = -1;

    /* Do nothing if there is no measurement */
    if (u32_meas_mask == 0)
    {
        return;
    }

    /* Skip 4 bytes of measurement mask */
    pu8_data += 4;

    /* Construct JSON message object */
    cJSON * px_notify_root = cJSON_CreateObject ();
    cJSON_AddNumberToObject (px_notify_root, "Timestamp", u32_timestamp);

    /* Parse measurement values */
    for (uint8_t u8_meas_id = 0; u8_meas_id < 32; u8_meas_id++)
    {
        if (u32_meas_mask & 0x1)
        {
            switch (u8_meas_id)
            {
                case RTLOG_TOP_HEATER_TEMP:
                    flt_top_temp = ((int32_t)ENDIAN_GET32 (pu8_data)) / 65536.0;
                    pu8_data += 4;
                    cJSON_AddNumberToObject (px_notify_root, "Top heater temperature", flt_top_temp);
                    break;

                case RTLOG_BTM_HEATER_TEMP:
                    flt_btm_temp = ((int32_t)ENDIAN_GET32 (pu8_data)) / 65536.0;
                    pu8_data += 4;
                    cJSON_AddNumberToObject (px_notify_root, "Bottom heater temperature", flt_btm_temp);
                    break;
            }
        }

        u32_meas_mask >>= 1;
    }

    /* Send the JSON message through Websocket server */
    char * pstri_notify = cJSON_Print (px_notify_root);
    cJSON_Delete (px_notify_root);
    if (pstri_notify != NULL)
    {
        enm_WSS_Send (g_x_ws_server_inst, WSS_ALL_CLIENTS, pstri_notify, strlen (pstri_notify));
        free (pstri_notify);
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
