/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_mqtt_ext.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 May 22
**  @brief      : Extended public header of Srvc_Mqtt module
**  @namespace  : MQTT
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Mqtt
** @{
*/

#ifndef __SRVC_MQTT_EXT_H__
#define __SRVC_MQTT_EXT_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  NULL topic table used for client which does not have any publish and/or subscribe topics */
#define MQTT_NULL_TABLE(X)

/**
** @brief   Table of all MQTT clients
** @details
**
** Each instance in this table encapsulates an MQTT client and has the following properties:
**
** - MQTT_Inst_ID           : Alias of an MQTT client
**
** - Publish topic table    : Name of the table containing information of all MQTT topics that the corresponding
**                            client publishes to. Use MQTT_NULL_TABLE if the client doesn't publish to any topics.
**
** - Subcribe topic table   : Name of the table containing information of all MQTT topics that the corresponding
**                            client subcribes to. Use MQTT_NULL_TABLE if the client doesn't subcribe to any topics.
**
** - Broker's URI           : Complete MQTT broker URI, leave this as NULL to manually configure the Broker with
**                            other parameters (IP, port, etc.)
**                            Example: "mqtt://mqtt.eclipseprojects.io"
**                                     "mqtt://username:password@mqtt.eclipseprojects.io:1884"
**
** - Broker's IP            : MQTT broker domain (ipv4 as string), setting the Broker's URI will override this parameter.
**                            Use NULL if this parameter is not used.
**                            Example: "192.168.0.123"
**
** - Broker's port          : MQTT broker port, specifying the port in Broker's URI will override this
**                            Example: 1883
**
** - User name              : MQTT username used for connecting to the broker, setting the Broker's URI will override
**                            this parameter. Use NULL if this parameter is not used.
**                            Example: "my_username"
**
** - Password               : MQTT password used for connecting to the broker, setting the Broker's URI will override
**                            this parameter. Use NULL if this parameter is not used.
**                            Example: "my_password"
**
** - LWT message            : The last will and testament (LWT) message to notify other clients when a client
**                            ungracefully disconnects. Use NULL if LWT feature is not needed.
**                            Example: "offline"
**
** - LWT topic ID           : ID of the topic in "Publish topic table" that will be used for last will and testament
**                            feature. If "LWT message" is NULL, the value of this parameter is not cared.
**                            Example: MQTT_TOPIC_LWT
**
** - Transmit buffer size   : Size in bytes of transmit buffer
**
** - Receive buffer size    : Size in bytes of receive buffer
**
*/
#define MQTT_INST_TABLE(X)                                                                           \
                                                                                                     \
/*-------------------------------------------------------------------------------------------------*/\
/* MQTT_Inst_ID             Configuration                                                          */\
/*-------------------------------------------------------------------------------------------------*/\
                                                                                                     \
/* MQTT client */                                                                                    \
X( MQTT_ESP32_CLIENT,       /* Publish topic table  */      MQTT_PUB_TOPIC_TABLE                    ,\
                            /* Subcribe topic table */      MQTT_SUB_TOPIC_TABLE                    ,\
                            /* Broker's URI         */      "mqtt://broker.hivemq.com"              ,\
                            /* Broker's IP          */      NULL                                    ,\
                            /* Broker's port        */      0                                       ,\
                            /* User name            */      NULL                                    ,\
                            /* Password             */      NULL                                    ,\
                            /* LWT message          */      NULL                                    ,\
                            /* LWT topic ID         */      0                                       ,\
                            /* Transmit buffer size */      2048                                    ,\
                            /* Receive buffer size  */      2048                                    )\
                                                                                                     \
/*-------------------------------------------------------------------------------------------------*/

/**
** @brief   Table of all topics that ESP32 client publishes to
** @details
**
** - Topic_ID   : Alias of a topic
**
** - QOS        : Quality of service, which is 0, 1 or 2
**
** - Retained   : Specifies the retain flag, which is either true or false
**
** - Topic      : The topic
**                Example: "client1/status"
*/
#define MQTT_PUB_TOPIC_TABLE(X)                                                                                        \
/*-------------------------------------------------------------------------------------------------------------------*/\
/* Topic_ID                 QOS     Retained    Topic                                                                */\
/*-------------------------------------------------------------------------------------------------------------------*/\
                                                                                                                       \
/* Placeholder of the topic sending unicast response messages */                                                       \
X( MQTT_S2M_RESPONSE,       1,      false,      "itor3/s2m/<group_id>/<slave_node_id>/<master_node_id>/response"      )\
                                                                                                                       \
/* Placeholder of the topic sending unicast data messages */                                                           \
X( MQTT_S2M_DATA,           1,      false,      "itor3/s2m/<group_id>/<slave_node_id>/<master_node_id>/data"          )\
                                                                                                                       \
/* Placeholder of the topic sending broadcast notify messages */                                                       \
X( MQTT_S2M_NOTIFY,         1,      false,      "itor3/s2m/<group_id>/<slave_node_id>/notify"                         )\
                                                                                                                       \
/*-------------------------------------------------------------------------------------------------------------------*/

/**
** @brief   Table of all topics that ESP32 client subscribes to
** @details
**
** - Topic_ID   : Alias of a topic
**
** - QOS        : Quality of service, which is 0, 1 or 2
**
** - Topic      : The topic
**                Example: "client1/status"
*/
#define MQTT_SUB_TOPIC_TABLE(X)                                                                                        \
/*-------------------------------------------------------------------------------------------------------------------*/\
/* Topic_ID                 QOS                 Topic                                                                */\
/*-------------------------------------------------------------------------------------------------------------------*/\
                                                                                                                       \
/* Placeholder of the topic receiving unicast messages from back-office nodes */                                       \
X( MQTT_M2S_UNICAST,        1,                  "itor3/m2s/<group_id>/<slave_node_id>/#"                              )\
                                                                                                                       \
/* Placeholder of the topic receiving multicast (group broadcast) messages from back-office nodes */                   \
X( MQTT_M2S_MULTICAST,      1,                  "itor3/m2s/<group_id>/_broadcast_/#"                                  )\
                                                                                                                       \
/* Topic receiving broadcast messages from back-office nodes */                                                        \
X( MQTT_M2S_BROADCAST,      1,                  "itor3/m2s/_broadcast_/#"                                             )\
                                                                                                                       \
/*-------------------------------------------------------------------------------------------------------------------*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#endif /* __SRVC_MQTT_EXT_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
