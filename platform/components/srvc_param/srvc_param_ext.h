/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_param_ext.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Feb 12
**  @brief      : Extended public header of Srvc_Param module
**  @namespace  : PARAM
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Param
** @{
*/

#ifndef __SRVC_PARAM_EXT_H__
#define __SRVC_PARAM_EXT_H__

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

/**
** @brief   Parameter table
** @details
**
** Definition of parameter table. Each parameter has following properties:
**
** - Param_ID       : Unique literal definition of a parameter
**
** - PUC            : Parameter unique code (16-bit number) uniquely identifying a parameter
**
** - Type           : Data type of a parameter. The following types are supported:
**                      + uint8_t, int8_t
**                      + uint16_t, int16_t
**                      + uint32_t, int32_t
**                      + uint64_t, int64_t
**                      + string: NUL-terminated string
**                      + blob: variable length binary data (array of uint8_t elements)
**
** - Min, Max       : Minimum and maximum value of a parameter. If the parameter value is not within this range, it
**                    shall be reset to default value when Srvc_Param is initialized. Setting both Min and Max to 0
**                    shall disable these limits.
**                    For parameters of types string and blob, these settings are minimum and maximum length in bytes
**                    of data stored.
**
** - Default        : Initialized value of the parameter when it is first created or restored from a corruption.
**                    Some examples:
**                         Type     |   Default
**                      ------------+-----------------------
**                         uint8_t  |   12
**                         int16_t  |   -1234
**                         string   |   "Default string"
**                         blob     |   {1, 2, 3, 4, 5, 6}
*/
#define PARAM_TABLE(X)                                                                                                 \
                                                                                                                       \
/*-------------------------------------------------------------------------------------------------------------------*/\
/* Param_ID                         PUC         Type        Min         Max         Default                          */\
/*-------------------------------------------------------------------------------------------------------------------*/\
                                                                                                                       \
/* Wifi SSID */                                                                                                        \
X( PARAM_WIFI_SSID,                 0x0000,     string,     0,          33,         "my_ssid"                         )\
                                                                                                                       \
/* Wifi password */                                                                                                    \
X( PARAM_WIFI_PSW,                  0x0001,     string,     0,          65,         "my_password"                     )\
                                                                                                                       \
/* MQTT group that this MQTT client belongs to */                                                                      \
X( PARAM_MQTT_GROUP_ID,             0x0010,     string,     0,          33,         "default"                         )\
                                                                                                                       \
/* Operating data of cooking script */                                                                                 \
X( PARAM_COOKING_SCRIPT_DATA,       0x0020,     blob,       0,          256,        {0}                               )\
                                                                                                                       \
/*-------------------------------------------------------------------------------------------------------------------*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#endif /* __SRVC_PARAM_EXT_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
