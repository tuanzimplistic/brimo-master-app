/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : menu.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Sep 10
**  @brief      : Implementation of Menu screen
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       Implements Menu screen, which is displayed when user clicks "Menu" button in Roti making screen
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "screen_common.h"              /* Common header of all screens */
#include "app_wifi_mngr.h"              /* Use Wifi Manager module */
#include "srvc_wifi.h"                  /* Use Wifi service */
#include "notify_msgbox.h"              /* Use notify message box */
#include "srvc_fwu_esp32.h"             /* Get firmware descriptor of master firmware */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Cycle (in milliseconds) polling Wifi status */
#define GUI_REFRESH_WIFI_CYCLE          1000

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static int8_t s8_GUI_Start_Menu_Screen (void);
static int8_t s8_GUI_Stop_Menu_Screen (void);
static int8_t s8_GUI_Run_Menu_Screen (void);
static void v_GUI_Refresh_Wifi_Button (void);
static void v_GUI_Btn_Back_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Btn_Wifi_Setting_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Btn_Developer_Tools_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Btn_About_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);

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
    .pstri_name         = "Menu",
    .px_icon            = NULL,
    .pfnc_start         = s8_GUI_Start_Menu_Screen,
    .pfnc_stop          = s8_GUI_Stop_Menu_Screen,
    .pfnc_run           = s8_GUI_Run_Menu_Screen,
    .enm_result         = GUI_SCREEN_RESULT_NONE,
};

/** @brief  Image for back button */
LV_IMG_DECLARE (img_back);

/** @brief  Image for wifi button */
LV_IMG_DECLARE (img_wifi);

/** @brief  Image for developer button */
LV_IMG_DECLARE (img_debug);

/** @brief  Image for about button */
LV_IMG_DECLARE (img_about);

/** @brief  Font for Wifi symbol */
LV_FONT_DECLARE (wifi_symbol);

/** @brief  Bold font */
LV_FONT_DECLARE (arial_bold_18);

/** @brief  Label of wifi signal symbol background */
static lv_obj_t * g_px_lbl_wifi_signal_bg = NULL;

/** @brief  Label of wifi signal symbol */
static lv_obj_t * g_px_lbl_wifi_signal = NULL;

/** @brief  Label of wifi setting button */
static lv_obj_t * g_px_lbl_wifi_setting = NULL;

/** @brief  Label of SSID in wifi setting button */
static lv_obj_t * g_px_lbl_wifi_ssid = NULL;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets structure wrapping Menu screen
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
int8_t s8_GUI_Get_Menu_Screen (GUI_screen_t ** ppstru_screen)
{
    ASSERT_PARAM (ppstru_screen != NULL);

    /* If this screen has not been initialized, initialize it */
    if (!g_b_initialized)
    {
        lv_obj_t * px_screen = lv_obj_create (NULL, NULL);

        static lv_style_t x_style_background;
        lv_style_init (&x_style_background);
        lv_style_set_bg_color (&x_style_background, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_style_set_border_width (&x_style_background, LV_STATE_DEFAULT, 0);
        lv_obj_add_style (px_screen, LV_LABEL_PART_MAIN, &x_style_background);

        /* Style for image buttons, darken the buttons when pressed */
        static lv_style_t x_style_imgbtn;
        lv_style_init (&x_style_imgbtn);
        lv_style_set_image_recolor_opa (&x_style_imgbtn, LV_STATE_PRESSED, LV_OPA_30);
        lv_style_set_image_recolor (&x_style_imgbtn, LV_STATE_PRESSED, LV_COLOR_BLACK);

        /* Back button */
        lv_obj_t * px_imgbtn_back = lv_imgbtn_create (px_screen, NULL);
        lv_obj_add_style (px_imgbtn_back, LV_IMGBTN_PART_MAIN, &x_style_imgbtn);
        lv_imgbtn_set_src (px_imgbtn_back, LV_BTN_STATE_RELEASED, &img_back);
        lv_obj_align (px_imgbtn_back, NULL, LV_ALIGN_IN_TOP_LEFT, 15, 15);
        lv_obj_set_event_cb (px_imgbtn_back, v_GUI_Btn_Back_Event_Cb);

        /* "SETTINGS" label */
        lv_obj_t * px_lbl_settings =  lv_label_create (px_screen, NULL);
        static lv_style_t x_style_settings;
        lv_style_init (&x_style_settings);
        lv_style_set_text_letter_space (&x_style_settings, LV_STATE_DEFAULT, 2);
        lv_obj_add_style (px_lbl_settings, LV_LABEL_PART_MAIN, &x_style_settings);
        lv_label_set_text (px_lbl_settings, "SETTINGS");
        lv_obj_align (px_lbl_settings, NULL, LV_ALIGN_IN_TOP_MID, 0, 20);

        /* Style for wifi symbol */
        static lv_style_t x_style_wifi_symbol;
        lv_style_init (&x_style_wifi_symbol);
        lv_style_set_text_font (&x_style_wifi_symbol, LV_STATE_DEFAULT, &wifi_symbol);
        lv_style_set_text_color (&x_style_wifi_symbol, LV_STATE_DEFAULT, LV_COLOR_MAKE (0xE0, 0xE0, 0xE0));

        /* Background of Wifi signal symbol */
        g_px_lbl_wifi_signal_bg = lv_label_create (px_screen, NULL);
        lv_obj_add_style (g_px_lbl_wifi_signal_bg, LV_LABEL_PART_MAIN, &x_style_wifi_symbol);
        lv_label_set_text (g_px_lbl_wifi_signal_bg, "6");
        lv_obj_align (g_px_lbl_wifi_signal_bg, NULL, LV_ALIGN_IN_TOP_RIGHT, -20, 25);

        /* Wifi signal symbol */
        g_px_lbl_wifi_signal = lv_label_create (px_screen, NULL);
        lv_obj_add_style (g_px_lbl_wifi_signal, LV_LABEL_PART_MAIN, &x_style_wifi_symbol);
        _lv_obj_set_style_local_color (g_px_lbl_wifi_signal, LV_LABEL_PART_MAIN, LV_STYLE_TEXT_COLOR,
                                       LV_THEME_DEFAULT_COLOR_PRIMARY);

        /* Style for button list */
        static lv_style_t x_style_list;
        lv_style_init (&x_style_list);
        lv_style_set_bg_color (&x_style_list, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_style_set_border_width (&x_style_list, LV_STATE_DEFAULT, 0);
        lv_style_set_pad_left (&x_style_list, LV_STATE_DEFAULT, 15);
        lv_style_set_pad_right (&x_style_list, LV_STATE_DEFAULT, 15);

        /* Create a button list */
        lv_obj_t * px_btn_list = lv_list_create (px_screen, NULL);
        lv_obj_set_size (px_btn_list, 400, 200);
        lv_obj_align (px_btn_list, NULL, LV_ALIGN_CENTER, 0, 10);
        lv_obj_add_style (px_btn_list, LV_LIST_PART_BG, &x_style_list);

        /* Style for each button in the button list */
        static lv_style_t x_style_btn;
        lv_style_init (&x_style_btn);
        lv_style_set_border_width (&x_style_btn, LV_STATE_FOCUSED, 0);
        lv_style_set_outline_width (&x_style_btn, LV_STATE_FOCUSED, 0);
        lv_style_set_bg_color (&x_style_btn, LV_STATE_FOCUSED, LV_COLOR_MAKE (0xDA, 0xDA, 0xDA));

        /* Wifi setting button */
        lv_obj_t * px_btn_wifi = lv_list_add_btn (px_btn_list, &img_wifi, "Connect to WiFi");
        lv_obj_add_style (px_btn_wifi, LV_BTN_PART_MAIN, &x_style_btn);
        lv_obj_set_event_cb (px_btn_wifi, v_GUI_Btn_Wifi_Setting_Event_Cb);
        g_px_lbl_wifi_setting = lv_obj_get_child (px_btn_wifi, NULL);

        g_px_lbl_wifi_ssid = lv_label_create (g_px_lbl_wifi_setting, NULL);
        lv_label_set_long_mode (g_px_lbl_wifi_ssid, LV_LABEL_LONG_SROLL_CIRC);
        lv_obj_set_width (g_px_lbl_wifi_ssid, 100);
        lv_obj_set_height (g_px_lbl_wifi_ssid, 20);
        lv_obj_align (g_px_lbl_wifi_ssid, g_px_lbl_wifi_setting, LV_ALIGN_IN_RIGHT_MID, 0, 0);
        lv_label_set_align (g_px_lbl_wifi_ssid, LV_LABEL_ALIGN_RIGHT);

        static lv_style_t x_style_ssid;
        lv_style_init (&x_style_ssid);
        lv_style_set_text_font (&x_style_ssid, LV_STATE_DEFAULT, &arial_bold_18);
        lv_obj_add_style (g_px_lbl_wifi_ssid, LV_LABEL_PART_MAIN, &x_style_ssid);
        v_GUI_Refresh_Wifi_Button ();

        /* Developer Tools button */
        lv_obj_t * px_btn_developer =  lv_list_add_btn (px_btn_list, &img_debug, "Developer tools");
        lv_obj_add_style (px_btn_developer, LV_BTN_PART_MAIN, &x_style_btn);
        lv_obj_set_event_cb (px_btn_developer, v_GUI_Btn_Developer_Tools_Event_Cb);

        /* About button */
        lv_obj_t * px_btn_about =  lv_list_add_btn (px_btn_list, &img_about, "About");
        lv_obj_add_style (px_btn_about, LV_BTN_PART_MAIN, &x_style_btn);
        lv_obj_set_event_cb (px_btn_about, v_GUI_Btn_About_Event_Cb);

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
**      Starts Menu screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Start_Menu_Screen (void)
{
    LOGD ("Menu screen started");
    g_stru_screen.enm_result = GUI_SCREEN_RESULT_NONE;

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Stops Menu screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Stop_Menu_Screen (void)
{
    LOGD ("Menu screen stopped");
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs Menu screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Run_Menu_Screen (void)
{
    /* Refresh Wifi status */
    static uint32_t u32_wifi_timer = 0;
    if (GUI_TIMER_ELAPSED (u32_wifi_timer) >= GUI_REFRESH_WIFI_CYCLE)
    {
        GUI_TIMER_RESET (u32_wifi_timer);

        /* Refresh appearance of wifi setting button */
        v_GUI_Refresh_Wifi_Button ();

        /* Check wifi connection status */
        bool b_connected;
        if (s8_WIFIMN_Get_Selected_Ap (NULL, &b_connected) == WIFIMN_OK)
        {
            if (!b_connected)
            {
                /* Hide wifi signal indicator */
                lv_label_set_text (g_px_lbl_wifi_signal_bg, "");
                lv_label_set_text (g_px_lbl_wifi_signal, "");
            }
            else
            {
                lv_label_set_text (g_px_lbl_wifi_signal_bg, "6");

                /* Wifi receive signal strength */
                WIFI_ap_info_t stru_ap_info;
                if (s8_WIFI_Get_Ap_Info (&stru_ap_info) == WIFI_OK)
                {
                    if (stru_ap_info.s8_rssi < -90)
                    {
                        /* Unusable */
                        lv_label_set_text (g_px_lbl_wifi_signal, "0");
                    }
                    else if (stru_ap_info.s8_rssi < -80)
                    {
                        /* Not Good */
                        lv_label_set_text (g_px_lbl_wifi_signal, "2");
                    }
                    else if (stru_ap_info.s8_rssi < -70)
                    {
                        /* Okay */
                        lv_label_set_text (g_px_lbl_wifi_signal, "4");
                    }
                    else
                    {
                        /* Very Good */
                        lv_label_set_text (g_px_lbl_wifi_signal, "6");
                    }
                    lv_obj_align (g_px_lbl_wifi_signal, NULL, LV_ALIGN_IN_TOP_RIGHT, -20, 25);
                }
            }
        }
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Refreshes apperaance of Wifi setting button accordingly to Wifi connection status
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Refresh_Wifi_Button (void)
{
    WIFIMN_cred_t * pstru_ap;
    bool            b_connected;

    /* Check wifi connection status */
    if (s8_WIFIMN_Get_Selected_Ap (&pstru_ap, &b_connected) == WIFIMN_OK)
    {
        if (b_connected)
        {
            lv_label_set_text (g_px_lbl_wifi_setting, "Reconnect WiFi");
            lv_label_set_text (g_px_lbl_wifi_ssid, pstru_ap->stri_ssid);
        }
        else
        {
            lv_label_set_text (g_px_lbl_wifi_setting, "Connect to WiFi");
            lv_label_set_text (g_px_lbl_wifi_ssid, "");
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Event handler of "back" button
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Btn_Back_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* Only process click event */
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Back to the previous screen */
        g_stru_screen.enm_result = GUI_SCREEN_RESULT_BACK;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Event handler of button Wifi setting
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Btn_Wifi_Setting_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Open Wifi setting screen */
        s8_GUI_Get_Screen (GUI_SCREEN_WIFI_SETTING, &g_stru_screen.pstru_next);
        g_stru_screen.enm_result = GUI_SCREEN_RESULT_NEXT;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Event handler of button Developer Tool
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Btn_Developer_Tools_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Open Developer screen */
        s8_GUI_Get_Screen (GUI_SCREEN_DEVELOPER, &g_stru_screen.pstru_next);
        g_stru_screen.enm_result = GUI_SCREEN_RESULT_NEXT;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Event handler of button About
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Btn_About_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Get firmware descriptor of master firmware */
        FWUESP_fw_desc_t stru_fw_desc;
        s8_FWUESP_Get_Fw_Descriptor (&stru_fw_desc);

        /* Get detailed information of cooking script */
        uint16_t u16_script_info_len = 128;
        char * pstri_script_info = malloc (u16_script_info_len);
        ASSERT_PARAM (pstri_script_info != NULL);
        s8_GUI_Get_Data (GUI_DATA_SCRIPT_DETAIL_INFO, pstri_script_info, &u16_script_info_len);

        /* Information about the firmware and cooking script */
        char * pstri_about = malloc (256);
        ASSERT_PARAM (pstri_about != NULL);
        snprintf (pstri_about, 256, "Platform:\n"
                                    "+ Version: %s\n"
                                    "+ Time: %s\n\n"
                                    "Cooking script:\n"
                                    "%s",
                                    stru_fw_desc.pstri_ver,
                                    stru_fw_desc.pstri_time,
                                    pstri_script_info);

        /* Show information about the firmware */
        GUI_notify_t stru_notify =
        {
            .enm_type = GUI_MSG_INFO,
            .pstri_brief = "About",
            .pstri_detail = pstri_about,
            .u32_wait_time = 0,
        };
        s8_GUI_Show_Notify_Msgbox (&stru_notify);

        /* Clean up */
        free (pstri_script_info);
        free (pstri_about);
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
