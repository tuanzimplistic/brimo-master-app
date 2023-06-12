/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : app_main.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Mar 7
**  @brief      : Implementation of App_Main module
**  @namespace  : MAIN
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Main
** @brief       App_Main module is the entry point of Itor3 firmware. It initializes and starts other modules of the
**              firmware.
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "app_main.h"               /* Public header of this module */
#include "app_gui_mngr.h"           /* Use GUI Manager module */
#include "app_wifi_mngr.h"          /* Use Wifi Manager module */
#include "app_ota_mngr.h"           /* Use OTA update manager */
#include "srvc_micropy.h"           /* Use MicroPy service */
#include "srvc_param.h"             /* Use parameter service */
#include "esp_event.h"              /* Use ESP event APIs */
#include "mbzpl_req_m.h"            /* Use Modbus communication */
#include "srvc_recovery.h"          /* Restore cooking data from power interruption */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "App_Main";

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static void v_MAIN_Init (void);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Entry point of Itor3 firmware
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void app_main (void)
{
#ifdef CONFIG_TEST_STATION_BUILD_ENABLED
    ESP_LOGI (TAG, "**** Itor3 application started in Test Station mode ****");
#else
    ESP_LOGI (TAG, "Itor3 application started");
#endif

    /* Initialize modules */
    v_MAIN_Init ();
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initilizes modules for the firmware to work
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MAIN_Init (void)
{
    /* Initialize Parameter service */
    s8_PARAM_Init ();

    /* Initialize Recovery service, which allows cooking data to be restored from a power interruption */
    enm_RCVR_Init ();

    /* Create default event loop */
    ESP_ERROR_CHECK (esp_event_loop_create_default ());

    /* Initialize MicroPython service */
    s8_MP_Init ();

    /* Initialize Mobus */
    MAL_REQ_init ();

    /* Initialize Wifi Manager */
    s8_WIFIMN_Init ();

    /* Initialize GUI Manager, must be initialized after wifi manager */
    s8_GUI_Init ();
    
    /* Initialize OTA Manager, should be the last to be initialized */
    s8_OTAMN_Init ();
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
