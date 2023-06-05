/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_mqtt.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 May 22
**  @brief      : Public header of Srvc_Mqtt module
**  @namespace  : MQTT
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Mqtt
** @{
*/

#ifndef __SRVC_MQTT_H__
#define __SRVC_MQTT_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"             /* Use common definitions */
#include "srvc_mqtt_ext.h"          /* MQTT client configuration */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Handle to manage an MQTT client */
typedef struct MQTT_obj *               MQTT_inst_t;

/** @brief  Status returned by APIs of Srvc_Mqtt module */
typedef enum
{
    MQTT_OK                 = 0,        //!< The function executed successfully
    MQTT_ERR                = -1,       //!< There is unknown error while executing the function
} MQTT_status_t;

/** @brief  Expand entries in MQTT client instance table as enumeration of instance IDs */
#define MQTT_INST_TABLE_EXPAND_AS_INST_ID(INST_ID, ...)         INST_ID,
typedef enum
{
    MQTT_INST_TABLE (MQTT_INST_TABLE_EXPAND_AS_INST_ID)
    MQTT_NUM_INST
} MQTT_inst_id_t;

/** @brief   Expand entries in publish and subcribe tables of all MQTT clients into enumeration of topic IDs */
#define MQTT_TOPIC_TABLE_EXPAND_AS_TOPIC_ID(TOPIC_ID, ...)      TOPIC_ID,
#define MQTT_INST_TABLE_EXPAND_AS_ALL_TOPIC_IDS(INST_ID, PUB_TOPICS, SUB_TOPICS, ...)   \
    enum                                                                                \
    {                                                                                   \
        PUB_TOPICS (MQTT_TOPIC_TABLE_EXPAND_AS_TOPIC_ID)                                \
        PUB_TOPICS##_NUM_TOPICS                                                         \
    };                                                                                  \
    enum                                                                                \
    {                                                                                   \
        SUB_TOPICS (MQTT_TOPIC_TABLE_EXPAND_AS_TOPIC_ID)                                \
        SUB_TOPICS##_NUM_TOPICS                                                         \
    };
MQTT_INST_TABLE (MQTT_INST_TABLE_EXPAND_AS_ALL_TOPIC_IDS)

/** @brief  MQTT client configuration */
typedef struct
{
    const char *    stri_uri;           //!< Complete MQTT broker URI, NULL if not used
    const char *    stri_ip;            //!< MQTT broker's IP address, overriden by stri_uri
    uint16_t        u16_port;           //!< MQTT broker port, overriden by stri_uri
    const char *    stri_username;      //!< MQTT username used for connecting to the broker.
    const char *    stri_password;      //!< MQTT password used for connecting to the broker.
    const char *    stri_lwt_msg;       //!< Last will and testament (LWT) message, NULL to disable LWT feature
    uint32_t        u32_lwt_topic_id;   //!< Index of the topic in publish topic table for LWT feature
} MQTT_config_t;

/** @brief  Context data of the events fired by the module */
typedef struct
{
    MQTT_inst_t         x_inst;         //!< The MQTT client that fires the event
    void *              pv_arg;         //!< Argument passed when the associated callback function was registered

    /** @brief  Events fired by the module */
    enum
    {
        MQTT_EVT_CONNECTED,             //!< The MQTT client is connected to the broker
        MQTT_EVT_DISCONNECTED,          //!< The MQTT client is disconnected from the broker
        MQTT_EVT_DATA_RECEIVED,         //!< A data is received from a subscribed topic
    } enm_evt;

    /**
    ** @brief   Context data specific for MQTT_EVT_DATA_RECEIVED
    ** @note    This event can be fired multiple times for a received data if its length is too big. In that case,
    **          the received data will be split into multiple fragments, each fragment is contained in a call of this
    **          event.
    */
    struct
    {
        const char *    pstri_topic;    //!< The subscribed topic where the received data comes from
        uint16_t        u16_topic_len;  //!< Length in bytes of the topic pointed by pstri_topic
        const void *    pv_data;        //!< Pointer to the data of the received fragment
        uint32_t        u32_data_len;   //!< Length in bytes of the fragment data received
        uint32_t        u32_offset;     //!< Offset of this fragment in the whole data
        uint32_t        u32_totlen;     //!< Total length in bytes of the whole data
    } stru_receive;

} MQTT_evt_data_t;

/** @brief  Callback invoked when an event occurs */
typedef void (*MQTT_callback_t) (MQTT_evt_data_t * pstru_evt_data);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets instance of an MQTT client */
extern MQTT_inst_t x_MQTT_Get_Inst (MQTT_inst_id_t enm_inst_id);

/* Gets current configuration of an MQTT client */
extern void v_MQTT_Get_Config (MQTT_inst_t x_inst, MQTT_config_t * pstru_config);

/* Configures an MQTT client */
extern MQTT_status_t enm_MQTT_Set_Config (MQTT_inst_t x_inst, MQTT_config_t * pstru_config);

/* Changes a publish topic, pstri_topic must be static (i.e not in stack memory) */
extern void v_MQTT_Set_Publish_Topic (MQTT_inst_t x_inst, uint32_t u32_pub_topic_id, const char * pstri_topic);

/* Changes a subscribe topic, pstri_topic must be static (i.e not in stack memory) */
extern void v_MQTT_Set_Subscribe_Topic (MQTT_inst_t x_inst, uint32_t u32_sub_topic_id, const char * pstri_topic);

/* Registers a callback function invoked when an event occurs to an MQTT client */
extern void v_MQTT_Register_Callback (MQTT_inst_t x_inst, MQTT_callback_t pfnc_cb, void * pv_arg);

/* Starts an MQTT client */
extern MQTT_status_t enm_MQTT_Start_Inst (MQTT_inst_t x_inst);

/* Stops an MQTT client */
extern MQTT_status_t enm_MQTT_Stop_Inst (MQTT_inst_t x_inst);

/* Publishes data to a topic */
extern MQTT_status_t enm_MQTT_Publish (MQTT_inst_t x_inst, uint32_t u32_pub_topic_id,
                                       const void * pv_data, uint32_t u32_len);

#endif /* __SRVC_MQTT_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
