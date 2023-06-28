/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : roti_making.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Aug 12
**  @brief      : Implementation of Roti making screen
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @section     RotiMakingScreen Roti making screen
** @brief       Implements Roti making screen
** @details
**
** Roti making screen is bound with the following data:
**
** @verbatim
** +----------------------------+------------+------------------------------------------------------------------+
** | GUI data                   | GUI access | Description                                                      |
** +----------------------------+------------+------------------------------------------------------------------+
** | GUI_DATA_ROTI_COUNT        | Read/Write | Number of Roti's to be made                                      |
** | GUI_DATA_ROTI_MADE         | Read       | Number of Roti's that have been made                             |
** | GUI_DATA_RECIPE_NAME       | Read       | Name of the recipe being used                                    |
** | GUI_DATA_FLOUR_NAME        | Read       | Name of the flour being used                                     |
** | GUI_DATA_ROAST_LEVEL       | Read/Write | Roast level                                                      |
** | GUI_DATA_THICKNESS_LEVEL   | Read/Write | Thickness level                                                  |
** | GUI_DATA_OIL_LEVEL         | Read/Write | Oil level                                                        |
** | GUI_DATA_COOKING_STARTED   | Read/Write | Indicates if cooking has been started by user (1) or not (0)     |
** | GUI_DATA_COOKING_STATE     | Read       | Instantaneous cooking state (0 = idle, 1 = cooking)              |
** +----------------------------+------------+------------------------------------------------------------------+
** @endverbatim
**
** This is an example on how to interact with Roti making screen from MicroPython:
**
** @code{.py}
**
**  import gui
**
**  # Initialize Roti making screen
**  gui.set_data (gui.GUI_DATA_ROTI_MADE, 0)
**  gui.set_data (gui.GUI_DATA_RECIPE_NAME, 'ROTI')
**  gui.set_data (gui.GUI_DATA_FLOUR_NAME, 'Pillsbury gold wholewheat atta')
**  gui.set_data (gui.GUI_DATA_COOKING_STATE, 0)
**
**  # Wait until user starts cooking
**  while gui.get_data (gui.GUI_DATA_COOKING_STARTED) == 0:
**     pass
**
**  # Get cooking parameters
**  roti_count      = gui.get_data (gui.GUI_DATA_ROTI_COUNT)
**  roast_level     = gui.get_data (gui.GUI_DATA_ROAST_LEVEL)
**  thickness_level = gui.get_data (gui.GUI_DATA_THICKNESS_LEVEL)
**  oil_level       = gui.get_data (gui.GUI_DATA_OIL_LEVEL)
**
**  # Start cooking
**  gui.set_data (gui.GUI_DATA_COOKING_STATE, 1)
**  for i in range(0, roti_count):
**      # Do the cooking
**      # ...
**      # This roti has been done
**      gui.set_data (gui.GUI_DATA_ROTI_MADE, i + 1)
**
**  # Cooking is done
**  gui.set_data (gui.GUI_DATA_COOKING_STATE, 0)
**
** @endcode
**
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "screen_common.h" /* Common header of all screens */
#include "app_wifi_mngr.h" /* Use Wifi Manager module */
#include "srvc_wifi.h"     /* Use Wifi service */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Maximum roast level */
#define GUI_MAX_ROAST_LEVEL 5

/** @brief  Maximum thickness level */
#define GUI_MAX_THICKNESS_LEVEL 5

/** @brief  Maximum oil level */
#define GUI_MAX_OIL_LEVEL 2

/** @brief  Cycle (in milliseconds) polling GUI data */
#define GUI_REFRESH_DATA_CYCLE 50

/** @brief  Cycle (in milliseconds) polling Wifi status */
#define GUI_REFRESH_WIFI_CYCLE 1000

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static int8_t s8_GUI_Start_Roti_Making_Screen(void);
static int8_t s8_GUI_Stop_Roti_Making_Screen(void);
static int8_t s8_GUI_Run_Roti_Making_Screen(void);
static void v_GUI_Lbl_Wifi_Event_Cb(lv_obj_t *px_obj, lv_event_t enm_event);
static void v_GUI_Btn_Start_Event_Cb(lv_obj_t *px_obj, lv_event_t enm_event);
static void v_GUI_Btn_Menu_Event_Cb(lv_obj_t *px_obj, lv_event_t enm_event);
static void event_handler_drinks(lv_obj_t *px_obj, lv_event_t enm_event);
static void event_handler_nocups(lv_obj_t *px_obj, lv_event_t enm_event);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char *TAG = "App_Gui_Mngr";

/** @brief  Indicates if this screen has been initialized */
static bool g_b_initialized = false;

/** @brief  Screen information */
static GUI_screen_t g_stru_screen =
    {
        .pstru_prev = NULL,
        .pstru_next = NULL,
        .px_lv_screen = NULL,
        .pstri_name = "Roti Making",
        .px_icon = NULL,
        .pfnc_start = s8_GUI_Start_Roti_Making_Screen,
        .pfnc_stop = s8_GUI_Stop_Roti_Making_Screen,
        .pfnc_run = s8_GUI_Run_Roti_Making_Screen,
        .enm_result = GUI_SCREEN_RESULT_NONE,
};

/** @brief  Image for start button */
LV_IMG_DECLARE(img_play);

/** @brief  Image for pause button */
LV_IMG_DECLARE(img_pause);

/** @brief  Image for plus button */
LV_IMG_DECLARE(img_plus);

/** @brief  Image for minus button */
LV_IMG_DECLARE(img_minus);

/** @brief  Font for Roti count */
LV_FONT_DECLARE(arial_96);

/** @brief  Bold font */
LV_FONT_DECLARE(arial_bold_18);

/** @brief  Font for Wifi symbol */
LV_FONT_DECLARE(wifi_symbol);

/** @brief  Label of wifi signal symbol */
static lv_obj_t *g_px_lbl_wifi_signal = NULL;

/** @brief  Label of wifi access point name */
static lv_obj_t *g_px_lbl_ap = NULL;

/** @brief  Image button to start/stop cooking */
static lv_obj_t *g_px_imgbtn_start;

/** @brief  list of available drinks */
const char *g_arr_drinks[] = {"Coffee", "Chai", "Cappuccino-Dalgona", "Cappuccino-Western", "Macchiato", "Espresso", "Affogato"};

/** @brief  list of available no cups */
const char *g_arr_nocups[] = {"1", "2", "3", "4", "5"};

/** @brief  list of drinks */
static lv_obj_t *g_list_drinks;

/** @brief  list of cups */
static lv_obj_t *g_list_no_cups;

/** @brief  current drink */
static lv_obj_t *g_btn_current_drink = NULL;

/** @brief  current cup */
static lv_obj_t *g_btn_current_cup = NULL;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets structure wrapping roti making screen
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
int8_t s8_GUI_Get_Roti_Making_Screen(GUI_screen_t **ppstru_screen)
{
    ASSERT_PARAM(ppstru_screen != NULL);

    /* If this screen has not been initialized, initialize it */
    if (!g_b_initialized)
    {
        lv_obj_t *px_screen = lv_obj_create(NULL, NULL);

        /* Left panel */
        lv_obj_t *px_left_panel = lv_obj_create(px_screen, NULL);
        lv_obj_set_size(px_left_panel, 90, LV_VER_RES);

        static lv_style_t x_style_left_panel;
        lv_style_init(&x_style_left_panel);
        lv_style_set_bg_color(&x_style_left_panel, LV_STATE_DEFAULT, LV_COLOR_MAKE(246, 246, 246));
        lv_style_set_radius(&x_style_left_panel, LV_STATE_DEFAULT, 0);
        lv_style_set_border_width(&x_style_left_panel, LV_STATE_DEFAULT, 0);
        lv_obj_add_style(px_left_panel, LV_LABEL_PART_MAIN, &x_style_left_panel);

        /* Menu button */
        lv_obj_t *px_btn_menu = lv_btn_create(px_left_panel, NULL);
        lv_obj_set_size(px_btn_menu, 80, 40);
        lv_obj_align(px_btn_menu, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
        lv_obj_set_event_cb(px_btn_menu, v_GUI_Btn_Menu_Event_Cb);
        lv_label_set_text(lv_label_create(px_btn_menu, NULL), "MENU");

        static lv_style_t x_style_menu_btn;
        lv_style_init(&x_style_menu_btn);
        lv_style_set_radius(&x_style_menu_btn, LV_STATE_DEFAULT, 8);
        lv_style_set_bg_color(&x_style_menu_btn, LV_STATE_DEFAULT, LV_THEME_DEFAULT_COLOR_PRIMARY);
        lv_style_set_border_width(&x_style_menu_btn, LV_STATE_DEFAULT, 0);
        lv_style_set_text_color(&x_style_menu_btn, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_style_set_text_font(&x_style_menu_btn, LV_STATE_DEFAULT, &arial_bold_18);
        lv_obj_add_style(px_btn_menu, LV_LABEL_PART_MAIN, &x_style_menu_btn);

        /* Right panel */
        lv_obj_t *px_right_panel = lv_obj_create(px_screen, NULL);
        lv_obj_set_size(px_right_panel, 390, LV_VER_RES);
        lv_obj_set_pos(px_right_panel, 90, 0);

        static lv_style_t x_style_right_panel;
        lv_style_init(&x_style_right_panel);
        lv_style_set_bg_color(&x_style_right_panel, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_style_set_radius(&x_style_right_panel, LV_STATE_DEFAULT, 0);
        lv_style_set_border_width(&x_style_right_panel, LV_STATE_DEFAULT, 0);
        lv_obj_add_style(px_right_panel, LV_LABEL_PART_MAIN, &x_style_right_panel);

        /* Background of Wifi singal */
        lv_obj_t *px_wifi_background = lv_obj_create(px_right_panel, NULL);
        lv_obj_set_size(px_wifi_background, 50, 35);
        lv_obj_align(px_wifi_background, NULL, LV_ALIGN_IN_TOP_RIGHT, -10, 10);
        lv_obj_set_click(px_wifi_background, true);
        lv_obj_set_event_cb(px_wifi_background, v_GUI_Lbl_Wifi_Event_Cb);
        lv_obj_add_style(px_wifi_background, LV_LABEL_PART_MAIN, &x_style_right_panel);

        /* Style for wifi symbol */
        static lv_style_t x_style_wifi_symbol;
        lv_style_init(&x_style_wifi_symbol);
        lv_style_set_text_font(&x_style_wifi_symbol, LV_STATE_DEFAULT, &wifi_symbol);
        lv_style_set_text_color(&x_style_wifi_symbol, LV_STATE_DEFAULT, LV_COLOR_MAKE(0xE0, 0xE0, 0xE0));

        /* Background of Wifi signal symbol */
        lv_obj_t *px_lbl_wifi_background = lv_label_create(px_wifi_background, NULL);
        lv_obj_add_style(px_lbl_wifi_background, LV_LABEL_PART_MAIN, &x_style_wifi_symbol);
        lv_label_set_text(px_lbl_wifi_background, "6");
        lv_obj_align(px_lbl_wifi_background, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

        /* Wifi signal symbol */
        g_px_lbl_wifi_signal = lv_label_create(px_wifi_background, NULL);
        lv_obj_add_style(g_px_lbl_wifi_signal, LV_LABEL_PART_MAIN, &x_style_wifi_symbol);
        _lv_obj_set_style_local_color(g_px_lbl_wifi_signal, LV_LABEL_PART_MAIN, LV_STYLE_TEXT_COLOR,
                                      LV_THEME_DEFAULT_COLOR_PRIMARY);

        /* Wifi access point name */
        g_px_lbl_ap = lv_label_create(px_wifi_background, NULL);
        lv_label_set_long_mode(g_px_lbl_ap, LV_LABEL_LONG_SROLL_CIRC);
        lv_obj_set_width(g_px_lbl_ap, 50);

        static lv_style_t x_style_ap;
        lv_style_init(&x_style_ap);
        lv_style_set_text_font(&x_style_ap, LV_STATE_DEFAULT, &lv_font_montserrat_10);
        lv_style_set_text_color(&x_style_ap, LV_STATE_DEFAULT, LV_COLOR_GRAY);
        lv_obj_add_style(g_px_lbl_ap, LV_LABEL_PART_MAIN, &x_style_ap);
        lv_obj_align(g_px_lbl_ap, px_lbl_wifi_background, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);

        /* options */
        lv_obj_t *lbl_drinks = lv_label_create(px_right_panel, NULL);
        lv_obj_align(lbl_drinks, px_right_panel, LV_ALIGN_OUT_LEFT_TOP, 80, 20);
        lv_label_set_text_fmt(lbl_drinks, "%s", "Choose a Drink");
        /*Create a list*/
        g_list_drinks = lv_list_create(px_right_panel, NULL);
        lv_obj_set_size(g_list_drinks, 250, 200);
        lv_obj_align(g_list_drinks, px_right_panel, LV_ALIGN_OUT_LEFT_TOP, 250, 50);
        lv_list_set_scrollbar_mode(g_list_drinks, LV_SCROLLBAR_MODE_AUTO);

        /*Add buttons to the list*/
        for (int i = 0; i < sizeof(g_arr_drinks) / sizeof(char *); i++)
        {
            lv_obj_t *btn = lv_btn_create(g_list_drinks, NULL);
            lv_obj_set_width(btn, 180);
            lv_obj_set_event_cb(btn, event_handler_drinks);
            if (i == 0)
            {
                g_btn_current_drink = btn;
            }
            lv_obj_t *lbl = lv_label_create(btn, NULL);
            lv_obj_align(lbl, NULL, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text_fmt(lbl, "%s", g_arr_drinks[i]);
        }

        /*Select the first button by default*/
        lv_obj_set_state(g_btn_current_drink, LV_STATE_CHECKED);

        /*Create a second list with up and down buttons*/
        lv_obj_t *lbl_cups = lv_label_create(px_right_panel, NULL);
        lv_obj_align(lbl_cups, px_right_panel, LV_ALIGN_OUT_LEFT_TOP, 255, 20);
        lv_label_set_text_fmt(lbl_cups, "%s", "Choose Cups");

        g_list_no_cups = lv_list_create(px_right_panel, NULL);
        lv_obj_set_size(g_list_no_cups, 120, 200);
        lv_obj_align(g_list_no_cups, px_right_panel, LV_ALIGN_OUT_LEFT_TOP, 375, 50);
        lv_list_set_scrollbar_mode(g_list_no_cups, LV_SCROLLBAR_MODE_AUTO);

        for (int i = 0; i < sizeof(g_arr_nocups) / sizeof(char *); i++)
        {
            lv_obj_t *btn = lv_btn_create(g_list_no_cups, NULL);
            lv_obj_set_width(btn, 60);
            lv_obj_set_event_cb(btn, event_handler_nocups);
            if (i == 0)
            {
                g_btn_current_cup = btn;
            }
            lv_obj_t *lbl = lv_label_create(btn, NULL);
            lv_obj_align(lbl, NULL, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text_fmt(lbl, "%s", g_arr_nocups[i]);
        }

        lv_obj_set_state(g_btn_current_cup, LV_STATE_CHECKED);
        /* Start/Stop cooking button */
        g_px_imgbtn_start = lv_btn_create(px_right_panel, NULL);
        lv_obj_set_width(g_px_imgbtn_start, 100);
        lv_obj_align(g_px_imgbtn_start, px_right_panel, LV_ALIGN_OUT_LEFT_TOP, 350, 260);
        lv_obj_set_event_cb(g_px_imgbtn_start, v_GUI_Btn_Start_Event_Cb);
        lv_label_set_text(lv_label_create(g_px_imgbtn_start, NULL), "Make It");

        lv_obj_add_style(g_px_imgbtn_start, LV_LABEL_PART_MAIN, &x_style_menu_btn);

        /* Done */
        g_stru_screen.px_lv_screen = px_screen;
        g_b_initialized = true;
    }

    *ppstru_screen = &g_stru_screen;
    return GUI_OK;
}

static void event_handler_drinks(lv_obj_t *px_obj, lv_event_t enm_event)
{
    if (enm_event == LV_EVENT_CLICKED)
    {
        lv_obj_clear_state(g_btn_current_drink, LV_STATE_CHECKED);
        g_btn_current_drink = px_obj;
        lv_obj_set_state(px_obj, LV_STATE_CHECKED);
    }
}

static void event_handler_nocups(lv_obj_t *px_obj, lv_event_t enm_event)
{
    if (enm_event == LV_EVENT_CLICKED)
    {
        lv_obj_clear_state(g_btn_current_cup, LV_STATE_CHECKED);
        g_btn_current_cup = px_obj;
        lv_obj_set_state(px_obj, LV_STATE_CHECKED);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Starts roti making screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Start_Roti_Making_Screen(void)
{
    LOGD("Roti making screen started");
    g_stru_screen.enm_result = GUI_SCREEN_RESULT_NONE;

    /* Display SSID of the access point and Itor3's IP address */
    WIFIMN_cred_t *pstru_ap;
    WIFI_ip_info_t stru_ip_info;
    if ((s8_WIFIMN_Get_Selected_Ap(&pstru_ap, NULL) == WIFIMN_OK) &&
        (s8_WIFI_Get_IP_Info(&stru_ip_info) == WIFI_OK))
    {
        lv_label_set_text_fmt(g_px_lbl_ap, "%s [%d.%d.%d.%d]", pstru_ap->stri_ssid,
                              stru_ip_info.au8_ip[0], stru_ip_info.au8_ip[1],
                              stru_ip_info.au8_ip[2], stru_ip_info.au8_ip[3]);
    }
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Stops roti making screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Stop_Roti_Making_Screen(void)
{
    LOGD("Roti making screen stopped");
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs roti making screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Run_Roti_Making_Screen(void)
{
    /* Refresh Wifi status */
    static uint32_t u32_wifi_timer = 0;
    if (GUI_TIMER_ELAPSED(u32_wifi_timer) >= GUI_REFRESH_WIFI_CYCLE)
    {
        GUI_TIMER_RESET(u32_wifi_timer);

        /* Check wifi connection status */
        bool b_connected;
        if (s8_WIFIMN_Get_Selected_Ap(NULL, &b_connected) == WIFIMN_OK)
        {
            if (!b_connected)
            {
                /* Open splash screen */
                s8_GUI_Get_Screen(GUI_SCREEN_SPLASH, &g_stru_screen.pstru_next);
                g_stru_screen.enm_result = GUI_SCREEN_RESULT_NEXT;
                return GUI_OK;
            }
        }

        /* Wifi receive access point */
        WIFI_ap_info_t stru_ap_info;
        if (s8_WIFI_Get_Ap_Info(&stru_ap_info) == WIFI_OK)
        {
            if (stru_ap_info.s8_rssi < -90)
            {
                /* Unusable */
                lv_label_set_text(g_px_lbl_wifi_signal, "0");
            }
            else if (stru_ap_info.s8_rssi < -80)
            {
                /* Not Good */
                lv_label_set_text(g_px_lbl_wifi_signal, "2");
            }
            else if (stru_ap_info.s8_rssi < -70)
            {
                /* Okay */
                lv_label_set_text(g_px_lbl_wifi_signal, "4");
            }
            else
            {
                /* Very Good */
                lv_label_set_text(g_px_lbl_wifi_signal, "6");
            }
            lv_obj_align(g_px_lbl_wifi_signal, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
        }
    }

    /* Refresh GUI data if changed */
    static uint32_t u32_data_timer = 0;
    if (GUI_TIMER_ELAPSED(u32_data_timer) >= GUI_REFRESH_DATA_CYCLE)
    {
        GUI_TIMER_RESET(u32_data_timer);

        /* Refresh number of Roti's has been made */

        /* Refresh number of Roti's to be made */

        /* Refresh recipe name */

        /* Refresh flour name */
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of events occurring on wifi icon label
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Lbl_Wifi_Event_Cb(lv_obj_t *px_obj, lv_event_t enm_event)
{
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Open Wifi setting screen */
        s8_GUI_Get_Screen(GUI_SCREEN_WIFI_SETTING, &g_stru_screen.pstru_next);
        g_stru_screen.enm_result = GUI_SCREEN_RESULT_NEXT;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Event handler of button toggling cooking
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Btn_Start_Event_Cb(lv_obj_t *px_obj, lv_event_t enm_event)
{
    /* Only process click event */
    if (enm_event == LV_EVENT_CLICKED)
    {
        lv_obj_t *lbl = lv_obj_get_child(g_btn_current_drink, NULL);
        char *tmp = lv_label_get_text(lbl);
        for (uint8_t i = 0; i < sizeof(g_arr_drinks) / sizeof(char *); i++)
        {
            if (strcmp(tmp, g_arr_drinks[i]) == 0)
            {
                s8_GUI_Set_Data(GUI_DATA_DRINK_INDEX, tmp, strlen(tmp) + 1);
            }
        }

        lbl = lv_obj_get_child(g_btn_current_cup, NULL);
        tmp = lv_label_get_text(lbl);
        for (uint8_t i = 0; i < sizeof(g_arr_nocups) / sizeof(char *); i++)
        {
            if (strcmp(tmp, g_arr_nocups[i]) == 0)
            {
                s8_GUI_Set_Data(GUI_DATA_CUP_INDEX, tmp, strlen(tmp) + 1);
            }
        }

        /* Toggle cooking request */
        uint8_t u8_cooking_started = 0;
        if (s8_GUI_Get_Data(GUI_DATA_COOKING_STARTED, &u8_cooking_started, NULL) == GUI_OK)
        {
            u8_cooking_started = !u8_cooking_started;
            s8_GUI_Set_Data(GUI_DATA_COOKING_STARTED, &u8_cooking_started, sizeof(u8_cooking_started));
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of button "Menu" events
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Btn_Menu_Event_Cb(lv_obj_t *px_obj, lv_event_t enm_event)
{
    /* Only process click event */
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Open Menu screen */
        s8_GUI_Get_Screen(GUI_SCREEN_MENU, &g_stru_screen.pstru_next);
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
