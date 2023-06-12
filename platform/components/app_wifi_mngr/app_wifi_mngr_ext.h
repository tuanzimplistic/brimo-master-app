/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : app_wifi_mngr_ext.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 July 24
**  @brief      : Extended header of App_Wifi_Mngr module
**  @namespace  : WIFIMN
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Wifi_Mngr
** @{
*/

#ifndef __APP_WIFI_MNGR_EXT_H__
#define __APP_WIFI_MNGR_EXT_H__

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
** @brief   Table of backup wifi access points
**
** @details
**  Wifi Manager manages one user configurable access point and a list of predefined backup access points.
**  Initilially, Wifi Manager shall try to connect with the user configurable access point. If that fails, it
**  shall retry to connect with the wifi access points provided in this list, one by one, until a wifi connection
**  is established. If all connect attempts fail, Wifi Manager shall try with the user configurable access point
**  again, and so on.
**
**  The table can have any number of entries. If the table is empty, Wifi Manager shall only use user configurable
**  access point.
**
**  Each entry in the table has the following properties:
**
**  - Access_point_ID    : Alias of backup wifi access point
**
**  - Wifi_SSID          : SSID (wifi name) of the corresponding access point (maximum 32 characters)
**
**  - Wifi_Password      : Password of the corresponding access point (maximum 64 characters)
*/
#define WIFIMN_BACKUP_AP_TABLE(X)                                                        \
                                                                                         \
/*-------------------------------------------------------------------------------------*/\
/*  Access_point_ID         Wifi_SSID               Wifi_Password                      */\
/*-------------------------------------------------------------------------------------*/\
                                                                                         \
X(  WIFIMN_BACKUP_AP_1,     "Zimplistic",       "Zimplistic123"                          )\
                                                                                         \
/*-------------------------------------------------------------------------------------*/

/** @brief  Macro expanding entries in WIFIMN_BACKUP_AP_TABLE as initialization value for WIFIMN_cred_t */
#define WIFIMN_EXPAND_BACKUP_TABLE_AS_STRUCT_INIT(ALIAS, SSID, PASSWORD)                 \
{                                                                                        \
    .stri_ssid      = SSID,                                                              \
    .stri_psw       = PASSWORD,                                                          \
},

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


#endif /* __APP_WIFI_MNGR_EXT_H__ */

/**
** @}
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
