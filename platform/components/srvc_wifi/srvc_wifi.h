/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_wifi.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Aug 8
**  @brief      : Public header of Srvc_Wifi module
**  @namespace  : WIFI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Wifi
** @{
*/

#ifndef __SRVC_WIFI_H__
#define __SRVC_WIFI_H__

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

/** @brief  Status returned by APIs of Srvc_Wifi module */
enum
{
    WIFI_OK             = 0,        //!< The function executed successfully
    WIFI_ERR            = -1,       //!< There is unknown error while executing the function
};

/** @brief  Events fired by Srvc_Wifi module */
typedef enum
{
    WIFI_EVT_STA_CONNECTED,         //!< Connected to the AP in station mode
    WIFI_EVT_STA_IP_OBTAINED,       //!< IP address obtained from DHCP server in station mode
    WIFI_EVT_STA_DISCONNECTED,      //!< Disconnected from the AP in station mode
    WIFI_EVT_SAP_CONNECTED,         //!< A client has been connected with the AP in soft access point mode
    WIFI_EVT_SAP_DISCONNECTED,      //!< A client has been disconnected from the AP in soft access point mode
} WIFI_event_t;

/** @brief  Callback of Srvc_Wifi module */
typedef void (*WIFI_callback_t) (WIFI_event_t enm_event);

/** @brief  Maximum length in bytes of wifi SSID */
#define WIFI_SSID_LEN               33

/** @brief  Structure wrapping information of an access point */
typedef struct
{
    uint8_t au8_mac[6];                 //!< MAC address
    char    stri_ssid[WIFI_SSID_LEN];   //!< Wifi SSID
    int8_t  s8_rssi;                    //!< Wifi receive signal strength

} WIFI_ap_info_t;

/** @brief  IP address information of wifi interface */
typedef struct
{
    uint8_t au8_ip[4];              //!< IP addres in form of x.y.z.t, au8_ip[0] = x, au8_ip[3] = t
    uint8_t au8_netmask[4];         //!< Subnet mask in form of x.y.z.t, au8_netmask[0] = x, au8_netmask[3] = t
    uint8_t au8_gateway[4];         //!< Gateway address in form of x.y.z.t, au8_gateway[0] = x, au8_gateway[3] = t
    uint8_t au8_dns[4];             //!< DNS address in form of x.y.z.t, au8_dns[0] = x, au8_dns[3] = t

} WIFI_ip_info_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Initializes Srvc_Wifi module */
extern int8_t s8_WIFI_Init (void);

/* Registers Wifi event handler */
extern int8_t s8_WIFI_Register_Event_Handler (WIFI_callback_t pfnc_event_handler);

/* Connects to a wifi access point */
extern int8_t s8_WIFI_Connect (const char * stri_ssid, const char * stri_psw, const WIFI_ip_info_t * pstru_static_addr);

/* Creates a soft access point */
extern int8_t s8_WIFI_Create_Soft_Ap (const char * stri_ssid, const char * stri_psw,
                                      const WIFI_ip_info_t * pstru_sap_addr);

/* Stops WiFi interface */
extern int8_t s8_WIFI_Stop (void);

/* Disconnects from an access point in station mode */
extern int8_t s8_WIFI_Disconnect (void);

/* Reconnects to the access point in station mode */
extern int8_t s8_WIFI_Reconnect (void);

/* Scans for wifi access point list, the caller MUST free the list buffer when it's done */
extern int8_t s8_WIFI_Scan_Ap_List (WIFI_ap_info_t ** ppastru_ap_list, uint16_t * pu16_num_ap);

/* Gets IP address information of wifi interface */
extern int8_t s8_WIFI_Get_IP_Info (WIFI_ip_info_t * pstru_ip_info);

/* Gets MAC address of Wifi interface */
extern int8_t s8_WIFI_Get_Mac (uint8_t au8_mac[8]);

/* Gets information of the access point that is being connected with */
extern int8_t s8_WIFI_Get_Ap_Info (WIFI_ap_info_t * pstru_ap_info);

#endif /* __SRVC_WIFI_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
