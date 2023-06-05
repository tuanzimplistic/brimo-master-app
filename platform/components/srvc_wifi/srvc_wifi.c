/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_wifi.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Aug 8
**  @brief      : Implementation of Srvc_Wifi module
**  @namespace  : WIFI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Wifi
** @brief       Provides a wrapper of ESP-IDF's wifi component
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_wifi.h"              /* Public header of this module */
#include "esp_wifi.h"               /* Use ESP-IDF's wifi component */

#include <string.h>                 /* Use strncpy(), bzero(), snprintf() */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Logging tag of this module */
#define TAG                         "Srvc_Wifi"

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Callback invoked when wifi event occurs */
static WIFI_callback_t g_pfnc_event_handler;

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Indicates if Wifi component has been started */
static bool g_b_started = false;

/** @brief  Indicates if Wifi has been connected to an access point (station mode) */
static bool g_b_connected = false;

/** @brief  Wifi network interface for wifi station (station mode) */
static esp_netif_t * g_px_wifi_sta_if = NULL;

/** @brief  Wifi network interface for soft access point (access point mode) */
static esp_netif_t * g_px_wifi_sap_if = NULL;

/** @brief  Indicates if static IP address will be used when connected to an AP (DHCP is not used) */
static bool g_b_static_addr_used = false;

/** @brief  Static IP address used (in case g_b_static_addr_used is true) */
static WIFI_ip_info_t g_stru_static_addr;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static void v_WIFI_Evt_Handler (void * pv_arg, esp_event_base_t x_evt_base, int32_t s32_evt_id, void * pv_evt_data);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_Wifi module
**
** @return
**      @arg    WIFI_OK
**      @arg    WIFI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFI_Init (void)
{
    if (g_b_initialized)
    {
        return WIFI_OK;
    }
    LOGD ("Initializing Srvc_Wifi module");

    /* Initialize the underlying TCP/IP stack */
    ESP_ERROR_CHECK (esp_netif_init ());

    /* Subcribe to all wifi events */
    ESP_ERROR_CHECK (esp_event_handler_instance_register (WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                          v_WIFI_Evt_Handler, NULL, NULL));

    /* Subcribe to Got IP event */
    ESP_ERROR_CHECK (esp_event_handler_instance_register (IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                          v_WIFI_Evt_Handler, NULL, NULL));

    /* Initialize WiFi with default configuration, allocate resource for WiFi driver, and start WiFi task */
    wifi_init_config_t x_default_cfg = WIFI_INIT_CONFIG_DEFAULT ();
    ESP_ERROR_CHECK (esp_wifi_init (&x_default_cfg));

    /* Disable any WiFi power save mode, this allows better throughput */
    ESP_ERROR_CHECK (esp_wifi_set_ps (WIFI_PS_NONE));

    /* Done */
    g_b_initialized = true;
    return WIFI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Registers Wifi event handler
**
** @param [in]
**      pfnc_event_handler: The callback function invoked when an event of Srvc_Wifi module occurs, NULL if not used
**
** @return
**      @arg    WIFI_OK
**      @arg    WIFI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFI_Register_Event_Handler (WIFI_callback_t pfnc_event_handler)
{
    ASSERT_PARAM (g_b_initialized);

    /* Store event handler callback */
    g_pfnc_event_handler = pfnc_event_handler;

    return WIFI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Connects to a wifi access point
**
** @note
**      This function puts the wifi interface into station mode. Soft access point, if any, therefore will be terminated
**
** @param [in]
**      stri_ssid: SSID of the access point to connect
**
** @param [in]
**      stri_psw: Password (WPA or WPA2) of the access point. The password must have at least 8 characters.
**
** @param [in]
**      pstru_static_addr
**      @arg    NULL: upon connected to the given access point, station IP address will be assigned by DHCP server
**      @arg    Otherwise: pointer to the IP address that will be assigned to the station upon connected to the
**                         given access point (DHCP is not used)
**
** @return
**      @arg    WIFI_OK
**      @arg    WIFI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFI_Connect (const char * stri_ssid, const char * stri_psw, const WIFI_ip_info_t * pstru_static_addr)
{
    ASSERT_PARAM (g_b_initialized && (stri_ssid != NULL) && (stri_psw != NULL));

    /* According to Wifi standard, wifi password must have at least 8 characters */
    if (strlen (stri_psw) < 8)
    {
        LOGE ("Wifi password has less than 8 characters");
        return WIFI_ERR;
    }

    /* Creates default WIFI station if have not done that */
    if (g_px_wifi_sta_if == NULL)
    {
        g_px_wifi_sta_if = esp_netif_create_default_wifi_sta ();

        /* Set host name of Wifi interface to be the device name and 3 LSB of MAC address */
        uint8_t au8_mac[8];
        esp_wifi_get_mac (WIFI_IF_STA, au8_mac);
        char stri_host_name[32];
        snprintf (stri_host_name, sizeof (stri_host_name), 
                  CONFIG_LWIP_LOCAL_HOSTNAME "_%02X%02X%02X", au8_mac[3], au8_mac[4], au8_mac[5]);
        esp_netif_set_hostname (g_px_wifi_sta_if, stri_host_name);
    }

    /* Stop connecting with any access point */
    if (g_b_connected)
    {
        esp_wifi_disconnect ();
        while (g_b_connected)
        {
            vTaskDelay (pdMS_TO_TICKS (10));
        }
    }

    /* Stop wifi interface */
    if (g_b_started)
    {
        esp_wifi_stop ();
        vTaskDelay (pdMS_TO_TICKS (10));
        g_b_started = false;
    }

    /* Ensure that the wifi interface is in station mode */
    wifi_mode_t enm_mode;
    if ((esp_wifi_get_mode (&enm_mode) != ESP_OK) || (enm_mode != WIFI_MODE_STA))
    {
        ESP_ERROR_CHECK (esp_wifi_set_mode (WIFI_MODE_STA));
    }

    /* Configure the wifi station */
    wifi_config_t stru_wifi_cfg;
    bzero (&stru_wifi_cfg, sizeof (wifi_config_t));
    strncpy ((char *)stru_wifi_cfg.sta.ssid, stri_ssid, sizeof (stru_wifi_cfg.sta.ssid));
    strncpy ((char *)stru_wifi_cfg.sta.password, stri_psw, sizeof (stru_wifi_cfg.sta.password));

    stru_wifi_cfg.sta.pmf_cfg.capable = true;
    stru_wifi_cfg.sta.pmf_cfg.required = false;

    /*
    ** Setting a password implies station will connect to all security modes including WEP/WPA.
    ** However these modes are deprecated and not advisable to be used. Incase your Access point
    ** doesn't support WPA2, these mode can be enabled by commenting below line
    */
    stru_wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK (esp_wifi_set_config (ESP_IF_WIFI_STA, &stru_wifi_cfg));

    /* If using static IP assignment, store the static address */
    g_b_static_addr_used = (pstru_static_addr != NULL);
    if (g_b_static_addr_used)
    {
        g_stru_static_addr = *pstru_static_addr;
    }

    /* Start wifi component */
    ESP_ERROR_CHECK (esp_wifi_start ());

    return WIFI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Creates a soft access point
**
** @note
**      This function puts the wifi interface into access point mode. If the wifi is being connected with an access
**      point in station mode, it will be disconnected.
**
** @param [in]
**      stri_ssid: SSID of the access point
**
** @param [in]
**      stri_psw: Password (WPA or WPA2) of the access point. The password must have at least 8 characters.
**
** @param [in]
**      pstru_sap_addr
**      @arg    NULL: use default addresses (192.168.4.1/24) for the DHCP server
**      @arg    Otherwise: pointer to the structure containing the addresses that will be assigned for the DHCP server
**
** @return
**      @arg    WIFI_OK
**      @arg    WIFI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFI_Create_Soft_Ap (const char * stri_ssid, const char * stri_psw, const WIFI_ip_info_t * pstru_sap_addr)
{
    ASSERT_PARAM (g_b_initialized && (stri_ssid != NULL) && (stri_psw != NULL));

    /* According to Wifi standard, wifi password must have at least 8 characters */
    if (strlen (stri_psw) < 8)
    {
        LOGE ("Wifi password has less than 8 characters");
        return WIFI_ERR;
    }

    /* Creates default WIFI soft access point if not done that */
    if (g_px_wifi_sap_if == NULL)
    {
        g_px_wifi_sap_if = esp_netif_create_default_wifi_ap ();

        /* Set host name of Wifi interface to be the device name and 3 LSB of MAC address */
        uint8_t au8_mac[8];
        esp_wifi_get_mac (WIFI_IF_AP, au8_mac);
        char stri_host_name[32];
        snprintf (stri_host_name, sizeof (stri_host_name), 
                  CONFIG_LWIP_LOCAL_HOSTNAME "_%02X%02X%02X", au8_mac[3], au8_mac[4], au8_mac[5]);
        esp_netif_set_hostname (g_px_wifi_sap_if, stri_host_name);
    }

    /* Stop connecting with any access point */
    if (g_b_connected)
    {
        esp_wifi_disconnect ();
        while (g_b_connected)
        {
            vTaskDelay (pdMS_TO_TICKS (10));
        }
    }

    /* Stop wifi interface */
    if (g_b_started)
    {
        esp_wifi_stop ();
        vTaskDelay (pdMS_TO_TICKS (10));
        g_b_started = false;
    }

    /* Configure addresses of the soft access point if requested */
    if (pstru_sap_addr != NULL)
    {
        esp_netif_ip_info_t stru_ip_info;
        bzero (&stru_ip_info, sizeof (stru_ip_info));
        IP4_ADDR (&stru_ip_info.ip, pstru_sap_addr->au8_ip[0], pstru_sap_addr->au8_ip[1],
                                    pstru_sap_addr->au8_ip[2], pstru_sap_addr->au8_ip[3]);
        IP4_ADDR (&stru_ip_info.gw, pstru_sap_addr->au8_gateway[0], pstru_sap_addr->au8_gateway[1],
                                    pstru_sap_addr->au8_gateway[2], pstru_sap_addr->au8_gateway[3]);
        IP4_ADDR (&stru_ip_info.netmask, pstru_sap_addr->au8_netmask[0], pstru_sap_addr->au8_netmask[1],
                                         pstru_sap_addr->au8_netmask[2], pstru_sap_addr->au8_netmask[3]);

        esp_netif_dhcps_stop (g_px_wifi_sap_if);
        ESP_ERROR_CHECK (esp_netif_set_ip_info (g_px_wifi_sap_if, &stru_ip_info));
        esp_netif_dhcps_start (g_px_wifi_sap_if);
    }

    /* Ensure that the wifi interface is in access point mode */
    wifi_mode_t enm_mode;
    if ((esp_wifi_get_mode (&enm_mode) != ESP_OK) || (enm_mode != WIFI_MODE_AP))
    {
        ESP_ERROR_CHECK (esp_wifi_set_mode (WIFI_MODE_AP));
    }

    /* Configure the soft access point */
    wifi_config_t stru_wifi_cfg;
    bzero (&stru_wifi_cfg, sizeof (wifi_config_t));
    strncpy ((char *)stru_wifi_cfg.ap.ssid, stri_ssid, sizeof (stru_wifi_cfg.ap.ssid));
    strncpy ((char *)stru_wifi_cfg.ap.password, stri_psw, sizeof (stru_wifi_cfg.ap.password));
    stru_wifi_cfg.ap.channel = 1;
    stru_wifi_cfg.ap.max_connection = 4;
    stru_wifi_cfg.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    ESP_ERROR_CHECK (esp_wifi_set_config (ESP_IF_WIFI_AP, &stru_wifi_cfg));

    /* Start wifi component */
    ESP_ERROR_CHECK (esp_wifi_start ());

    LOGD ("Start wifi access point");
    return WIFI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Stops WiFi interface
**
** @return
**      @arg    WIFI_OK
**      @arg    WIFI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFI_Stop (void)
{
    ASSERT_PARAM (g_b_initialized);

    /* Stop connecting with any access point */
    if (g_b_connected)
    {
        esp_wifi_disconnect ();
        while (g_b_connected)
        {
            vTaskDelay (pdMS_TO_TICKS (10));
        }
    }

    /* Stop wifi interface */
    if (g_b_started)
    {
        esp_wifi_stop ();
        vTaskDelay (pdMS_TO_TICKS (10));
        g_b_started = false;
    }

    /* Disable Wifi as a station or an acess point */
    esp_wifi_set_mode (WIFI_MODE_NULL);

    return WIFI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Disconnects from the access point in station mode
**
** @note
**      This function is applicable for station mode only
**
** @return
**      @arg    WIFI_OK
**      @arg    WIFI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFI_Disconnect (void)
{
    LOGD ("Disconnecting from Wifi");
    ASSERT_PARAM (g_b_initialized);

    /* Ensure that the wifi interface is in station mode */
    wifi_mode_t enm_mode;
    if ((esp_wifi_get_mode (&enm_mode) != ESP_OK) || (enm_mode != WIFI_MODE_STA))
    {
        LOGE ("The wifi interface is not in station mode");
        return WIFI_ERR;
    }

    /* Disconnect from Wifi */
    esp_wifi_disconnect ();

    return WIFI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Reconnects to the access point if it's been disconnected before with s8_WIFI_Disconnect()
**
** @note
**      This function is applicable for station mode only
**
** @return
**      @arg    WIFI_OK
**      @arg    WIFI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFI_Reconnect (void)
{
    LOGD ("Reconnectting to Wifi");
    ASSERT_PARAM (g_b_initialized);

    if (g_b_started)
    {
        /* Ensure that the wifi interface is in station mode */
        wifi_mode_t enm_mode;
        if ((esp_wifi_get_mode (&enm_mode) != ESP_OK) || (enm_mode != WIFI_MODE_STA))
        {
            LOGE ("The wifi interface is not in station mode");
            return WIFI_ERR;
        }

        if (!g_b_connected)
        {
            esp_wifi_connect ();
        }
        return WIFI_OK;
    }
    else
    {
        return WIFI_ERR;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Scans for wifi access point list, the caller MUST free the list buffer when it's done
**
** @note
**      The wifi driver must not be on connecting progress when this function is called, otherwise the function will fail
**      This function is applicable for station mode only
**
** @note
**      This is a blocking function, i.e the function only returns when scanning is done
**
** @param [out]
**      ppstru_ap_info: Pointer to the array containing list of wifi access points.
**                      This is NULL if there is no access point found.
**
** @param [out]
**      pu16_num_ap: Number of items stored in ppastru_ap_list
**                   This is 0 if there is no access point found.
**
** @return
**      @arg    WIFI_OK
**      @arg    WIFI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFI_Scan_Ap_List (WIFI_ap_info_t ** ppastru_ap_list, uint16_t * pu16_num_ap)
{
    esp_err_t x_err;

    ASSERT_PARAM (g_b_initialized && (ppastru_ap_list != NULL) && (pu16_num_ap != NULL));

    /* Ensure that the wifi interface is in station mode */
    wifi_mode_t enm_mode;
    if ((esp_wifi_get_mode (&enm_mode) != ESP_OK) || (enm_mode != WIFI_MODE_STA))
    {
        LOGE ("The wifi interface is not in station mode");
        return WIFI_ERR;
    }

    /* Start wifi component if it's started */
    if (!g_b_started)
    {
        ESP_ERROR_CHECK (esp_wifi_start ());
    }

    /* Start wifi scanning in blocking mode */
    x_err = esp_wifi_scan_start (NULL, true);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to start wifi scanning (%s)", esp_err_to_name (x_err));
        return WIFI_ERR;
    }

    /* Get number of APs found in last scan, retry several times if no access point found */
    uint16_t u16_num_ap;
    for (uint8_t u8_retry = 0; u8_retry < 15; u8_retry++)
    {
        x_err = esp_wifi_scan_get_ap_num (&u16_num_ap);
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to get number of access points in last scan (%s)", esp_err_to_name (x_err));
            return WIFI_ERR;
        }
        if (u16_num_ap)
        {
            break;
        }
        vTaskDelay (pdMS_TO_TICKS(200));
    }

    /* If no access point was found */
    if (u16_num_ap == 0)
    {
        *ppastru_ap_list = NULL;
        *pu16_num_ap = 0;
        LOGW ("Found no Wifi access point");
        return WIFI_OK;
    }

    /* Allocate memory to store AP records */
    wifi_ap_record_t * pstru_ap_records = calloc (u16_num_ap, sizeof (wifi_ap_record_t));
    if (pstru_ap_records == NULL)
    {
        LOGE ("Failed to allocate %d-byte memory for AP records", sizeof (wifi_ap_record_t) * u16_num_ap);
        return WIFI_ERR;
    }

    /* Get AP list found in last scan */
    x_err = esp_wifi_scan_get_ap_records (&u16_num_ap, pstru_ap_records);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to get AP list found in last scan (%s)", esp_err_to_name (x_err));
        free (pstru_ap_records);
        return WIFI_ERR;
    }

    /* Allocate memory to store AP info list */
    WIFI_ap_info_t * pastru_ap_list = calloc (u16_num_ap, sizeof (WIFI_ap_info_t));
    if (pastru_ap_list == NULL)
    {
        LOGE ("Failed to allocate %d-byte memory for AP list", sizeof (WIFI_ap_info_t) * u16_num_ap);
        free (pstru_ap_records);
        return WIFI_ERR;
    }

    /* Construct items in the AP info list */
    uint16_t u16_ap_count = 0;
    for (uint16_t u16_idx = 0; u16_idx < u16_num_ap; u16_idx++)
    {
        /* There may be some access points with the same name, take only one of them */
        bool b_taken = false;
        for (uint16_t u16_taken_idx = 0; u16_taken_idx < u16_ap_count; u16_taken_idx++)
        {
            if (strncmp (pastru_ap_list[u16_taken_idx].stri_ssid,
                (char *)pstru_ap_records[u16_idx].ssid, WIFI_SSID_LEN) == 0)
            {
                b_taken = true;
                break;
            }
        }
        if (!b_taken)
        {
            memcpy (pastru_ap_list[u16_ap_count].au8_mac, pstru_ap_records[u16_idx].bssid, 6);
            strncpy (pastru_ap_list[u16_ap_count].stri_ssid, (char *)pstru_ap_records[u16_idx].ssid, WIFI_SSID_LEN);
            pastru_ap_list[u16_ap_count].stri_ssid[WIFI_SSID_LEN - 1] = '\0';
            pastru_ap_list[u16_ap_count].s8_rssi = pstru_ap_records[u16_idx].rssi;
            u16_ap_count++;
        }
    }

    /* Cleanup */
    free (pstru_ap_records);

    /* Done */
    *ppastru_ap_list = pastru_ap_list;
    *pu16_num_ap = u16_ap_count;
    return WIFI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets IP address information of wifi interface
**
** @note
**      If the wifi interface is in station mode, information of station interface will be returned. Otherwise, that
**      of soft access point interface will be returned.
**
** @param [out]
**      pstru_ip_info: IP address information
**
** @return
**      @arg    WIFI_OK
**      @arg    WIFI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFI_Get_IP_Info (WIFI_ip_info_t * pstru_ip_info)
{
    ASSERT_PARAM (g_b_initialized && (pstru_ip_info != NULL));

    /* Get current mode of the wifi interface */
    wifi_mode_t enm_mode;
    if (esp_wifi_get_mode (&enm_mode) != ESP_OK)
    {
        LOGE ("Failed to get current mode of the wifi interface");
        return WIFI_ERR;
    }

    /* Wifi interface corresponding with the wifi mode */
    esp_netif_t * px_wifi_if = (enm_mode == WIFI_MODE_STA) ? g_px_wifi_sta_if : g_px_wifi_sap_if;
    if (px_wifi_if == NULL)
    {
        LOGE ("Wifi interface is not up");
        return WIFI_ERR;
    }

    /* Get interface’s IP address information and DNS Server information */
    esp_netif_ip_info_t stru_ip_info;
    esp_netif_dns_info_t stru_dns_info;
    if ((esp_netif_get_ip_info (px_wifi_if, &stru_ip_info) == ESP_OK) &&
        (esp_netif_get_dns_info (px_wifi_if, ESP_NETIF_DNS_MAIN, &stru_dns_info) == ESP_OK))
    {
        ENDIAN_PUT32 (pstru_ip_info->au8_ip, stru_ip_info.ip.addr);
        ENDIAN_PUT32 (pstru_ip_info->au8_netmask, stru_ip_info.netmask.addr);
        ENDIAN_PUT32 (pstru_ip_info->au8_gateway, stru_ip_info.gw.addr);
        ENDIAN_PUT32 (pstru_ip_info->au8_dns, stru_dns_info.ip.u_addr.ip4.addr);
        return WIFI_OK;
    }

    return WIFI_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets MAC address of Wifi interface
**
** @param [out]
**      au8_mac: MAC address of ESP32's wifi interface
**
** @return
**      @arg    WIFI_OK
**      @arg    WIFI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFI_Get_Mac (uint8_t au8_mac[8])
{
    ASSERT_PARAM (g_b_initialized && (au8_mac != NULL));

    /* Get MAC address of Wifi interface */
    ESP_ERROR_CHECK (esp_wifi_get_mac (WIFI_IF_STA, au8_mac));

    return WIFI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets information of the access point that is being connected with
**
** @note
**      This function is applicable for station mode only
**
** @param [out]
**      pstru_ap_info: Structure wrapping information of an access point
**
** @return
**      @arg    WIFI_OK
**      @arg    WIFI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFI_Get_Ap_Info (WIFI_ap_info_t * pstru_ap_info)
{
    ASSERT_PARAM (g_b_initialized && (pstru_ap_info != NULL));

    /* Ensure that the wifi interface is in station mode */
    wifi_mode_t enm_mode;
    if ((esp_wifi_get_mode (&enm_mode) != ESP_OK) || (enm_mode != WIFI_MODE_STA))
    {
        LOGE ("The wifi interface is not in station mode");
        return WIFI_ERR;
    }

    /* Get information of the associated access point */
    wifi_ap_record_t stru_ap_record;
    if (esp_wifi_sta_get_ap_info (&stru_ap_record) == ESP_OK)
    {
        memcpy (pstru_ap_info->au8_mac, stru_ap_record.bssid, 6);
        strncpy (pstru_ap_info->stri_ssid, (char *)stru_ap_record.ssid, WIFI_SSID_LEN);
        pstru_ap_info->stri_ssid[WIFI_SSID_LEN - 1] = '\0';
        pstru_ap_info->s8_rssi = stru_ap_record.rssi;
        return WIFI_OK;
    }

    return WIFI_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of subcribed events
**
** @param [in]
**      pv_arg: Argument of the event fired
**
** @param [in]
**      x_evt_base: Group of the event fired
**
** @param [in]
**      s32_evt_id: ID of the event fired
**
** @param [in]
**      pv_evt_data: Context data of the event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_WIFI_Evt_Handler (void * pv_arg, esp_event_base_t x_evt_base, int32_t s32_evt_id, void * pv_evt_data)
{
    /* Process wifi events */
    if (x_evt_base == WIFI_EVENT)
    {
        switch (s32_evt_id)
        {
            /* Station starts */
            case WIFI_EVENT_STA_START:
            {
                LOGI ("WIFI_EVENT_STA_START");
                g_b_started = true;

                /* Connect the ESP32 WiFi station to the AP */
                esp_wifi_connect ();
                break;
            }

            /* Station stops */
            case WIFI_EVENT_STA_STOP:
            {
                LOGI ("WIFI_EVENT_STA_STOP");
                g_b_started = false;
                break;
            }

            /* Station connected to AP */
            case WIFI_EVENT_STA_CONNECTED:
            {
                LOGI ("WIFI_EVENT_STA_CONNECTED");

                /* Get status of DHCP client */
                esp_netif_dhcp_status_t enm_dhcpc_status;
                esp_netif_dhcpc_get_status (g_px_wifi_sta_if, &enm_dhcpc_status);

                /* Check if static IP address is used */
                if (g_b_static_addr_used)
                {
                    /* Stop DHCP client */
                    if (enm_dhcpc_status != ESP_NETIF_DHCP_STOPPED)
                    {
                        esp_netif_dhcpc_stop (g_px_wifi_sta_if);
                    }

                    /* Assign static IP address */
                    esp_netif_ip_info_t stru_ip_info;
                    IP4_ADDR (&stru_ip_info.ip, g_stru_static_addr.au8_ip[0], g_stru_static_addr.au8_ip[1],
                                                g_stru_static_addr.au8_ip[2], g_stru_static_addr.au8_ip[3]);
                    IP4_ADDR (&stru_ip_info.netmask, g_stru_static_addr.au8_netmask[0], g_stru_static_addr.au8_netmask[1],
                                                     g_stru_static_addr.au8_netmask[2], g_stru_static_addr.au8_netmask[3]);
                    IP4_ADDR (&stru_ip_info.gw, g_stru_static_addr.au8_gateway[0], g_stru_static_addr.au8_gateway[1],
                                                g_stru_static_addr.au8_gateway[2], g_stru_static_addr.au8_gateway[3]);
                    esp_netif_set_ip_info (g_px_wifi_sta_if, &stru_ip_info);

                    /* Set DNS Server information */
                    esp_netif_dns_info_t stru_dns_info;
                    stru_dns_info.ip.type = 0;
                    IP4_ADDR (&stru_dns_info.ip.u_addr.ip4, g_stru_static_addr.au8_dns[0], g_stru_static_addr.au8_dns[1],
                                                            g_stru_static_addr.au8_dns[2], g_stru_static_addr.au8_dns[3]);
                    esp_netif_set_dns_info (g_px_wifi_sta_if, ESP_NETIF_DNS_MAIN, &stru_dns_info);
                }
                else
                {
                    /* Start DHCP client */
                    if (enm_dhcpc_status != ESP_NETIF_DHCP_STARTED)
                    {
                        esp_netif_dhcpc_start (g_px_wifi_sta_if);
                    }
                }

                /* Invoke callback function */
                g_b_connected = true;
                if (g_pfnc_event_handler != NULL)
                {
                    g_pfnc_event_handler (WIFI_EVT_STA_CONNECTED);
                }
                break;
            }

            /* ESP32 station disconnected from AP */
            case WIFI_EVENT_STA_DISCONNECTED:
            {
                LOGI ("WIFI_EVENT_STA_DISCONNECTED");

                /* Invoke callback function */
                g_b_connected = false;
                if (g_pfnc_event_handler != NULL)
                {
                    g_pfnc_event_handler (WIFI_EVT_STA_DISCONNECTED);
                }
                break;
            }

            /* A client has been connected with the soft access point */
            case WIFI_EVENT_AP_STACONNECTED:
            {
                LOGI ("WIFI_EVENT_AP_STACONNECTED");

                /* Invoke callback function */
                if (g_pfnc_event_handler != NULL)
                {
                    g_pfnc_event_handler (WIFI_EVT_SAP_CONNECTED);
                }
                break;
            }

            /* A client has been disconnected with the soft access point */
            case WIFI_EVENT_AP_STADISCONNECTED:
            {
                LOGI ("WIFI_EVENT_AP_STADISCONNECTED");

                /* Invoke callback function */
                if (g_pfnc_event_handler != NULL)
                {
                    g_pfnc_event_handler (WIFI_EVT_SAP_DISCONNECTED);
                }
                break;
            }
        }
    }

    /* Process IP events */
    else if (x_evt_base == IP_EVENT)
    {
        if (s32_evt_id == IP_EVENT_STA_GOT_IP)
        {
            /* DHCP client successfully gets the IPV4 address from the DHCP server */
            LOGD ("Event IP_EVENT.IP_EVENT_STA_GOT_IP occurs");

            /* Invoke callback function */
            if (g_pfnc_event_handler != NULL)
            {
                g_pfnc_event_handler (WIFI_EVT_STA_IP_OBTAINED);
            }
        }
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
