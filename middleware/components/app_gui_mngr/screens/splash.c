/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : splash.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 May 24
**  @brief      : Implementation of Splash screen
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       Implements Splash screen
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "screen_common.h"              /* Common header of all screens */
#include "app_wifi_mngr.h"              /* Use Wifi Manager module */
#include "srvc_fwu_esp32.h"             /* Get firmware version of master firmware */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Minimum time (milliseconds) in splash screen before switching to another screen */
#define GUI_MIN_SPLASH_SCREEN_TIME      3000

/** @brief  Cycle (in milliseconds) polling GUI data */
#define GUI_REFRESH_DATA_CYCLE          50

/** @brief  Maximum length in bytes of cooking script information */
#define GUI_SCRIPT_INFO_LEN             32

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static int8_t s8_GUI_Start_Splash_Screen (void);
static int8_t s8_GUI_Stop_Splash_Screen (void);
static int8_t s8_GUI_Run_Splash_Screen (void);
static void v_GUI_Create_No_Wifi_Msgbox (void);
static void v_GUI_Msgbox_No_Wifi_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "App_Gui_Mngr";

/** @brief  Indicates if this screen has been initialized */
static bool g_b_initialized = false;

/** @brief  Screen information */
static GUI_screen_t g_stru_screen =
{
    .pstru_prev         = NULL,
    .pstru_next         = NULL,
    .px_lv_screen       = NULL,
    .pstri_name         = "Splash",
    .px_icon            = NULL,
    .pfnc_start         = s8_GUI_Start_Splash_Screen,
    .pfnc_stop          = s8_GUI_Stop_Splash_Screen,
    .pfnc_run           = s8_GUI_Run_Splash_Screen,
    .enm_result         = GUI_SCREEN_RESULT_NONE,
};

/** @brief  Progress bar of splash screen */
static lv_obj_t * g_px_bar_progress;

/** @brief  Working progress in percents */
static uint8_t g_u8_working_progress;

/** @brief  Message box displaying no wifi connection message */
static lv_obj_t * g_px_msgbox_no_wifi = NULL;

/** @brief  Label of firmware information */
static lv_obj_t * g_px_lbl_fw_info;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets structure wrapping splash screen
**
** @param [out]
**      ppstru_screen: Pointer to the structure wrapping the screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Get_Splash_Screen (GUI_screen_t ** ppstru_screen)
{
    ASSERT_PARAM (ppstru_screen != NULL);

    /* If this screen has not been initialized, initialize it */
    if (!g_b_initialized)
    {
        lv_obj_t * px_screen = lv_obj_create (NULL, NULL);

#ifdef CONFIG_TEST_STATION_BUILD_ENABLED
        /* Test station build info */
        lv_obj_t * px_lbl_test_station =  lv_label_create (px_screen, NULL);
        lv_label_set_text (px_lbl_test_station, "Test station build");

        static lv_style_t x_style_test_station;
        lv_style_init (&x_style_test_station);
        lv_style_set_text_font (&x_style_test_station, LV_STATE_DEFAULT, &lv_font_montserrat_14);
        lv_style_set_text_color (&x_style_test_station, LV_STATE_DEFAULT, LV_COLOR_RED);
        lv_obj_add_style (px_lbl_test_station, LV_LABEL_PART_MAIN, &x_style_test_station);
        lv_obj_align (px_lbl_test_station, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
#endif

        /* "rotimatic" logo with 'o' in orange */
        lv_obj_t * px_lbl_logo = lv_label_create (px_screen, NULL);
        lv_label_set_recolor (px_lbl_logo, true);
        lv_label_set_text (px_lbl_logo, "r#FFA500 o##000000 timatic#");

        static lv_style_t x_style_logo;
        lv_style_init (&x_style_logo);
        lv_style_set_text_font (&x_style_logo, LV_STATE_DEFAULT, &lv_font_montserrat_48);
        lv_obj_add_style (px_lbl_logo, LV_LABEL_PART_MAIN, &x_style_logo);
        lv_obj_align (px_lbl_logo, NULL, LV_ALIGN_CENTER, 0, -30);

        /* Working progress bar */
        g_px_bar_progress = lv_bar_create (px_screen, NULL);
        lv_obj_set_size (g_px_bar_progress, 200, 20);
        lv_obj_align (g_px_bar_progress, px_lbl_logo, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_bar_set_range (g_px_bar_progress, 0, 100);

        /* Work description */
        lv_obj_t * px_lbl_desc =  lv_label_create (g_px_bar_progress, NULL);
        lv_label_set_text (px_lbl_desc, "Connecting to wifi");

        static lv_style_t x_style_desc;
        lv_style_init (&x_style_desc);
        lv_style_set_text_font (&x_style_desc, LV_STATE_DEFAULT, &lv_font_montserrat_14);
        lv_style_set_text_color (&x_style_desc, LV_STATE_DEFAULT, LV_COLOR_BLUE);
        lv_obj_add_style (px_lbl_desc, LV_LABEL_PART_MAIN, &x_style_desc);
        lv_obj_align (px_lbl_desc, NULL, LV_ALIGN_CENTER, 0, 0);

        /* Style of extra information labels at the footer */
        static lv_style_t x_style_footer;
        lv_style_init (&x_style_footer);
        lv_style_set_text_font (&x_style_footer, LV_STATE_DEFAULT, &lv_font_montserrat_14);
        lv_style_set_text_color (&x_style_footer, LV_STATE_DEFAULT, LV_COLOR_GRAY);

        /* Firmware version and cooking script information */
        char stri_script_info[GUI_SCRIPT_INFO_LEN] = "";
        uint16_t u16_script_info_len = sizeof (stri_script_info);
        s8_GUI_Get_Data (GUI_DATA_SCRIPT_BRIEF_INFO, stri_script_info, &u16_script_info_len);

        FWUESP_fw_desc_t stru_fw_desc;
        s8_FWUESP_Get_Fw_Descriptor (&stru_fw_desc);

        g_px_lbl_fw_info = lv_label_create (px_screen, NULL);
        lv_label_set_text_fmt (g_px_lbl_fw_info, "Platform v%s - %s", stru_fw_desc.pstri_ver, stri_script_info);
        lv_obj_add_style (g_px_lbl_fw_info, LV_LABEL_PART_MAIN, &x_style_footer);
        lv_obj_align (g_px_lbl_fw_info, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -30);

        /* Footer text "Designed by Zimplistic" */
        lv_obj_t * px_lbl_footer =  lv_label_create (px_screen, NULL);
        lv_label_set_text (px_lbl_footer, "Designed by Zimplistic");
        lv_obj_add_style (px_lbl_footer, LV_LABEL_PART_MAIN, &x_style_footer);
        lv_obj_align (px_lbl_footer, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -10);

        g_stru_screen.px_lv_screen = px_screen;
        g_b_initialized = true;
    }

    *ppstru_screen = &g_stru_screen;
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Starts Splash screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Start_Splash_Screen (void)
{
    LOGD ("Splash screen started");
    g_stru_screen.enm_result = GUI_SCREEN_RESULT_NONE;

    /* Initialize working progress */
    g_u8_working_progress = 100;
    lv_bar_set_value (g_px_bar_progress, g_u8_working_progress, LV_ANIM_OFF);

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Stops Splash screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Stop_Splash_Screen (void)
{
    LOGD ("Splash screen stopped");
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs Splash screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Run_Splash_Screen (void)
{
    static uint32_t u32_run_timer = 0;
    static uint16_t u16_time_of_one_percent;
    static uint8_t u8_percents_to_switch_screen;

    /*
    ** Determine time (in milliseconds) necessary to attempt to connect to all known wifi access points:
    **      Maximum connect time = time to connect to user access point +
    **                             (time to connect to back up access point * number of backup access point)
    ** This maximum connection time is divided into 100 intervals corresponding with 100%
    */
    if (u32_run_timer == 0)
    {
        uint8_t u8_num_backup_ap;
        s8_WIFIMN_Get_Num_Backup_Ap (&u8_num_backup_ap);
        uint32_t u32_max_time = 20000 + (10000 * u8_num_backup_ap);

#ifdef CONFIG_TEST_STATION_BUILD_ENABLED
        u32_max_time += CONFIG_TEST_STATION_WIFI_RETRIES * 3000;
#endif

        u16_time_of_one_percent = u32_max_time / 100;
        u8_percents_to_switch_screen = 100 - (GUI_MIN_SPLASH_SCREEN_TIME / u16_time_of_one_percent);
    }

    /* Process splash screen */
    if ((GUI_TIMER_ELAPSED (u32_run_timer) >= u16_time_of_one_percent) && (g_u8_working_progress > 0))
    {
        GUI_TIMER_RESET (u32_run_timer);

        /* Update connecting progress */
        g_u8_working_progress--;
        lv_bar_set_value (g_px_bar_progress, g_u8_working_progress, LV_ANIM_OFF);

        /* Check if we are connected to wifi an access point */
        bool b_wifi_connected;
        s8_WIFIMN_Get_Selected_Ap (NULL, &b_wifi_connected);
        if ((b_wifi_connected) && (g_u8_working_progress <= u8_percents_to_switch_screen))
        {
            /* Go to roti making screen */
            s8_GUI_Get_Screen (GUI_SCREEN_ROTI_MAKING, &g_stru_screen.pstru_next);
            g_stru_screen.enm_result = GUI_SCREEN_RESULT_NEXT;
        }
        else if (g_u8_working_progress == 0)
        {
            /* Failed to connect to wifi, inform user */
            v_GUI_Create_No_Wifi_Msgbox ();
        }
    }

    /* Refresh GUI data if changed */
    static uint32_t u32_data_timer = 0;
    if (GUI_TIMER_ELAPSED (u32_data_timer) >= GUI_REFRESH_DATA_CYCLE)
    {
        GUI_TIMER_RESET (u32_data_timer);

        /* Refresh cooking script information */
        char stri_script_info[GUI_SCRIPT_INFO_LEN] = "";
        uint16_t u16_script_info_len = sizeof (stri_script_info);
        if (s8_GUI_Get_Data_If_Changed (GUI_DATA_SCRIPT_BRIEF_INFO, stri_script_info, &u16_script_info_len) == GUI_OK)
        {
            FWUESP_fw_desc_t stru_fw_desc;
            s8_FWUESP_Get_Fw_Descriptor (&stru_fw_desc);
            lv_label_set_text_fmt (g_px_lbl_fw_info, "Platform v%s - %s", stru_fw_desc.pstri_ver, stri_script_info);
            lv_obj_align (g_px_lbl_fw_info, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -30);
        }
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Creates message box displaying no wifi connection
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Create_No_Wifi_Msgbox (void)
{
    if (g_px_msgbox_no_wifi == NULL)
    {
        /* Create message box with one button */
        static const char * apstri_buttons[] = { "Wifi setting", "" };
        g_px_msgbox_no_wifi = lv_msgbox_create (g_stru_screen.px_lv_screen, NULL);
        lv_msgbox_set_text (g_px_msgbox_no_wifi, "Failed to connect to wifi access point.");
        lv_msgbox_add_btns (g_px_msgbox_no_wifi, apstri_buttons);
        lv_obj_set_width (g_px_msgbox_no_wifi, LV_HOR_RES * 3 / 4);
        lv_obj_set_event_cb (g_px_msgbox_no_wifi, v_GUI_Msgbox_No_Wifi_Event_Cb);
        lv_obj_align (g_px_msgbox_no_wifi, NULL, LV_ALIGN_CENTER, 0, 0);

        /* Configure the button */
        lv_obj_t * px_buttons = lv_msgbox_get_btnmatrix (g_px_msgbox_no_wifi);
        lv_obj_set_size (&px_buttons[0], 150, 50);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handles events of the message box displaying no wifi connection message
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Msgbox_No_Wifi_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* If user clicked the button on the message box */
    if (enm_event == LV_EVENT_VALUE_CHANGED)
    {
        /* Close the message box */
        lv_msgbox_start_auto_close (g_px_msgbox_no_wifi, 0);
        g_px_msgbox_no_wifi = NULL;

        /* Go to wifi setting screen */
        s8_GUI_Get_Screen (GUI_SCREEN_WIFI_SETTING, &g_stru_screen.pstru_next);
        g_stru_screen.enm_result = GUI_SCREEN_RESULT_NEXT;
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
