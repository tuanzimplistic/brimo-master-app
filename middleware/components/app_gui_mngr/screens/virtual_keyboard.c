/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : virtual_keyboard.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 June 1
**  @brief      : Implementation of on-screen keyboard
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       Implements on-screen keyboard
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "screen_common.h"              /* Common header of all screens */
#include "virtual_keyboard.h"           /* Internal public header of this screen */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static int8_t s8_GUI_Start_Virtual_Keyboard_Screen (void);
static int8_t s8_GUI_Stop_Virutal_Keyboard_Screen (void);
static int8_t s8_GUI_Run_Virtual_Keyboard_Screen (void);
static void v_GUI_Vir_Kb_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static void v_GUI_Img_Visibility_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);

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
    .pstri_name         = "Keyboard",
    .px_icon            = NULL,
    .pfnc_start         = s8_GUI_Start_Virtual_Keyboard_Screen,
    .pfnc_stop          = s8_GUI_Stop_Virutal_Keyboard_Screen,
    .pfnc_run           = s8_GUI_Run_Virtual_Keyboard_Screen,
    .enm_result         = GUI_SCREEN_RESULT_NONE,
};

/** @brief  Declare image showing password text */
LV_IMG_DECLARE (img_visible);

/** @brief  Declare image hiding password text */
LV_IMG_DECLARE (img_invisible);

/** @brief  Brief description label */
static lv_obj_t * g_px_lbl_brief = NULL;

/** @brief  Input text area */
static lv_obj_t * g_px_txt_input = NULL;

/** @brief  Image showing/hiding password text */
static lv_obj_t * g_px_img_visibility = NULL;

/** @brief  Configuration of virtual keyboard */
static GUI_virkb_cfg_t g_stru_config = GUI_VIRKB_DEFAULT_CFG;

/** @brief  Indicates if the configuration structure is valid */
static bool g_b_config_valid = false;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets structure wrapping virtual keyboard screen
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
int8_t s8_GUI_Get_Virtual_Keyboard_Screen (GUI_screen_t ** ppstru_screen)
{
    ASSERT_PARAM (ppstru_screen != NULL);

    /* If this screen has not been initialized, initialize it */
    if (!g_b_initialized)
    {
        lv_obj_t * px_screen = lv_obj_create (NULL, NULL);

        /* Brief description label */
        g_px_lbl_brief = lv_label_create (px_screen, NULL);
        lv_obj_align (g_px_lbl_brief, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);

        /* Image showing/hiding password text */
        g_px_img_visibility = lv_img_create (px_screen, NULL);
        lv_img_set_src (g_px_img_visibility, &img_visible);
        lv_obj_align (g_px_img_visibility, NULL, LV_ALIGN_IN_TOP_RIGHT, -30, 35);
        lv_obj_set_click (g_px_img_visibility, true);
        lv_obj_set_event_cb (g_px_img_visibility, v_GUI_Img_Visibility_Event_Cb);

        /* Text area */
        g_px_txt_input = lv_textarea_create (px_screen, NULL);
        lv_obj_align (g_px_txt_input, g_px_lbl_brief, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
        lv_obj_set_size (g_px_txt_input, LV_HOR_RES - 2 * lv_obj_get_x (g_px_txt_input),
                         3 * LV_VER_RES / 10 - lv_obj_get_y (g_px_txt_input) - 10);

        /* Virtual keyboard */
        lv_obj_t * px_kb = lv_keyboard_create (px_screen, NULL);
        lv_coord_t x_max_height = 7 * LV_VER_RES / 10;
        lv_obj_set_height (px_kb, x_max_height);
        lv_keyboard_set_textarea (px_kb, g_px_txt_input);
        lv_keyboard_set_cursor_manage (px_kb, true);
        lv_obj_align (px_kb, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
        lv_obj_set_event_cb (px_kb, v_GUI_Vir_Kb_Event_Cb);

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
**      Configures the virtual keyboard before it's displayed
**
** @param [in]
**      pstru_config: Pointer to the configuration structure
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Virkb_Set_Config (const GUI_virkb_cfg_t * pstru_config)
{
    ASSERT_PARAM (pstru_config != NULL);

    /* Store the configuration */
    g_stru_config = *pstru_config;
    g_b_config_valid = true;

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets the text that has been input
**
** @param [out]
**      ppstri_text: Pointer to the text
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Virkb_Get_Text (const char ** ppstri_text)
{
    ASSERT_PARAM (ppstri_text != NULL);

    /* Get pointer to the text */
    *ppstri_text = lv_textarea_get_text (g_px_txt_input);

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Starts virtual keyboard screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Start_Virtual_Keyboard_Screen (void)
{
    LOGD ("Virtual keyboard screen started");
    g_stru_screen.enm_result = GUI_SCREEN_RESULT_NONE;

    /* If user doesn't configure keyboad screen, use default configuration */
    if (!g_b_config_valid)
    {
        GUI_virkb_cfg_t stru_default_cfg = GUI_VIRKB_DEFAULT_CFG;
        g_stru_config = stru_default_cfg;
    }
    else
    {
        g_b_config_valid = false;
    }

    /* Password mode */
    lv_textarea_set_pwd_mode (g_px_txt_input, g_stru_config.b_password_mode);
    lv_textarea_set_one_line (g_px_txt_input, g_stru_config.b_password_mode);
    if (g_stru_config.b_password_mode)
    {
        lv_obj_set_width (g_px_txt_input, LV_HOR_RES - 2 * lv_obj_get_x (g_px_txt_input) - 80);
        lv_obj_set_hidden (g_px_img_visibility, false);
        lv_img_set_src (g_px_img_visibility, &img_invisible);
    }
    else
    {
        lv_obj_set_width (g_px_txt_input, LV_HOR_RES - 2 * lv_obj_get_x (g_px_txt_input));
        lv_obj_set_hidden (g_px_img_visibility, true);
    }

    /* Max text length */
    lv_textarea_set_max_length (g_px_txt_input, g_stru_config.u16_max_text_len);

    /* Initialized input text */
    if (g_stru_config.pstri_init_text != NULL)
    {
        lv_textarea_set_text (g_px_txt_input, g_stru_config.pstri_init_text);
        if (g_stru_config.b_password_mode)
        {
            /* This trick is to stop the text area to display the final character of the password */
            lv_textarea_add_char (g_px_txt_input, '*');
            lv_textarea_del_char (g_px_txt_input);
        }
    }
    else
    {
        lv_textarea_set_text (g_px_txt_input, "");
    }

    /* Brief description about the text to input */
    if (g_stru_config.pstri_brief != NULL)
    {
        lv_label_set_text (g_px_lbl_brief, g_stru_config.pstri_brief);
    }
    else
    {
        lv_label_set_text (g_px_lbl_brief, "");
    }

    /* Accepted characters */
    if (g_stru_config.pstri_accepted_chars != NULL)
    {
        lv_textarea_set_accepted_chars (g_px_txt_input, g_stru_config.pstri_accepted_chars);
    }
    else
    {
        lv_textarea_set_accepted_chars (g_px_txt_input, NULL);
    }

    /* Placeholder text */
    if (g_stru_config.pstri_placeholder != NULL)
    {
        lv_textarea_set_placeholder_text (g_px_txt_input, g_stru_config.pstri_placeholder);
    }
    else
    {
        lv_textarea_set_placeholder_text (g_px_txt_input, "");
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Stops virtual keyboard screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Stop_Virutal_Keyboard_Screen (void)
{
    LOGD ("Virtual keyboard screen stopped");
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs virtual keyboard screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Run_Virtual_Keyboard_Screen (void)
{
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handles event of on-screen keyboard
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Vir_Kb_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* If user clicked Close (cancel) button on the keyboard */
    if (enm_event == LV_EVENT_CANCEL)
    {
        /* Cancel changes and restore the initial text */
        if (g_stru_config.pstri_init_text != NULL)
        {
            lv_textarea_set_text (g_px_txt_input, g_stru_config.pstri_init_text);
        }
        else
        {
            lv_textarea_set_text (g_px_txt_input, "");
        }
        g_stru_screen.enm_result = GUI_SCREEN_RESULT_BACK;
    }

    /* If user clicked OK button on the keyboard */
    else if (enm_event == LV_EVENT_APPLY)
    {
        g_stru_screen.enm_result = GUI_SCREEN_RESULT_BACK;
    }

    /* Other events */
    else
    {
        /* Invoke default keyboard event handler */
        lv_keyboard_def_event_cb (px_obj, enm_event);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handles events of image showing/hiding password text
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Img_Visibility_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    /* If user clicked the image */
    if (enm_event == LV_EVENT_CLICKED)
    {
        bool b_password_mode = !lv_textarea_get_pwd_mode (g_px_txt_input);
        lv_textarea_set_pwd_mode (g_px_txt_input, b_password_mode);
        lv_img_set_src (g_px_img_visibility, b_password_mode ? &img_invisible : &img_visible);
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
