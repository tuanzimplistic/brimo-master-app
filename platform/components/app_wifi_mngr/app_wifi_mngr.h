/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : app_wifi_mngr.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Mar 12
**  @brief      : Public header of App_Wifi_Mngr module
**  @namespace  : WIFIMN
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Wifi_Mngr
** @{
*/

#ifndef __APP_WIFI_MNGR_H__
#define __APP_WIFI_MNGR_H__

#ifdef __cplusplus
extern "C" {
#endif

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

/** @brief  Status returned by APIs of App_Wifi_Mngr module */
enum
{
    WIFIMN_OK               = 0,        //!< The function executed successfully
    WIFIMN_ERR              = -1,       //!< There is unknown error while executing the function
    WIFIMN_ERR_BUSY         = -2,       //!< The function failed because the module is busy
};

/** @brief  Maximum length in bytes of wifi SSID */
#define WIFIMN_SSID_LEN     33

/** @brief  Maximum length in bytes of wifi password */
#define WIFIMN_PSW_LEN      65

/** @brief  Structure wrapping SSID and password of an access point */
typedef struct
{
    char stri_ssid [WIFIMN_SSID_LEN];   //!< Wifi SSID
    char stri_psw [WIFIMN_PSW_LEN];     //!< Wifi password

} WIFIMN_cred_t;

/** @brief  Structure wrapping information of an access point */
typedef struct
{
    char stri_ssid [WIFIMN_SSID_LEN];   //!< Wifi SSID

} WIFIMN_ap_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Initializes App_Wifi_Mngr module */
extern int8_t s8_WIFIMN_Init (void);

/* Gets information of the user configurable wifi access point */
extern int8_t s8_WIFIMN_Get_User_Ap (WIFIMN_cred_t ** ppstru_ap);

/* Gets information of the currently selected wifi access point and checks the connectivity with it */
extern int8_t s8_WIFIMN_Get_Selected_Ap (WIFIMN_cred_t ** ppstru_ap, bool * pb_connected);

/* Gets number of backup access points */
extern int8_t s8_WIFIMN_Get_Num_Backup_Ap (uint8_t * pu8_num_backup_ap);

/* Forces to connect with an access point */
extern int8_t s8_WIFIMN_Connect (const WIFIMN_cred_t * pstru_ap);

/* Forces to disconnect from the current access point */
extern int8_t s8_WIFIMN_Disconnect (void);

/* Starts scanning of all currently available access points */
extern int8_t s8_WIFIMN_Start_Scan (void);

/* Gets list of available access points in last scan */
extern int8_t s8_WIFIMN_Get_Scan_Ap_List (WIFIMN_ap_t ** ppastru_ap_list, uint16_t * pu16_num_ap);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __APP_WIFI_MNGR_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
