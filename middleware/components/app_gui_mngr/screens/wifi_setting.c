/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : wifi_setting.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 May 24
**  @brief      : Implementation of Wifi Setting screen
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       Implements Wifi Setting screen
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
#include "virtual_keyboard.h"           /* Use virtual keyboard */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  States of this screen */
enum
{
    GUI_STATE_IDLE,                     //!< The screen is idling
    GUI_STATE_WIFI_SCANNING,            //!< The screen is scanning wifi
    GUI_STATE_PSW_INPUTTING,            //!< The screen is waiting for password to be input
};

/** @brief  Text displayed when there is no wifi access point */
#define GUI_NO_WIFI_ACCESS_POINT        "<No access point found>"

/** @brief  Cycle (in milliseconds) polling Wifi status */
#define GUI_REFRESH_WIFI_CYCLE          1000

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static int8_t s8_GUI_Start_Wifi_Setting_Screen (void);
static int8_t s8_GUI_Stop_Wifi_Setting_Screen (void);
static int8_t s8_GUI_Run_Wifi_Setting_Screen (void);
static void v_GUI_Create_Wifi_Scanning_Progress (void);
static void v_GUI_Txt_Password_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Ddl_Ap_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Btn_Connect_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Btn_Rescan_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Btn_Back_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);

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
    .pstri_name         = "Wifi Setting",
    .px_icon            = NULL,
    .pfnc_start         = s8_GUI_Start_Wifi_Setting_Screen,
    .pfnc_stop          = s8_GUI_Stop_Wifi_Setting_Screen,
    .pfnc_run           = s8_GUI_Run_Wifi_Setting_Screen,
    .enm_result         = GUI_SCREEN_RESULT_NONE,
};

/** @brief  State of this screen */
static uint8_t g_u8_state = GUI_STATE_IDLE;

/** @brief  Wifi scanning progress */
static lv_obj_t * g_px_scanning_progress = NULL;

/** @brief  Wifi password box */
static lv_obj_t * g_px_txt_password = NULL;

/** @brief  List of available access points */
static lv_obj_t * g_px_ddl_ap_list = NULL;

/** @brief  Label of "Connect" button */
static lv_obj_t * g_px_lbl_connect = NULL;

/** @brief  Font for Wifi symbol */
LV_FONT_DECLARE (wifi_symbol);

/** @brief  Label of wifi signal symbol background */
static lv_obj_t * g_px_lbl_wifi_signal_bg = NULL;

/** @brief  Label of wifi signal symbol */
static lv_obj_t * g_px_lbl_wifi_signal = NULL;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets structure wrapping wifi setting screen
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
int8_t s8_GUI_Get_Wifi_Setting_Screen (GUI_screen_t ** ppstru_screen)
{
    ASSERT_PARAM (ppstru_screen != NULL);

    /* If this screen has not been initialized, initialize it */
    if (!g_b_initialized)
    {
        lv_obj_t * px_screen = lv_obj_create (NULL, NULL);

        /* "Available networks" label */
        lv_obj_t * px_lbl_network =  lv_label_create (px_screen, NULL);
        lv_label_set_text (px_lbl_network, "Available networks");
        lv_obj_align (px_lbl_network, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);

        /* Style for wifi symbol */
        static lv_style_t x_style_wifi_symbol;
        lv_style_init (&x_style_wifi_symbol);
        lv_style_set_text_font (&x_style_wifi_symbol, LV_STATE_DEFAULT, &wifi_symbol);
        lv_style_set_text_color (&x_style_wifi_symbol, LV_STATE_DEFAULT, LV_COLOR_MAKE (0xE0, 0xE0, 0xE0));

        /* Background of Wifi signal symbol */
        g_px_lbl_wifi_signal_bg = lv_label_create (px_screen, NULL);
        lv_obj_add_style (g_px_lbl_wifi_signal_bg, LV_LABEL_PART_MAIN, &x_style_wifi_symbol);
        lv_label_set_text (g_px_lbl_wifi_signal_bg, "6");
        lv_obj_align (g_px_lbl_wifi_signal_bg, NULL, LV_ALIGN_IN_TOP_RIGHT, -15, 10);

        /* Wifi signal symbol */
        g_px_lbl_wifi_signal = lv_label_create (px_screen, NULL);
        lv_obj_add_style (g_px_lbl_wifi_signal, LV_LABEL_PART_MAIN, &x_style_wifi_symbol);
        _lv_obj_set_style_local_color (g_px_lbl_wifi_signal, LV_LABEL_PART_MAIN, LV_STYLE_TEXT_COLOR,
                                       LV_THEME_DEFAULT_COLOR_PRIMARY);

        /* List of available networks */
        g_px_ddl_ap_list = lv_dropdown_create (px_screen, NULL);
        lv_dropdown_clear_options (g_px_ddl_ap_list);
        lv_dropdown_add_option (g_px_ddl_ap_list, "<No wifi access point>", 0);
        lv_obj_align (g_px_ddl_ap_list, px_lbl_network, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
        lv_obj_set_width (g_px_ddl_ap_list, LV_HOR_RES - 2 * lv_obj_get_x (g_px_ddl_ap_list));
        lv_dropdown_set_max_height (g_px_ddl_ap_list, LV_VER_RES / 2);
        lv_obj_set_event_cb (g_px_ddl_ap_list, v_GUI_Ddl_Ap_Event_Cb);

        /* "Password" label */
        lv_obj_t * px_lbl_password = lv_label_create (px_screen, NULL);
        lv_label_set_text (px_lbl_password, "Password");
        lv_obj_align (px_lbl_password, g_px_ddl_ap_list, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);

        /* Password box */
        g_px_txt_password = lv_textarea_create (px_screen, NULL);
        lv_textarea_set_text (g_px_txt_password, "");
        lv_obj_align (g_px_txt_password, px_lbl_password, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
        lv_textarea_set_pwd_mode (g_px_txt_password, true);
        lv_textarea_set_one_line (g_px_txt_password, true);
        lv_textarea_set_cursor_hidden (g_px_txt_password, true);
        lv_obj_set_width (g_px_txt_password, LV_HOR_RES - 2 * lv_obj_get_x (g_px_txt_password));
        lv_obj_set_event_cb (g_px_txt_password, v_GUI_Txt_Password_Event_Cb);

        /* "Back" button */
        lv_obj_t * px_btn_back = lv_btn_create (px_screen, NULL);
        lv_obj_set_size (px_btn_back, 110, 35);
        lv_obj_align (px_btn_back, px_screen, LV_ALIGN_IN_BOTTOM_LEFT, 30, -40);
        lv_obj_set_event_cb (px_btn_back, v_GUI_Btn_Back_Event_Cb);

        lv_obj_t * px_lbl_back = lv_label_create (px_btn_back, NULL);
        lv_label_set_text (px_lbl_back, "Back");

        /* "Rescan" button */
        lv_obj_t * px_btn_rescan = lv_btn_create (px_screen, NULL);
        lv_obj_set_size (px_btn_rescan, 110, 35);
        lv_obj_align (px_btn_rescan, px_screen, LV_ALIGN_IN_BOTTOM_MID, 0, -40);
        lv_obj_set_event_cb (px_btn_rescan, v_GUI_Btn_Rescan_Event_Cb);

        lv_obj_t * px_lbl_rescan = lv_label_create (px_btn_rescan, NULL);
        lv_label_set_text (px_lbl_rescan, "Rescan");

        /* "Connect" button */
        lv_obj_t * px_btn_connect = lv_btn_create (px_screen, NULL);
        lv_obj_set_size (px_btn_connect, 110, 35);
        lv_obj_align (px_btn_connect, px_screen, LV_ALIGN_IN_BOTTOM_RIGHT, -30, -40);
        lv_obj_set_event_cb (px_btn_connect, v_GUI_Btn_Connect_Event_Cb);

        g_px_lbl_connect = lv_label_create (px_btn_connect, NULL);
        lv_label_set_text (g_px_lbl_connect, "Connect");

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
**      Starts Wifi Setting screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Start_Wifi_Setting_Screen (void)
{
    LOGI ("Wifi Setting screen started");
    g_stru_screen.enm_result = GUI_SCREEN_RESULT_NONE;

    /* Start wifi scanning */
    if (g_u8_state == GUI_STATE_IDLE)
    {
        g_u8_state = GUI_STATE_WIFI_SCANNING;
        s8_WIFIMN_Start_Scan ();
        v_GUI_Create_Wifi_Scanning_Progress ();
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Stops Wifi Setting screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Stop_Wifi_Setting_Screen (void)
{
    LOGI ("Wifi Setting screen stopped");
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs Wifi Setting screen
**
** @return
**      @arg    GUI_DONE
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Run_Wifi_Setting_Screen (void)
{
    /* Refresh Wifi status */
    static uint32_t u32_wifi_timer = 0;
    if (GUI_TIMER_ELAPSED (u32_wifi_timer) >= GUI_REFRESH_WIFI_CYCLE)
    {
        GUI_TIMER_RESET (u32_wifi_timer);

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

                /* Wifi receive access point */
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
                    lv_obj_align (g_px_lbl_wifi_signal, NULL, LV_ALIGN_IN_TOP_RIGHT, -15, 10);
                }
            }
        }
    }

    /* If we are scanning wifi */
    if (g_u8_state == GUI_STATE_WIFI_SCANNING)
    {
        WIFIMN_ap_t * pastru_ap_list;
        uint16_t u16_num_ap = 0;
        int8_t s8_result = s8_WIFIMN_Get_Scan_Ap_List (&pastru_ap_list, &u16_num_ap);
        if (s8_result != WIFIMN_ERR_BUSY)
        {
            /* Scanning is done, close progress indicator */
            g_u8_state = GUI_STATE_IDLE;
            lv_obj_del (g_px_scanning_progress);
            g_px_scanning_progress = NULL;

            /* Wifi access point list */
            lv_dropdown_clear_options (g_px_ddl_ap_list);
            if ((s8_result == WIFIMN_OK) && (u16_num_ap > 0))
            {
                /* Scanning succeeded */
                lv_dropdown_clear_options (g_px_ddl_ap_list);
                for (uint8_t u8_idx = 0; u8_idx < u16_num_ap; u8_idx++)
                {
                    lv_dropdown_add_option (g_px_ddl_ap_list, pastru_ap_list[u8_idx].stri_ssid, LV_DROPDOWN_POS_LAST);
                }

                /* Select the user access point (if available) from the list */
                bool b_ap_found = false;
                WIFIMN_cred_t * pstru_ap;
                if (s8_WIFIMN_Get_User_Ap (&pstru_ap) == WIFIMN_OK)
                {
                    for (uint8_t u8_idx = 0; u8_idx < u16_num_ap; u8_idx++)
                    {
                        if (strcmp (pstru_ap->stri_ssid, pastru_ap_list[u8_idx].stri_ssid) == 0)
                        {
                            lv_dropdown_set_selected (g_px_ddl_ap_list, u8_idx);
                            v_GUI_Ddl_Ap_Event_Cb (g_px_ddl_ap_list, LV_EVENT_VALUE_CHANGED);
                            b_ap_found = true;
                            break;
                        }
                    }
                }

                /* If user access point is not in the list, select the first one */
                if (!b_ap_found)
                {
                    lv_dropdown_set_selected (g_px_ddl_ap_list, 0);
                    v_GUI_Ddl_Ap_Event_Cb (g_px_ddl_ap_list, LV_EVENT_VALUE_CHANGED);
                }
            }
            else
            {
                lv_dropdown_add_option (g_px_ddl_ap_list, GUI_NO_WIFI_ACCESS_POINT, LV_DROPDOWN_POS_LAST);
                lv_textarea_set_text (g_px_txt_password, "");

                lv_dropdown_set_selected (g_px_ddl_ap_list, 0);
                v_GUI_Ddl_Ap_Event_Cb (g_px_ddl_ap_list, LV_EVENT_VALUE_CHANGED);
            }
        }
    }

    /* If we are waiting for password */
    else if (g_u8_state == GUI_STATE_PSW_INPUTTING)
    {
        g_u8_state = GUI_STATE_IDLE;

        /* Get the password */
        const char * pstri_psw;
        s8_GUI_Virkb_Get_Text (&pstri_psw);
        lv_textarea_set_text (g_px_txt_password, pstri_psw);

        /* This trick is to stop the text area to display the final character of the password */
        lv_textarea_add_char (g_px_txt_password, '*');
        lv_textarea_del_char (g_px_txt_password);
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Creates progress spinner for wifi scanning
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Create_Wifi_Scanning_Progress (void)
{
    if (g_px_scanning_progress == NULL)
    {
        /* Create a transparent full-screen object to absorb all click events */
        g_px_scanning_progress = lv_obj_create (g_stru_screen.px_lv_screen, NULL);
        lv_obj_reset_style_list (g_px_scanning_progress, LV_OBJ_PART_MAIN);
        lv_obj_set_size (g_px_scanning_progress, LV_HOR_RES, LV_VER_RES);

        /* Create a spinner for the scanning progress */
        lv_obj_t * px_spinner = lv_spinner_create (g_px_scanning_progress, NULL);
        lv_obj_set_size (px_spinner, 100, 100);
        lv_obj_align (px_spinner, NULL, LV_ALIGN_CENTER, 0, 0);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of Password text area events
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Txt_Password_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* If user first clicked inside the password box */
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Don't need to enter password if there is no wifi access point */
        if (lv_dropdown_get_option_cnt (g_px_ddl_ap_list) == 1)
        {
            char stri_ap[WIFIMN_SSID_LEN];
            lv_dropdown_get_selected_str (g_px_ddl_ap_list, stri_ap, sizeof(stri_ap));
            if (strcmp (GUI_NO_WIFI_ACCESS_POINT, stri_ap) == 0)
            {
                return;
            }
        }

        /* Open virtual keyboard to enter password */
        GUI_virkb_cfg_t stru_kb_config = GUI_VIRKB_DEFAULT_CFG;
        stru_kb_config.b_password_mode = true;
        stru_kb_config.pstri_brief = "Wifi password";
        stru_kb_config.u16_max_text_len = WIFIMN_PSW_LEN - 1;
        stru_kb_config.pstri_init_text = lv_textarea_get_text (g_px_txt_password);
        s8_GUI_Virkb_Set_Config (&stru_kb_config);

        g_u8_state = GUI_STATE_PSW_INPUTTING;
        s8_GUI_Get_Screen (GUI_SCREEN_VIRTUAL_KEYBOARD, &g_stru_screen.pstru_next);
        g_stru_screen.enm_result = GUI_SCREEN_RESULT_NEXT;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handles events of Access point drop down list
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Ddl_Ap_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* If user changed an option in the list */
    if (enm_event == LV_EVENT_VALUE_CHANGED)
    {
        /* Check if the currently selected option is also the user selected access point */
        WIFIMN_cred_t * pstru_ap;
        if (s8_WIFIMN_Get_User_Ap (&pstru_ap) == WIFIMN_OK)
        {
            char stri_ap[WIFIMN_SSID_LEN];
            lv_dropdown_get_selected_str (g_px_ddl_ap_list, stri_ap, sizeof (stri_ap));

            if (strcmp (pstru_ap->stri_ssid, stri_ap) == 0)
            {
                /* Automatically input the password */
                lv_textarea_set_text (g_px_txt_password, pstru_ap->stri_psw);
                lv_btn_set_state (lv_obj_get_parent (g_px_lbl_connect), LV_BTN_STATE_RELEASED);

                /* This trick is to stop the password box to display the whole wifi password */
                lv_textarea_add_char (g_px_txt_password, '*');
                lv_textarea_del_char (g_px_txt_password);
            }
            else if ((lv_dropdown_get_option_cnt (g_px_ddl_ap_list) == 1) &&
                     (strcmp (GUI_NO_WIFI_ACCESS_POINT, stri_ap) == 0))
            {
                /* No wifi access point, disable "Connect" button */
                lv_textarea_set_text (g_px_txt_password, "");
                lv_btn_set_state (lv_obj_get_parent (g_px_lbl_connect), LV_BTN_STATE_DISABLED);
            }
            else
            {
                lv_textarea_set_text (g_px_txt_password, "");
                lv_btn_set_state (lv_obj_get_parent (g_px_lbl_connect), LV_BTN_STATE_RELEASED);
            }
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of button "Connect" events
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Btn_Connect_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* If user clicked the button */
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Connect to the selected access point */
        WIFIMN_cred_t stru_cred;
        lv_dropdown_get_selected_str (g_px_ddl_ap_list, stru_cred.stri_ssid, sizeof (stru_cred.stri_ssid));
        strncpy (stru_cred.stri_psw, lv_textarea_get_text (g_px_txt_password), sizeof (stru_cred.stri_psw));
        s8_WIFIMN_Connect (&stru_cred);

        /* Open splash screen */
        s8_GUI_Get_Screen (GUI_SCREEN_SPLASH, &g_stru_screen.pstru_next);
        g_stru_screen.enm_result = GUI_SCREEN_RESULT_NEXT;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of button "Rescan" events
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Btn_Rescan_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Start wifi scanning */
        g_u8_state = GUI_STATE_WIFI_SCANNING;
        s8_WIFIMN_Start_Scan ();
        v_GUI_Create_Wifi_Scanning_Progress ();
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of button "Back" events
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
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Back to the previous screen */
        g_stru_screen.enm_result = GUI_SCREEN_RESULT_BACK;
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
