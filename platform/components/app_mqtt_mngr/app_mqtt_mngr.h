/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : app_mqtt_mngr.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Mar 16
**  @brief      : Public header of App_Mqtt_Mngr module
**  @namespace  : MQTTMN
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Mqtt_Mngr
** @{
*/

#ifndef __APP_MQTT_MNGR_H__
#define __APP_MQTT_MNGR_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"             /* Use common definitions */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Status returned by APIs of App_Mqtt_Mngr module */
enum
{
    MQTTMN_OK           = 0,        //!< The function executed successfully
    MQTTMN_ERR          = -1,       //!< There is unknown error while executing the function
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Initializes App_Mqtt_Mngr module */
extern int8_t s8_MQTTMN_Init (void);

/* Sends a notify via MQTT interface about firmware download progress of OTA firmware update */
extern int8_t s8_MQTTMN_Notify_Ota_Download_Progress (uint8_t u8_percents);

/* Sends a notify message via MQTT interface about firmware install progress of OTA firmware update */
extern int8_t s8_MQTTMN_Notify_Ota_Install_Progress (uint8_t u8_percents);

/* Sends a notify message via MQTT interface about overall status of OTA firmware update when done */
extern int8_t s8_MQTTMN_Notify_Ota_Status (bool b_ok, const char * pstri_error_desc);

/* Gets total size and free space of the LittleFS storage */
extern int8_t s8_MQTTMN_Get_Storage_Space (uint32_t * pu32_total_space, uint32_t * pu32_free_space);

#endif /* __APP_MQTT_MNGR_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
