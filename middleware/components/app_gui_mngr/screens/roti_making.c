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

#include "screen_common.h"              /* Common header of all screens */
#include "app_wifi_mngr.h"              /* Use Wifi Manager module */
#include "srvc_wifi.h"                  /* Use Wifi service */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Maximum roast level */
#define GUI_MAX_ROAST_LEVEL             5

/** @brief  Maximum thickness level */
#define GUI_MAX_THICKNESS_LEVEL         5

/** @brief  Maximum oil level */
#define GUI_MAX_OIL_LEVEL               2

/** @brief  Cycle (in milliseconds) polling GUI data */
#define GUI_REFRESH_DATA_CYCLE          50

/** @brief  Cycle (in milliseconds) polling Wifi status */
#define GUI_REFRESH_WIFI_CYCLE          1000

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static int8_t s8_GUI_Start_Roti_Making_Screen (void);
static int8_t s8_GUI_Stop_Roti_Making_Screen (void);
static int8_t s8_GUI_Run_Roti_Making_Screen (void);
static void v_GUI_Lbl_Wifi_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Roast_Level_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Thickness_Level_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Oil_Level_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Btn_Start_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Btn_Minus_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Btn_Plus_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Btn_Menu_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);

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
    .pstri_name         = "Roti Making",
    .px_icon            = NULL,
    .pfnc_start         = s8_GUI_Start_Roti_Making_Screen,
    .pfnc_stop          = s8_GUI_Stop_Roti_Making_Screen,
    .pfnc_run           = s8_GUI_Run_Roti_Making_Screen,
    .enm_result         = GUI_SCREEN_RESULT_NONE,
};

/** @brief  Image for start button */
LV_IMG_DECLARE (img_play);

/** @brief  Image for pause button */
LV_IMG_DECLARE (img_pause);

/** @brief  Image for plus button */
LV_IMG_DECLARE (img_plus);

/** @brief  Image for minus button */
LV_IMG_DECLARE (img_minus);

/** @brief  Font for Roti count */
LV_FONT_DECLARE (arial_96);

/** @brief  Bold font */
LV_FONT_DECLARE (arial_bold_18);

/** @brief  Font for Wifi symbol */
LV_FONT_DECLARE (wifi_symbol);

/** @brief  Label of wifi signal symbol */
static lv_obj_t * g_px_lbl_wifi_signal = NULL;

/** @brief  Label of wifi access point name */
static lv_obj_t * g_px_lbl_ap = NULL;

/** @brief  Array of objects for roast levels */
static lv_obj_t * g_pax_roast_level[GUI_MAX_ROAST_LEVEL];

/** @brief  Array of objects for thickness levels */
static lv_obj_t * g_pax_thickness_level[GUI_MAX_THICKNESS_LEVEL];

/** @brief  Array of objects for oil levels */
static lv_obj_t * g_pax_oil_level[GUI_MAX_OIL_LEVEL];

/** @brief  Label for cooking status */
static lv_obj_t * g_px_lbl_status = NULL;

/** @brief  Label for number of Roti's has been made */
static lv_obj_t * g_px_lbl_roti_made = NULL;

/** @brief  Label for number of Roti's to be made */
static lv_obj_t * g_px_lbl_roti_count = NULL;

/** @brief  Image button to start/stop cooking */
static lv_obj_t * g_px_imgbtn_start;

/** @brief  Label of recipe name */
static lv_obj_t * g_px_lbl_recipe;

/** @brief  Label of flour name */
static lv_obj_t * g_px_lbl_flour;

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
int8_t s8_GUI_Get_Roti_Making_Screen (GUI_screen_t ** ppstru_screen)
{
    ASSERT_PARAM (ppstru_screen != NULL);

    /* If this screen has not been initialized, initialize it */
    if (!g_b_initialized)
    {
        lv_obj_t * px_screen = lv_obj_create (NULL, NULL);

        /* Left panel */
        lv_obj_t * px_left_panel = lv_obj_create (px_screen, NULL);
        lv_obj_set_size (px_left_panel, 120, LV_VER_RES);

        static lv_style_t x_style_left_panel;
        lv_style_init (&x_style_left_panel);
        lv_style_set_bg_color (&x_style_left_panel, LV_STATE_DEFAULT, LV_COLOR_MAKE (246, 246, 246));
        lv_style_set_radius (&x_style_left_panel, LV_STATE_DEFAULT, 0);
        lv_style_set_border_width (&x_style_left_panel, LV_STATE_DEFAULT, 0);
        lv_obj_add_style (px_left_panel, LV_LABEL_PART_MAIN, &x_style_left_panel);

        /* Menu button */
        lv_obj_t * px_btn_menu = lv_btn_create (px_left_panel, NULL);
        lv_obj_set_size (px_btn_menu, 100, 40);
        lv_obj_align (px_btn_menu, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
        lv_obj_set_event_cb (px_btn_menu, v_GUI_Btn_Menu_Event_Cb);
        lv_label_set_text (lv_label_create (px_btn_menu, NULL), "MENU");

        static lv_style_t x_style_menu_btn;
        lv_style_init (&x_style_menu_btn);
        lv_style_set_radius (&x_style_menu_btn, LV_STATE_DEFAULT, 8);
        lv_style_set_bg_color (&x_style_menu_btn, LV_STATE_DEFAULT, LV_THEME_DEFAULT_COLOR_PRIMARY);
        lv_style_set_border_width (&x_style_menu_btn, LV_STATE_DEFAULT, 0);
        lv_style_set_text_color (&x_style_menu_btn, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_style_set_text_font (&x_style_menu_btn, LV_STATE_DEFAULT, &arial_bold_18);
        lv_obj_add_style (px_btn_menu, LV_LABEL_PART_MAIN, &x_style_menu_btn);

        /* Style of text in left panel */
        static lv_style_t x_style_left_panel_text;
        lv_style_init (&x_style_left_panel_text);
        lv_style_set_text_font (&x_style_left_panel_text, LV_STATE_DEFAULT, &lv_font_montserrat_18);
        lv_style_set_text_color (&x_style_left_panel_text, LV_STATE_DEFAULT, LV_THEME_DEFAULT_COLOR_PRIMARY);

        /* Style of roast/thickness/oil levels */
        static lv_style_t x_style_level;
        lv_style_init (&x_style_level);
        lv_style_set_radius (&x_style_level, LV_STATE_DEFAULT, 0);
        lv_style_set_border_color (&x_style_level, LV_STATE_DEFAULT, LV_THEME_DEFAULT_COLOR_PRIMARY);
        lv_style_set_border_width (&x_style_level, LV_STATE_DEFAULT, 1);

        /* Background for roast level */
        lv_obj_t * px_roast_background = lv_obj_create (px_left_panel, NULL);
        lv_obj_set_size (px_roast_background, 97, 45);
        lv_obj_add_style (px_roast_background, LV_LABEL_PART_MAIN, &x_style_left_panel);
        lv_obj_align (px_roast_background, NULL, LV_ALIGN_IN_TOP_LEFT, 11, 80);
        lv_obj_set_event_cb (px_roast_background, v_GUI_Roast_Level_Event_Cb);

        /* "Roast" label */
        lv_obj_t * px_lbl_roast = lv_label_create (px_roast_background, NULL);
        lv_label_set_text (px_lbl_roast, "Roast");
        lv_obj_add_style (px_lbl_roast, LV_LABEL_PART_MAIN, &x_style_left_panel_text);

        /* Roast level */
        uint8_t u8_roast_level = 1;
        s8_GUI_Get_Data (GUI_DATA_ROAST_LEVEL, &u8_roast_level, NULL);
        for (uint8_t u8_idx = 0; u8_idx < GUI_MAX_ROAST_LEVEL; u8_idx++)
        {
            g_pax_roast_level[u8_idx] = lv_obj_create (px_roast_background, NULL);
            lv_obj_set_size (g_pax_roast_level[u8_idx], 17, 17);
            if (u8_idx == 0)
            {
                lv_obj_align (g_pax_roast_level[u8_idx], px_lbl_roast, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
            }
            else
            {
                lv_obj_align (g_pax_roast_level[u8_idx], g_pax_roast_level[u8_idx - 1], LV_ALIGN_OUT_RIGHT_MID, 3, 0);
            }
            lv_obj_set_click (g_pax_roast_level[u8_idx], false);
            lv_obj_add_style (g_pax_roast_level[u8_idx], LV_LABEL_PART_MAIN, &x_style_level);
        }

        /* Background for thickness level */
        lv_obj_t * px_thickness_background = lv_obj_create (px_left_panel, NULL);
        lv_obj_set_size (px_thickness_background, 97, 45);
        lv_obj_add_style (px_thickness_background, LV_LABEL_PART_MAIN, &x_style_left_panel);
        lv_obj_align (px_thickness_background, NULL, LV_ALIGN_IN_TOP_LEFT, 11, 150);
        lv_obj_set_event_cb (px_thickness_background, v_GUI_Thickness_Level_Event_Cb);

        /* "Thickness" label */
        lv_obj_t * px_lbl_thickness = lv_label_create (px_thickness_background, NULL);
        lv_label_set_text (px_lbl_thickness, "Thickness");
        lv_obj_add_style (px_lbl_thickness, LV_LABEL_PART_MAIN, &x_style_left_panel_text);

        /* Thickness level */
        uint8_t u8_thickness_level = 1;
        s8_GUI_Get_Data (GUI_DATA_THICKNESS_LEVEL, &u8_thickness_level, NULL);
        for (uint8_t u8_idx = 0; u8_idx < GUI_MAX_THICKNESS_LEVEL; u8_idx++)
        {
            g_pax_thickness_level[u8_idx] = lv_obj_create (px_thickness_background, NULL);
            lv_obj_set_size (g_pax_thickness_level[u8_idx], 17, 17);
            if (u8_idx == 0)
            {
                lv_obj_align (g_pax_thickness_level[u8_idx], px_lbl_thickness, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
            }
            else
            {
                lv_obj_align (g_pax_thickness_level[u8_idx], g_pax_thickness_level[u8_idx - 1],
                              LV_ALIGN_OUT_RIGHT_MID, 3, 0);
            }
            lv_obj_set_click (g_pax_thickness_level[u8_idx], false);
            lv_obj_add_style (g_pax_thickness_level[u8_idx], LV_LABEL_PART_MAIN, &x_style_level);
        }

        /* Background for oil level */
        lv_obj_t * px_oil_background = lv_obj_create (px_left_panel, NULL);
        lv_obj_set_size (px_oil_background, 97, 45);
        lv_obj_add_style (px_oil_background, LV_LABEL_PART_MAIN, &x_style_left_panel);
        lv_obj_align (px_oil_background, NULL, LV_ALIGN_IN_TOP_LEFT, 11, 220);
        lv_obj_set_event_cb (px_oil_background, v_GUI_Oil_Level_Event_Cb);

        /* "Oil" label */
        lv_obj_t * px_lbl_oil = lv_label_create (px_oil_background, NULL);
        lv_label_set_text (px_lbl_oil, "Oil");
        lv_obj_add_style (px_lbl_oil, LV_LABEL_PART_MAIN, &x_style_left_panel_text);

        /* Oil level */
        uint8_t u8_oil_level = 1;
        s8_GUI_Get_Data (GUI_DATA_OIL_LEVEL, &u8_oil_level, NULL);
        for (uint8_t u8_idx = 0; u8_idx < GUI_MAX_OIL_LEVEL; u8_idx++)
        {
            g_pax_oil_level[u8_idx] = lv_obj_create (px_oil_background, NULL);
            lv_obj_set_size (g_pax_oil_level[u8_idx], 17, 17);
            if (u8_idx == 0)
            {
                lv_obj_align (g_pax_oil_level[u8_idx], px_lbl_oil, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
            }
            else
            {
                lv_obj_align (g_pax_oil_level[u8_idx], g_pax_oil_level[u8_idx - 1], LV_ALIGN_OUT_RIGHT_MID, 3, 0);
            }
            lv_obj_set_click (g_pax_oil_level[u8_idx], false);
            lv_obj_add_style (g_pax_oil_level[u8_idx], LV_LABEL_PART_MAIN, &x_style_level);
        }

        /* Right panel */
        lv_obj_t * px_right_panel = lv_obj_create (px_screen, NULL);
        lv_obj_set_size (px_right_panel, 360, LV_VER_RES);
        lv_obj_set_pos (px_right_panel, 120, 0);

        static lv_style_t x_style_right_panel;
        lv_style_init (&x_style_right_panel);
        lv_style_set_bg_color (&x_style_right_panel, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_style_set_radius (&x_style_right_panel, LV_STATE_DEFAULT, 0);
        lv_style_set_border_width (&x_style_right_panel, LV_STATE_DEFAULT, 0);
        lv_obj_add_style (px_right_panel, LV_LABEL_PART_MAIN, &x_style_right_panel);

        /* Background of Wifi singal */
        lv_obj_t * px_wifi_background = lv_obj_create (px_right_panel, NULL);
        lv_obj_set_size (px_wifi_background, 50, 35);
        lv_obj_align (px_wifi_background, NULL, LV_ALIGN_IN_TOP_RIGHT, -10, 10);
        lv_obj_set_click (px_wifi_background, true);
        lv_obj_set_event_cb (px_wifi_background, v_GUI_Lbl_Wifi_Event_Cb);
        lv_obj_add_style (px_wifi_background, LV_LABEL_PART_MAIN, &x_style_right_panel);

        /* Style for wifi symbol */
        static lv_style_t x_style_wifi_symbol;
        lv_style_init (&x_style_wifi_symbol);
        lv_style_set_text_font (&x_style_wifi_symbol, LV_STATE_DEFAULT, &wifi_symbol);
        lv_style_set_text_color (&x_style_wifi_symbol, LV_STATE_DEFAULT, LV_COLOR_MAKE (0xE0, 0xE0, 0xE0));

        /* Background of Wifi signal symbol */
        lv_obj_t * px_lbl_wifi_background = lv_label_create (px_wifi_background, NULL);
        lv_obj_add_style (px_lbl_wifi_background, LV_LABEL_PART_MAIN, &x_style_wifi_symbol);
        lv_label_set_text (px_lbl_wifi_background, "6");
        lv_obj_align (px_lbl_wifi_background, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

        /* Wifi signal symbol */
        g_px_lbl_wifi_signal = lv_label_create (px_wifi_background, NULL);
        lv_obj_add_style (g_px_lbl_wifi_signal, LV_LABEL_PART_MAIN, &x_style_wifi_symbol);
        _lv_obj_set_style_local_color (g_px_lbl_wifi_signal, LV_LABEL_PART_MAIN, LV_STYLE_TEXT_COLOR,
                                       LV_THEME_DEFAULT_COLOR_PRIMARY);

        /* Wifi access point name */
        g_px_lbl_ap = lv_label_create (px_wifi_background, NULL);
        lv_label_set_long_mode (g_px_lbl_ap, LV_LABEL_LONG_SROLL_CIRC);
        lv_obj_set_width (g_px_lbl_ap, 50);

        static lv_style_t x_style_ap;
        lv_style_init (&x_style_ap);
        lv_style_set_text_font (&x_style_ap, LV_STATE_DEFAULT, &lv_font_montserrat_10);
        lv_style_set_text_color (&x_style_ap, LV_STATE_DEFAULT, LV_COLOR_GRAY);
        lv_obj_add_style (g_px_lbl_ap, LV_LABEL_PART_MAIN, &x_style_ap);
        lv_obj_align (g_px_lbl_ap, px_lbl_wifi_background, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);

        /* Cooking status label */
        g_px_lbl_status = lv_label_create (px_right_panel, NULL);
        static lv_style_t x_style_status;
        lv_style_init (&x_style_status);
        lv_style_set_text_letter_space (&x_style_status, LV_STATE_DEFAULT, 2);
        lv_obj_add_style (g_px_lbl_status, LV_LABEL_PART_MAIN, &x_style_status);

        /* Number of Roti's has been made */
        g_px_lbl_roti_made = lv_label_create (px_right_panel, NULL);
        static lv_style_t x_style_roti_count;
        lv_style_init (&x_style_roti_count);
        lv_style_set_text_font (&x_style_roti_count, LV_STATE_DEFAULT, &arial_96);
        lv_style_set_text_color (&x_style_roti_count, LV_STATE_DEFAULT, LV_COLOR_BLACK);
        lv_obj_add_style (g_px_lbl_roti_made, LV_LABEL_PART_MAIN, &x_style_roti_count);

        /* "of" label for Roti count */
        lv_obj_t * px_lbl_of = lv_label_create (px_right_panel, NULL);
        lv_label_set_text (px_lbl_of, "of");
        lv_obj_align (px_lbl_of, NULL, LV_ALIGN_IN_TOP_MID, 0, 120);

        /* Number of Roti's to be made */
        g_px_lbl_roti_count = lv_label_create (px_right_panel, NULL);
        lv_obj_add_style (g_px_lbl_roti_count, LV_LABEL_PART_MAIN, &x_style_roti_count);

        /* Style for image buttons, darken the buttons when pressed */
        static lv_style_t x_style_imgbtn;
        lv_style_init (&x_style_imgbtn);
        lv_style_set_image_recolor_opa (&x_style_imgbtn, LV_STATE_PRESSED, LV_OPA_30);
        lv_style_set_image_recolor (&x_style_imgbtn, LV_STATE_PRESSED, LV_COLOR_BLACK);

        /* Start/Stop cooking button */
        g_px_imgbtn_start = lv_imgbtn_create (px_right_panel, NULL);
        lv_obj_add_style (g_px_imgbtn_start, LV_IMGBTN_PART_MAIN, &x_style_imgbtn);
        lv_imgbtn_set_src (g_px_imgbtn_start, LV_BTN_STATE_RELEASED, &img_play);
        lv_obj_align (g_px_imgbtn_start, NULL, LV_ALIGN_IN_TOP_MID, 0, 150);
        lv_obj_set_event_cb (g_px_imgbtn_start, v_GUI_Btn_Start_Event_Cb);

        /* Minus button */
        lv_obj_t * px_imgbtn_minus = lv_imgbtn_create (px_right_panel, NULL);
        lv_obj_add_style (px_imgbtn_minus, LV_IMGBTN_PART_MAIN, &x_style_imgbtn);
        lv_imgbtn_set_src (px_imgbtn_minus, LV_BTN_STATE_RELEASED, &img_minus);
        lv_obj_align (px_imgbtn_minus, g_px_imgbtn_start, LV_ALIGN_OUT_LEFT_MID, -20, 0);
        lv_obj_set_event_cb (px_imgbtn_minus, v_GUI_Btn_Minus_Event_Cb);

        /* Plus button */
        lv_obj_t * px_imgbtn_plus = lv_imgbtn_create (px_right_panel, NULL);
        lv_obj_add_style (px_imgbtn_plus, LV_IMGBTN_PART_MAIN, &x_style_imgbtn);
        lv_imgbtn_set_src (px_imgbtn_plus, LV_BTN_STATE_RELEASED, &img_plus);
        lv_obj_align (px_imgbtn_plus, g_px_imgbtn_start, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
        lv_obj_set_event_cb (px_imgbtn_plus, v_GUI_Btn_Plus_Event_Cb);

        /* Label of recipe name */
        g_px_lbl_recipe = lv_label_create (px_right_panel, NULL);
        static lv_style_t x_style_recipe;
        lv_style_init (&x_style_recipe);
        lv_style_set_text_font (&x_style_recipe, LV_STATE_DEFAULT, &arial_bold_18);
        lv_obj_add_style (g_px_lbl_recipe, LV_LABEL_PART_MAIN, &x_style_recipe);

        /* Label of flour name */
        g_px_lbl_flour = lv_label_create (px_right_panel, NULL);
        lv_label_set_long_mode (g_px_lbl_flour, LV_LABEL_LONG_SROLL_CIRC);
        lv_obj_set_width (g_px_lbl_flour, 300);
        lv_label_set_align (g_px_lbl_flour, LV_LABEL_ALIGN_CENTER);

        static lv_style_t x_style_flour;
        lv_style_init (&x_style_flour);
        lv_style_set_text_color (&x_style_flour, LV_STATE_DEFAULT, LV_COLOR_GRAY);
        lv_obj_add_style (g_px_lbl_flour, LV_LABEL_PART_MAIN, &x_style_flour);

        /* Done */
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
**      Starts roti making screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Start_Roti_Making_Screen (void)
{
    LOGD("Roti making screen started");
    g_stru_screen.enm_result = GUI_SCREEN_RESULT_NONE;

    /* Display SSID of the access point and Itor3's IP address */
    WIFIMN_cred_t * pstru_ap;
    WIFI_ip_info_t stru_ip_info;
    if ((s8_WIFIMN_Get_Selected_Ap (&pstru_ap, NULL) == WIFIMN_OK) &&
        (s8_WIFI_Get_IP_Info (&stru_ip_info) == WIFI_OK))
    {
        lv_label_set_text_fmt (g_px_lbl_ap, "%s [%d.%d.%d.%d]", pstru_ap->stri_ssid,
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
static int8_t s8_GUI_Stop_Roti_Making_Screen (void)
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
static int8_t s8_GUI_Run_Roti_Making_Screen (void)
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
                /* Open splash screen */
                s8_GUI_Get_Screen (GUI_SCREEN_SPLASH, &g_stru_screen.pstru_next);
                g_stru_screen.enm_result = GUI_SCREEN_RESULT_NEXT;
                return GUI_OK;
            }
        }

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
            lv_obj_align (g_px_lbl_wifi_signal, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
        }
    }

    /* Refresh GUI data if changed */
    static uint32_t u32_data_timer = 0;
    if (GUI_TIMER_ELAPSED (u32_data_timer) >= GUI_REFRESH_DATA_CYCLE)
    {
        GUI_TIMER_RESET (u32_data_timer);

        /* Refresh roast level */
        uint8_t u8_roast_level = 0;
        if (s8_GUI_Get_Data_If_Changed (GUI_DATA_ROAST_LEVEL, &u8_roast_level, NULL) == GUI_OK)
        {
            for (uint8_t u8_idx = 0; u8_idx < GUI_MAX_ROAST_LEVEL; u8_idx++)
            {
                _lv_obj_set_style_local_color (g_pax_roast_level[u8_idx], LV_LABEL_PART_MAIN, LV_STYLE_BG_COLOR,
                            (u8_idx < u8_roast_level) ? LV_THEME_DEFAULT_COLOR_PRIMARY : LV_COLOR_WHITE);
            }
        }

        /* Refresh thickness level */
        uint8_t u8_thickness_level = 0;
        if (s8_GUI_Get_Data_If_Changed (GUI_DATA_THICKNESS_LEVEL, &u8_thickness_level, NULL) == GUI_OK)
        {
            for (uint8_t u8_idx = 0; u8_idx < GUI_MAX_THICKNESS_LEVEL; u8_idx++)
            {
                _lv_obj_set_style_local_color (g_pax_thickness_level[u8_idx], LV_LABEL_PART_MAIN, LV_STYLE_BG_COLOR,
                            (u8_idx < u8_thickness_level) ? LV_THEME_DEFAULT_COLOR_PRIMARY : LV_COLOR_WHITE);
            }
        }

        /* Refresh oil level */
        uint8_t u8_oil_level = 0;
        if (s8_GUI_Get_Data_If_Changed (GUI_DATA_OIL_LEVEL, &u8_oil_level, NULL) == GUI_OK)
        {
            for (uint8_t u8_idx = 0; u8_idx < GUI_MAX_OIL_LEVEL; u8_idx++)
            {
                _lv_obj_set_style_local_color (g_pax_oil_level[u8_idx], LV_LABEL_PART_MAIN, LV_STYLE_BG_COLOR,
                            (u8_idx < u8_oil_level) ? LV_THEME_DEFAULT_COLOR_PRIMARY : LV_COLOR_WHITE);
            }
        }

        /* Refresh number of Roti's has been made */
        uint8_t u8_roti_made = 0;
        if (s8_GUI_Get_Data_If_Changed (GUI_DATA_ROTI_MADE, &u8_roti_made, NULL) == GUI_OK)
        {
            lv_label_set_text_fmt (g_px_lbl_roti_made, "%d", u8_roti_made);
            lv_obj_align (g_px_lbl_roti_made, NULL, LV_ALIGN_IN_RIGHT_MID, -195, -60);
        }

        /* Refresh number of Roti's to be made */
        uint8_t u8_roti_count = 0;
        if (s8_GUI_Get_Data_If_Changed (GUI_DATA_ROTI_COUNT, &u8_roti_count, NULL) == GUI_OK)
        {
            lv_label_set_text_fmt (g_px_lbl_roti_count, "%d", u8_roti_count);
            lv_obj_align (g_px_lbl_roti_count, NULL, LV_ALIGN_IN_LEFT_MID, 195, -60);
        }

        /* Refresh recipe name */
        char stri_buf[64];
        uint16_t u16_len = sizeof (stri_buf);
        if (s8_GUI_Get_Data_If_Changed (GUI_DATA_RECIPE_NAME, stri_buf, &u16_len) == GUI_OK)
        {
            lv_label_set_text (g_px_lbl_recipe, stri_buf);
            lv_obj_align (g_px_lbl_recipe, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -50);
        }

        /* Refresh flour name */
        u16_len = sizeof (stri_buf);
        if (s8_GUI_Get_Data_If_Changed (GUI_DATA_FLOUR_NAME, stri_buf, &u16_len) == GUI_OK)
        {
            lv_label_set_text (g_px_lbl_flour, stri_buf);
            lv_obj_align (g_px_lbl_flour, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -25);
        }

        /* Refresh cooking state */
        uint8_t u8_cooking_state = 0;
        if (s8_GUI_Get_Data_If_Changed (GUI_DATA_COOKING_STATE, &u8_cooking_state, NULL) == GUI_OK)
        {
            /* If idle */
            if (u8_cooking_state == 0)
            {
                lv_label_set_text (g_px_lbl_status, "LET'S GET COOKING!");
                lv_obj_align (g_px_lbl_status, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
                lv_imgbtn_set_src (g_px_imgbtn_start, LV_BTN_STATE_RELEASED, &img_play);
            }

            /* If cooking */
            else if (u8_cooking_state == 1)
            {
                lv_label_set_text (g_px_lbl_status, "COOKING...");
                lv_obj_align (g_px_lbl_status, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
                lv_imgbtn_set_src (g_px_imgbtn_start, LV_BTN_STATE_RELEASED, &img_pause);
            }
        }
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
static void v_GUI_Lbl_Wifi_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
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
**      Handler of events occurring on Roast level background
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Roast_Level_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* Only process click event */
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Increase roast level */
        uint8_t u8_roast_level = 0;
        if (s8_GUI_Get_Data (GUI_DATA_ROAST_LEVEL, &u8_roast_level, NULL) == GUI_OK)
        {
            u8_roast_level = (u8_roast_level < GUI_MAX_ROAST_LEVEL) ? u8_roast_level + 1 : 1;
            s8_GUI_Set_Data (GUI_DATA_ROAST_LEVEL, &u8_roast_level, sizeof (u8_roast_level));
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of events occurring on Thickness level background
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Thickness_Level_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* Only process click event */
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Increase thickness level */
        uint8_t u8_thickness_level = 0;
        if (s8_GUI_Get_Data (GUI_DATA_THICKNESS_LEVEL, &u8_thickness_level, NULL) == GUI_OK)
        {
            u8_thickness_level = (u8_thickness_level < GUI_MAX_THICKNESS_LEVEL) ? u8_thickness_level + 1 : 1;
            s8_GUI_Set_Data (GUI_DATA_THICKNESS_LEVEL, &u8_thickness_level, sizeof (u8_thickness_level));
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of events occurring on Oil level background
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Oil_Level_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* Only process click event */
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Increase oil level */
        uint8_t u8_oil_level = 0;
        if (s8_GUI_Get_Data (GUI_DATA_OIL_LEVEL, &u8_oil_level, NULL) == GUI_OK)
        {
            u8_oil_level = (u8_oil_level < GUI_MAX_OIL_LEVEL) ? u8_oil_level + 1 : 1;
            s8_GUI_Set_Data (GUI_DATA_OIL_LEVEL, &u8_oil_level, sizeof (u8_oil_level));
        }
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
static void v_GUI_Btn_Start_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* Only process click event */
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Toggle cooking request */
        uint8_t u8_cooking_started = 0;
        if (s8_GUI_Get_Data (GUI_DATA_COOKING_STARTED, &u8_cooking_started, NULL) == GUI_OK)
        {
            u8_cooking_started = !u8_cooking_started;
            s8_GUI_Set_Data (GUI_DATA_COOKING_STARTED, &u8_cooking_started, sizeof (u8_cooking_started));
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Event handler of button minus
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Btn_Minus_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* Only process click event */
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Decrease number of roti's to be made */
        uint8_t u8_roti_count = 0;
        if (s8_GUI_Get_Data (GUI_DATA_ROTI_COUNT, &u8_roti_count, NULL) == GUI_OK)
        {
            u8_roti_count = (u8_roti_count > 1) ? u8_roti_count - 1 : 1;
            s8_GUI_Set_Data (GUI_DATA_ROTI_COUNT, &u8_roti_count, sizeof (u8_roti_count));
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Event handler of button plus
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Btn_Plus_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* Only process click event */
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Increase number of roti's to be made */
        uint8_t u8_roti_count = 0;
        if (s8_GUI_Get_Data (GUI_DATA_ROTI_COUNT, &u8_roti_count, NULL) == GUI_OK)
        {
            u8_roti_count++;
            s8_GUI_Set_Data (GUI_DATA_ROTI_COUNT, &u8_roti_count, sizeof (u8_roti_count));
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
static void v_GUI_Btn_Menu_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* Only process click event */
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Open Menu screen */
        s8_GUI_Get_Screen (GUI_SCREEN_MENU, &g_stru_screen.pstru_next);
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
