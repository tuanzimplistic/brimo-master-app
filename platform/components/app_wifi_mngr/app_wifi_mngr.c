/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : app_wifi_mngr.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Mar 12
**  @brief      : Implementation of App_Wifi_Mngr module
**  @namespace  : WIFIMN
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Wifi_Mngr
** @brief       Manages network connection over Wifi and provides API for other module to make use of the network
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "app_wifi_mngr.h"              /* Public header of this module */
#include "app_wifi_mngr_ext.h"          /* Use WIFIMN_BACKUP_AP_TABLE */
#include "srvc_wifi.h"                  /* Use Wifi service */
#include "app_mqtt_mngr.h"              /* Use MQTT Manager module */
#include "srvc_param.h"                 /* Use Parameter service */

#include <string.h>                     /* Use memset(), strncpy() */
#include <stdio.h>                      /* Use sscanf() */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/task.h"              /* Use FreeRTOS task */
#include "freertos/event_groups.h"      /* Use FreeRTOS event group */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  ID of the CPU that App_Wifi_Mngr task runs on */
#define WIFIMN_TASK_CPU_ID              1

/** @brief  Stack size (in bytes) of App_Wifi_Mngr task */
#define WIFIMN_TASK_STACK_SIZE          4096

/** @brief  Priority of App_Wifi_Mngr task */
#define WIFIMN_TASK_PRIORITY            (tskIDLE_PRIORITY + 0)

/** @brief  Cycle in milliseconds of App_Wifi_Mngr task */
#define WIFIMN_TASK_PERIOD_MS           100

/** @brief  Wifi start scan event */
#define WIFIMN_START_SCAN_EVENT         (BIT0)

/** @brief  Number of known wifi access points */
#define WIFIMN_NUM_KNOWN_AP             (sizeof (g_astru_ap) / sizeof (g_astru_ap[0]))

/** @brief  Number of attempts connecting to a Wifi access point */
#define WIFIMN_NUM_CONNECT_ATTEMPTS     3

/** @brief  Index in g_astru_ap[] of test station Wifi access point */
#define WIFIMN_TEST_STATION_AP_IDX      0

/** @brief  Index in g_astru_ap[] of user configurable Wifi access point */
#define WIFIMN_USER_AP_IDX              1

/** @brief  Wifi scanning states */
typedef enum
{
    WIFIMN_SCANNING_IDLE,               //!< No scanning is performed
    WIFIMN_SCANNING_IN_PROGRESS,        //!< Wifi scanning is in progress
    WIFIMN_SCANNING_DONE_OK,            //!< Wifi scanning has been done successfully
    WIFIMN_SCANNING_DONE_FAILED,        //!< Wifi scanning failed

} WIFIMN_scan_state_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "App_Wifi_Mngr";

/** @brief  Structure that will hold the TCB of the task being created */
static StaticTask_t g_x_task_buffer;

/** @brief  Buffer that the task being created will use as its stack */
static StackType_t g_x_task_stack [WIFIMN_TASK_STACK_SIZE];

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  FreeRTOS event group */
static EventGroupHandle_t g_x_event_group;

/**
** @brief   List of known wifi access points
** @note    The first item in the list is user configurable.
**          The remaining items are backup access points which are used if the first access point cannot be connected.
*/
static WIFIMN_cred_t g_astru_ap [] =
{
    /* Test station access point */
    [WIFIMN_TEST_STATION_AP_IDX] =
    {
        .stri_ssid = CONFIG_TEST_STATION_WIFI_SSID,
        .stri_psw  = CONFIG_TEST_STATION_WIFI_PASSWORD,
    },

    /* User configurable access point */
    [WIFIMN_USER_AP_IDX] =
    {
        .stri_ssid = "dummy_ssid",
        .stri_psw  = "dummy_psw",
    },

    /* Backup access points */
    WIFIMN_BACKUP_AP_TABLE (WIFIMN_EXPAND_BACKUP_TABLE_AS_STRUCT_INIT)
};

/** @brief  Index in g_astru_ap[] of the access point that is currently used for connecting */
static uint8_t g_u8_current_ap_idx = 0;

/** @brief  Number of attempts connecting to an access point */
static uint8_t g_u8_retries = 0;

/** @brief  Indicates if ESP32 is connected with an access point */
static bool g_b_wifi_connected = false;

/** @brief  Indicates if Wifi is forced to disconnect */
static bool g_b_wifi_disconnect_forced = false;

/** @brief  Wifi scanning state */
static WIFIMN_scan_state_t g_enm_scanning_state = WIFIMN_SCANNING_IDLE;

/** @brief  Pointer to the access point list populated by wifi scanning */
static WIFIMN_ap_t * g_pastru_ap_list = NULL;

/** @brief  Number of entries in the access point list g_pastru_ap_list */
static uint16_t g_u16_ap_list_count = 0;

/** @brief  Indicates whether Wifi manager is running in test station mode */
#ifdef CONFIG_TEST_STATION_BUILD_ENABLED
 static bool g_b_test_station_mode = true;
#else
 static bool g_b_test_station_mode = false;
#endif

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static void v_WIFIMN_Main_Task (void * pv_param);
static void v_WIFIMN_Do_Scanning (void);
static void v_WIFIMN_Event_Handler_Normal_Station (WIFI_event_t enm_event);
static void v_WIFIMN_Event_Handler_Test_Station (WIFI_event_t enm_event);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes App_Wifi_Mngr module
**
** @return
**      @arg    WIFIMN_OK
**      @arg    WIFIMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFIMN_Init (void)
{
    LOGD ("Initializing App_Wifi_Mngr module");

    /* Initializes Srvc_Wifi module */
    if (s8_WIFI_Init () != WIFI_OK)
    {
        return WIFIMN_ERR;
    }

    /* Get SSID and password of the user configurable wifi access point */
    char * pstri_ssid;
    if (s8_PARAM_Get_String (PARAM_WIFI_SSID, &pstri_ssid) == PARAM_OK)
    {
        strncpy (g_astru_ap[WIFIMN_USER_AP_IDX].stri_ssid, pstri_ssid, WIFIMN_SSID_LEN);
        g_astru_ap[WIFIMN_USER_AP_IDX].stri_ssid [WIFIMN_SSID_LEN - 1] = 0;
        free (pstri_ssid);

        /* Get password of the selected wifi access point */
        char * pstri_password;
        if (s8_PARAM_Get_String (PARAM_WIFI_PSW, &pstri_password) == PARAM_OK)
        {
            strncpy (g_astru_ap[WIFIMN_USER_AP_IDX].stri_psw, pstri_password, WIFIMN_PSW_LEN);
            g_astru_ap[WIFIMN_USER_AP_IDX].stri_psw [WIFIMN_PSW_LEN - 1] = 0;
            free (pstri_password);
        }
    }

    /* Check if we are in test station mode */
    if (g_b_test_station_mode)
    {
        /* Register callback function handling WiFi events */
        s8_WIFI_Register_Event_Handler (v_WIFIMN_Event_Handler_Test_Station);

        /* Try to connect to test access point with static IP address assignment */
        int32_t as32_ip[4];
        int32_t as32_nm[4];
        int32_t as32_gw[4];
        int32_t as32_dns[4];
        sscanf (CONFIG_TEST_STATION_IP_ADDR, "%d.%d.%d.%d", &as32_ip[0], &as32_ip[1], &as32_ip[2], &as32_ip[3]);
        sscanf (CONFIG_TEST_STATION_NETMASK, "%d.%d.%d.%d", &as32_nm[0], &as32_nm[1], &as32_nm[2], &as32_nm[3]);
        sscanf (CONFIG_TEST_STATION_GATEWAY, "%d.%d.%d.%d", &as32_gw[0], &as32_gw[1], &as32_gw[2], &as32_gw[3]);
        sscanf (CONFIG_TEST_STATION_DNS, "%d.%d.%d.%d", &as32_dns[0], &as32_dns[1], &as32_dns[2], &as32_dns[3]);

        WIFI_ip_info_t stru_ip_info;
        for (uint8_t u8_idx = 0; u8_idx < 4; u8_idx++)
        {
            stru_ip_info.au8_ip[u8_idx] = (uint8_t)as32_ip[u8_idx];
            stru_ip_info.au8_netmask[u8_idx] = (uint8_t)as32_nm[u8_idx];
            stru_ip_info.au8_gateway[u8_idx] = (uint8_t)as32_gw[u8_idx];
            stru_ip_info.au8_dns[u8_idx] = (uint8_t)as32_dns[u8_idx];
        }

        g_u8_current_ap_idx = WIFIMN_TEST_STATION_AP_IDX;
        g_u8_retries = 0;
        s8_WIFI_Connect (g_astru_ap[g_u8_current_ap_idx].stri_ssid,
                         g_astru_ap[g_u8_current_ap_idx].stri_psw, &stru_ip_info);
    }
    else
    {
        /* Register callback function handling WiFi events */
        s8_WIFI_Register_Event_Handler (v_WIFIMN_Event_Handler_Normal_Station);

        /* Try to connect to user's wifi access point with dynamic IP address assignment */
        g_u8_current_ap_idx = WIFIMN_USER_AP_IDX;
        g_u8_retries = 0;
        s8_WIFI_Connect (g_astru_ap[g_u8_current_ap_idx].stri_ssid, g_astru_ap[g_u8_current_ap_idx].stri_psw, NULL);
    }

    /* Create FreeRTOS event group */
    g_x_event_group = xEventGroupCreate ();

    /* Create task running this module */
    xTaskCreateStaticPinnedToCore ( v_WIFIMN_Main_Task,         /* Function that implements the task */
                                    "App_Wifi_Mngr",            /* Text name for the task */
                                    WIFIMN_TASK_STACK_SIZE,     /* Stack size in bytes, not words */
                                    NULL,                       /* Parameter passed into the task */
                                    WIFIMN_TASK_PRIORITY,       /* Priority at which the task is created */
                                    g_x_task_stack,             /* Array to use as the task's stack */
                                    &g_x_task_buffer,           /* Variable to hold the task's data structure */
                                    WIFIMN_TASK_CPU_ID);        /* ID of the CPU that App_Wifi_Mngr task runs on */

    /* Done */
    LOGD ("Initialization of App_Wifi_Mngr module is done");
    g_b_initialized = true;
    return WIFIMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets information of the user configurable wifi access point
**
** @param [out]
**      ppstru_ap: Information of the access point
**
** @return
**      @arg    WIFIMN_OK
**      @arg    WIFIMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFIMN_Get_User_Ap (WIFIMN_cred_t ** ppstru_ap)
{
    ASSERT_PARAM (g_b_initialized && (ppstru_ap != NULL));

    /* User configurable wifi access point is the first access point in known wifi AP list */
    *ppstru_ap = &g_astru_ap[WIFIMN_USER_AP_IDX];

    return WIFIMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets information of the currently selected wifi access point and checks the connectivity with it
**
** @param [out]
**      ppstru_ap: Information of the access point. This argument can be NULL.
**
** @param [out]
**      pb_connected: Indicates if ESP32 is connected with the selected access point. This argument can be NULL.
**
** @return
**      @arg    WIFIMN_OK
**      @arg    WIFIMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFIMN_Get_Selected_Ap (WIFIMN_cred_t ** ppstru_ap, bool * pb_connected)
{
    ASSERT_PARAM (g_b_initialized);

    /* Information of the access point */
    if (ppstru_ap != NULL)
    {
        *ppstru_ap = &g_astru_ap[g_u8_current_ap_idx];
    }

    /* Connection status */
    if (pb_connected != NULL)
    {
        *pb_connected = g_b_wifi_connected;
    }

    return WIFIMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets number of backup access points
**
** @param [out]
**      pu8_num_backup_ap: number of backup access points
**
** @return
**      @arg    WIFIMN_OK
**      @arg    WIFIMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFIMN_Get_Num_Backup_Ap (uint8_t * pu8_num_backup_ap)
{
    ASSERT_PARAM (g_b_initialized && (pu8_num_backup_ap != NULL));

    /* Don't count user access point and test station access point */
    *pu8_num_backup_ap = WIFIMN_NUM_KNOWN_AP - 2;

    return WIFIMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Forces to connect with an user access point.
**
** @note
**      This function also stores information of the given access point into non-volatile flash
**
** @param [out]
**      pstru_ap: Information of the access point to connect
**
** @return
**      @arg    WIFIMN_OK
**      @arg    WIFIMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFIMN_Connect (const WIFIMN_cred_t * pstru_ap)
{
    ASSERT_PARAM (g_b_initialized && (pstru_ap != NULL));

    /* Store information of the given user access point */
    g_astru_ap[WIFIMN_USER_AP_IDX] = *pstru_ap;
    if (s8_PARAM_Set_String (PARAM_WIFI_SSID, g_astru_ap[WIFIMN_USER_AP_IDX].stri_ssid) != PARAM_OK)
    {
        LOGE ("Failed to save wifi SSID to non-volatile storage");
    }
    if (s8_PARAM_Set_String (PARAM_WIFI_PSW, g_astru_ap[WIFIMN_USER_AP_IDX].stri_psw) != PARAM_OK)
    {
        LOGE ("Failed to save wifi password to non-volatile storage");
    }

    /* If we are in test station mode, switch back to normal mode */
    if (g_b_test_station_mode)
    {
        s8_WIFI_Register_Event_Handler (v_WIFIMN_Event_Handler_Normal_Station);
        g_b_test_station_mode = false;
    }

    /* Try to connects to the given wifi access point */
    g_b_wifi_disconnect_forced = false;
    g_u8_current_ap_idx = WIFIMN_USER_AP_IDX;
    g_u8_retries = 0;
    s8_WIFI_Connect (g_astru_ap[g_u8_current_ap_idx].stri_ssid, g_astru_ap[g_u8_current_ap_idx].stri_psw, NULL);

    return WIFIMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Forces to disconnect from the current access point
**
** @return
**      @arg    WIFIMN_OK
**      @arg    WIFIMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFIMN_Disconnect (void)
{
    ASSERT_PARAM (g_b_initialized);

    /* Disconnect from Wifi */
    g_b_wifi_disconnect_forced = true;
    s8_WIFI_Disconnect ();

    return WIFIMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Starts scanning of all currently available access points
**
** @return
**      @arg    WIFIMN_OK
**      @arg    WIFIMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFIMN_Start_Scan (void)
{
    ASSERT_PARAM (g_b_initialized);

    /* Request wifi scanning */
    g_enm_scanning_state = WIFIMN_SCANNING_IN_PROGRESS;
    xEventGroupSetBits (g_x_event_group, WIFIMN_START_SCAN_EVENT);

    return WIFIMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets list of available access points in last scan
**
** @param [out]
**      ppastru_ap_list: Pointer to the array containing list of wifi access points.
**                       This is NULL if there is no access point found.
**
** @param [out]
**      pu16_num_ap: Number of items stored in ppastru_ap_list
**                   This is 0 if there is no access point found.
**
** @return
**      @arg    WIFIMN_OK
**      @arg    WIFIMN_ERR
**      @arg    WIFIMN_ERR_BUSY
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_WIFIMN_Get_Scan_Ap_List (WIFIMN_ap_t ** ppastru_ap_list, uint16_t * pu16_num_ap)
{
    ASSERT_PARAM (g_b_initialized && (ppastru_ap_list != NULL) && (pu16_num_ap != NULL));

    /* Check wifi scanning state */
    if (g_enm_scanning_state == WIFIMN_SCANNING_IN_PROGRESS)
    {
        return WIFIMN_ERR_BUSY;
    }
    else if (g_enm_scanning_state != WIFIMN_SCANNING_DONE_OK)
    {
        return WIFIMN_ERR;
    }

    /* Wifi scanning has done, get result */
    *ppastru_ap_list = g_pastru_ap_list;
    *pu16_num_ap = g_u16_ap_list_count;

    return WIFIMN_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Task running App_Wifi_Mngr module
**
** @param [in]
**      pv_param: Parameter passed into the task
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_WIFIMN_Main_Task (void * pv_param)
{
    if (g_b_test_station_mode)
    {
        LOGI ("**** App_Wifi_Mngr task started in Test Station mode ****");
    }
    else
    {
        LOGD ("App_Wifi_Mngr task started");
    }

    /* Endless loop of the task */
    while (true)
    {
        /* Waiting for task tick or a FreeRTOS event occurs */
        EventBits_t x_event_bits = xEventGroupWaitBits (g_x_event_group, WIFIMN_START_SCAN_EVENT, pdTRUE, pdFALSE,
                                                        pdMS_TO_TICKS (WIFIMN_TASK_PERIOD_MS));

        /* If requested to start wifi scanning */
        if (x_event_bits & WIFIMN_START_SCAN_EVENT)
        {
            v_WIFIMN_Do_Scanning ();
        }

        /* Display remaining stack space every 30s */
        // PRINT_STACK_USAGE (30000);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Performs wifi scanning
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_WIFIMN_Do_Scanning (void)
{
    WIFI_ap_info_t *    pastru_ap_list = NULL;
    uint16_t            u16_num_ap = 0;
    WIFIMN_scan_state_t enm_scanning_state = WIFIMN_SCANNING_DONE_OK;
    bool                b_ap_found = true;

    /* Stop connecting if not being connected to any access point */
    if (!g_b_wifi_connected)
    {
        s8_WIFI_Disconnect ();
    }

    /* Clean current access point list */
    g_u16_ap_list_count = 0;
    if (g_pastru_ap_list != NULL)
    {
        free (g_pastru_ap_list);
        g_pastru_ap_list = NULL;
    }

    /* Scan for access point list */
    if (s8_WIFI_Scan_Ap_List (&pastru_ap_list, &u16_num_ap) != WIFI_OK)
    {
        enm_scanning_state = WIFIMN_SCANNING_DONE_FAILED;
        b_ap_found = false;
    }
    else if ((pastru_ap_list == NULL) || (u16_num_ap == 0))
    {
        /* No access point found */
        b_ap_found = false;
    }

    /* Get access point information */
    if (b_ap_found)
    {
        LOGI ("List of access points found:");
        for (uint8_t u8_idx = 0; u8_idx < u16_num_ap; u8_idx++)
        {
            LOGI ("%d) %s", u8_idx, pastru_ap_list[u8_idx].stri_ssid);
        }

        /* Allocate buffer for access point list */
        g_pastru_ap_list = calloc (u16_num_ap, sizeof (WIFIMN_ap_t));
        if (g_pastru_ap_list == NULL)
        {
            LOGE ("Failed to allocate %d-byte buffer for wifi AP list", u16_num_ap * sizeof (WIFIMN_ap_t));
            enm_scanning_state = WIFIMN_SCANNING_DONE_FAILED;
        }
        else
        {
            g_u16_ap_list_count = u16_num_ap;
            for (uint8_t u8_idx = 0; u8_idx < u16_num_ap; u8_idx++)
            {
                strncpy (g_pastru_ap_list [u8_idx].stri_ssid, pastru_ap_list [u8_idx].stri_ssid,
                         sizeof (g_pastru_ap_list [u8_idx].stri_ssid));
                g_pastru_ap_list [u8_idx].stri_ssid [sizeof (g_pastru_ap_list [u8_idx].stri_ssid) - 1] = 0;
            }
        }
    }

    /* Retry connecting to the selected Wifi AP if not being connected */
    if ((!g_b_wifi_connected) && (!g_b_wifi_disconnect_forced))
    {
        s8_WIFI_Reconnect ();
    }

    /* Cleanup */
    if (pastru_ap_list != NULL)
    {
        free (pastru_ap_list);
    }

    g_enm_scanning_state = enm_scanning_state;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Wifi event handler used for normal station
**
** @param [in]
**      enm_event: Event fired
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_WIFIMN_Event_Handler_Normal_Station (WIFI_event_t enm_event)
{
    /* Process wifi events */
    switch (enm_event)
    {
        /* Connected to the AP */
        case WIFI_EVT_STA_CONNECTED:
        {
            LOGI ("Connected to wifi access point %s", g_astru_ap[g_u8_current_ap_idx].stri_ssid);
            g_u8_retries = 0;
            break;
        }

        /* IP address obtained from DHCP server */
        case WIFI_EVT_STA_IP_OBTAINED:
        {
            g_b_wifi_connected = true;

            /* Show wifi information */
            WIFI_ip_info_t stru_ip_info;
            s8_WIFI_Get_IP_Info (&stru_ip_info);
            LOGI ("IP address obtained from DHCP server:");
            LOGI ("+ IP: %d.%d.%d.%d", stru_ip_info.au8_ip[0], stru_ip_info.au8_ip[1],
                                       stru_ip_info.au8_ip[2], stru_ip_info.au8_ip[3]);
            LOGI ("+ Netmask: %d.%d.%d.%d", stru_ip_info.au8_netmask[0], stru_ip_info.au8_netmask[1],
                                            stru_ip_info.au8_netmask[2], stru_ip_info.au8_netmask[3]);
            LOGI ("+ Gateway: %d.%d.%d.%d", stru_ip_info.au8_gateway[0], stru_ip_info.au8_gateway[1],
                                            stru_ip_info.au8_gateway[2], stru_ip_info.au8_gateway[3]);
            LOGI ("+ DNS: %d.%d.%d.%d", stru_ip_info.au8_dns[0], stru_ip_info.au8_dns[1],
                                        stru_ip_info.au8_dns[2], stru_ip_info.au8_dns[3]);

            /* Start MQTT Manager, add a small delay to ensure that other modules have already started */
            vTaskDelay (pdMS_TO_TICKS (100));
            LOGI ("Start MQTT interface");
            s8_MQTTMN_Init ();
            break;
        }

        /* Disconnected from the AP */
        case WIFI_EVT_STA_DISCONNECTED:
        {
            LOGW ("Disconnected from wifi access point %s", g_astru_ap[g_u8_current_ap_idx].stri_ssid);
            if (g_b_wifi_connected)
            {
                g_b_wifi_connected = false;
            }

            /* Attempt to connect if disconnection is not forced */
            if (!g_b_wifi_disconnect_forced)
            {
                if (g_u8_retries < WIFIMN_NUM_CONNECT_ATTEMPTS)
                {
                    s8_WIFI_Reconnect ();
                }
                else
                {
                    g_u8_retries = 0;
                    /* Don't use test station access point in normal mode */
                    do
                    {
                        g_u8_current_ap_idx++;
                        if (g_u8_current_ap_idx >= WIFIMN_NUM_KNOWN_AP)
                        {
                            g_u8_current_ap_idx = 0;
                        }
                    }
                    while (g_u8_current_ap_idx == WIFIMN_TEST_STATION_AP_IDX);

                    s8_WIFI_Connect (g_astru_ap[g_u8_current_ap_idx].stri_ssid,
                                     g_astru_ap[g_u8_current_ap_idx].stri_psw, NULL);
                }

                g_u8_retries++;
                LOGI ("Attempt %d to connect to wifi access point %s", g_u8_retries,
                      g_astru_ap[g_u8_current_ap_idx].stri_ssid);
            }
            break;
        }

        /* Unsupported events */
        default:
            break;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Wifi event handler used for test station
**
** @param [in]
**      enm_event: Event fired
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_WIFIMN_Event_Handler_Test_Station (WIFI_event_t enm_event)
{
    /* Process wifi events */
    switch (enm_event)
    {
        /* Connected to the test access point */
        case WIFI_EVT_STA_CONNECTED:
        {
            LOGI ("Connected to wifi access point %s", g_astru_ap[g_u8_current_ap_idx].stri_ssid);
            g_u8_retries = 0;
            break;
        }

        /* Static IP address assigned */
        case WIFI_EVT_STA_IP_OBTAINED:
        {
            g_b_wifi_connected = true;

            /* Show wifi information */
            WIFI_ip_info_t stru_ip_info;
            s8_WIFI_Get_IP_Info (&stru_ip_info);
            LOGI ("Static IP address assigned:");
            LOGI ("+ IP: %d.%d.%d.%d", stru_ip_info.au8_ip[0], stru_ip_info.au8_ip[1],
                                       stru_ip_info.au8_ip[2], stru_ip_info.au8_ip[3]);
            LOGI ("+ Netmask: %d.%d.%d.%d", stru_ip_info.au8_netmask[0], stru_ip_info.au8_netmask[1],
                                            stru_ip_info.au8_netmask[2], stru_ip_info.au8_netmask[3]);
            LOGI ("+ Gateway: %d.%d.%d.%d", stru_ip_info.au8_gateway[0], stru_ip_info.au8_gateway[1],
                                            stru_ip_info.au8_gateway[2], stru_ip_info.au8_gateway[3]);
            LOGI ("+ DNS: %d.%d.%d.%d", stru_ip_info.au8_dns[0], stru_ip_info.au8_dns[1],
                                        stru_ip_info.au8_dns[2], stru_ip_info.au8_dns[3]);

            /* Start MQTT Manager, add a small delay to ensure that other modules have already started */
            vTaskDelay (pdMS_TO_TICKS (500));
            LOGI ("Start MQTT interface");
            s8_MQTTMN_Init ();

            /* Wifi Manager can work normally now */
            s8_WIFI_Register_Event_Handler (v_WIFIMN_Event_Handler_Normal_Station);
            g_b_test_station_mode = false;
            break;
        }

        /* Disconnected from the test access point */
        case WIFI_EVT_STA_DISCONNECTED:
        {
            LOGW ("Failed to connect to wifi access point %s", g_astru_ap[g_u8_current_ap_idx].stri_ssid);

            if (g_u8_retries < CONFIG_TEST_STATION_WIFI_RETRIES)
            {
                s8_WIFI_Reconnect ();
            }
            else
            {
                /* Back to normal mode */
                s8_WIFI_Register_Event_Handler (v_WIFIMN_Event_Handler_Normal_Station);
                g_b_test_station_mode = false;

                /* Try to connect to user's wifi access point with dynamic IP address assignment */
                g_u8_current_ap_idx = WIFIMN_USER_AP_IDX;
                g_u8_retries = 0;
                s8_WIFI_Connect (g_astru_ap[g_u8_current_ap_idx].stri_ssid,
                                 g_astru_ap[g_u8_current_ap_idx].stri_psw, NULL);
            }

            g_u8_retries++;
            LOGI ("Attempt %d to connect to wifi access point %s", g_u8_retries,
                  g_astru_ap[g_u8_current_ap_idx].stri_ssid);
            break;
        }

        /* Unsupported events */
        default:
            break;
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
