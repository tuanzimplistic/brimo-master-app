/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : notify_msgbox.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 July 29
**  @brief      : Implementation of notify message box
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       Implements notify message box
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "notify_msgbox.h"              /* Internal public header of this control */

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

static int8_t s8_GUI_Create_Empty_Notify_Msgbox (void);
static int8_t s8_GUI_Run_Notify_Msgbox (void);
static void v_GUI_Btn_Gotit_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "App_Gui_Mngr";

/** @brief  Information of the user control */
static GUI_control_t g_stru_control =
{
    .pfnc_run = s8_GUI_Run_Notify_Msgbox,
};

/** @brief  Info icon */
LV_IMG_DECLARE (img_info);

/** @brief  Warning icon */
LV_IMG_DECLARE (img_warning);

/** @brief  Error icon */
LV_IMG_DECLARE (img_error);

/** @brief  Wait time to automatically close the message box */
static uint32_t g_u32_wait_time = 0;

/** @brief  Background of the message box */
static lv_obj_t * g_px_msgbox;

/** @brief  Window of the message box */
static lv_obj_t * g_px_window;

/** @brief  Icon of the message box */
static lv_obj_t * g_px_img_icon;

/** @brief  Detail description label */
static lv_obj_t * g_px_lbl_detail = NULL;

/** @brief  "Got it" button */
static lv_obj_t * g_px_btn_gotit;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets structure wrapping information of notify message box control
**
** @param [out]
**      ppstru_control: Pointer to the structure wrapping the user control
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Get_Notify_Msgbox_Control (GUI_control_t ** ppstru_control)
{
    ASSERT_PARAM (ppstru_control != NULL);

    /* Return the control if a message box is being displayed */
    if (g_px_msgbox != NULL)
    {
        *ppstru_control = &g_stru_control;
    }
    else
    {
        *ppstru_control = NULL;
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Shows the notify message box with the given message
**
** @param [in]
**      pstru_notify: The notify message
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Show_Notify_Msgbox (const GUI_notify_t * pstru_notify)
{
    ASSERT_PARAM (pstru_notify != NULL);

    /* Create an empty notify message box */
    s8_GUI_Create_Empty_Notify_Msgbox ();

    /* Store the wait time, 0 means wait forever */
    g_u32_wait_time = pstru_notify->u32_wait_time;
    if (g_u32_wait_time != 0)
    {
        /* Round it to multiple of 1000 and offset it with 1000 ms */
        g_u32_wait_time = ((g_u32_wait_time + 1500) / 1000) * 1000;

        /* Change background color and text color of the default button */
        static lv_style_t x_style_btn_default;
        lv_style_init (&x_style_btn_default);
        lv_style_set_bg_color (&x_style_btn_default, LV_STATE_DEFAULT, LV_THEME_DEFAULT_COLOR_PRIMARY);
        lv_style_set_text_color (&x_style_btn_default, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_obj_add_style (g_px_btn_gotit, LV_LABEL_PART_MAIN, &x_style_btn_default);
    }

    /* Change color of the title bar correspondingly with notify type */
    static lv_style_t x_style_header;
    lv_style_init (&x_style_header);
    lv_style_set_bg_color (&x_style_header, LV_STATE_DEFAULT,
                           (pstru_notify->enm_type == GUI_MSG_INFO) ? LV_COLOR_LIME :
                           (pstru_notify->enm_type == GUI_MSG_WARNING) ? LV_COLOR_YELLOW : LV_COLOR_RED);
    lv_obj_add_style (g_px_window, LV_WIN_PART_HEADER, &x_style_header);

    /* Change color of message box's border correspondingly with notify type */
    static lv_style_t x_style_border;
    lv_style_init (&x_style_border);
    lv_style_set_outline_width (&x_style_border, LV_STATE_DEFAULT, 1);
    lv_style_set_bg_color (&x_style_border, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_outline_color (&x_style_border, LV_STATE_DEFAULT,
                                (pstru_notify->enm_type == GUI_MSG_INFO) ? LV_COLOR_LIME :
                                (pstru_notify->enm_type == GUI_MSG_WARNING) ? LV_COLOR_YELLOW : LV_COLOR_RED);
    lv_obj_add_style (g_px_window, LV_WIN_PART_BG, &x_style_border);

    /* Set icon of the message box correspondingly with notify type */
    lv_img_set_src (g_px_img_icon, (pstru_notify->enm_type == GUI_MSG_INFO) ? &img_info :
                                   (pstru_notify->enm_type == GUI_MSG_WARNING) ? &img_warning : &img_error);

    /* Brief description of the message */
    if (pstru_notify->pstri_brief != NULL)
    {
        lv_win_set_title (g_px_window, pstru_notify->pstri_brief);
    }
    else
    {
        lv_win_set_title (g_px_window, "");
    }

    /* Detail description of the message */
    if (pstru_notify->pstri_detail != NULL)
    {
        lv_label_set_text (g_px_lbl_detail, pstru_notify->pstri_detail);
    }
    else
    {
        lv_label_set_text (g_px_lbl_detail, "");
    }

    /* Done */
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Creates an empty notify message box
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Create_Empty_Notify_Msgbox (void)
{
    static lv_style_t x_style_btn_gotit;

    /* Only create new message box if one is not existing */
    if (g_px_msgbox == NULL)
    {
        /* Create a transparent full-screen object to absorb all click events */
        g_px_msgbox = lv_obj_create (lv_layer_top (), NULL);
        lv_obj_reset_style_list (g_px_msgbox, LV_OBJ_PART_MAIN);
        lv_obj_set_size (g_px_msgbox, LV_HOR_RES, LV_VER_RES);

        /* Window of the message box */
        g_px_window = lv_win_create (g_px_msgbox, NULL);
        lv_obj_set_size (g_px_window, 400, 250);
        lv_obj_align (g_px_window, NULL, LV_ALIGN_CENTER, 0, 0);
        lv_win_title_set_alignment (g_px_window, 4);
        lv_win_set_header_height (g_px_window, 30);
        lv_win_set_layout (g_px_window, LV_LAYOUT_COLUMN_MID);
        //lv_win_set_drag (g_px_window, true);

        static lv_style_t x_style_win;
        lv_style_init (&x_style_win);
        lv_style_set_pad_inner (&x_style_win, LV_STATE_DEFAULT, 10);
        lv_obj_add_style (g_px_window, LV_WIN_PART_CONTENT_SCROLLABLE, &x_style_win);

        /* Common style for containers and pages */
        static lv_style_t x_style_cont_page;
        lv_style_init (&x_style_cont_page);
        lv_style_set_border_width (&x_style_cont_page, LV_STATE_DEFAULT, 0);
        lv_style_set_pad_left (&x_style_cont_page, LV_STATE_DEFAULT, 5);
        lv_style_set_pad_right (&x_style_cont_page, LV_STATE_DEFAULT, 5);
        lv_style_set_pad_top (&x_style_cont_page, LV_STATE_DEFAULT, 10);
        lv_style_set_pad_bottom (&x_style_cont_page, LV_STATE_DEFAULT, 10);

        /* Container of message box icon and detail description */
        lv_obj_t * px_cont_msg = lv_cont_create (g_px_window, NULL);
        lv_cont_set_layout (px_cont_msg, LV_LAYOUT_ROW_MID);
        lv_obj_set_size (px_cont_msg, 375, 140);
        lv_obj_add_style (px_cont_msg, LV_CONT_PART_MAIN, &x_style_cont_page);

        /* Message box icon */
        lv_obj_t * px_page_icon = lv_page_create (px_cont_msg, NULL);
        lv_obj_set_size (px_page_icon, 75, 140);
        lv_obj_add_style (px_page_icon, LV_PAGE_PART_BG, &x_style_cont_page);
        g_px_img_icon =  lv_img_create (px_page_icon, NULL);

        /* Detail description of the message */
        lv_obj_t * px_page_detail = lv_page_create (px_cont_msg, NULL);
        lv_obj_set_size (px_page_detail, 280, 140);
        lv_obj_add_style (px_page_detail, LV_PAGE_PART_BG, &x_style_cont_page);

        g_px_lbl_detail = lv_label_create (px_page_detail, NULL);
        lv_label_set_long_mode (g_px_lbl_detail, LV_LABEL_LONG_BREAK);
        lv_obj_set_size (g_px_lbl_detail, 240, 140);

        /* Button container */
        lv_obj_t * px_cont_btn = lv_cont_create (g_px_window, NULL);
        lv_cont_set_layout (px_cont_btn, LV_LAYOUT_PRETTY_TOP);
        lv_obj_set_size (px_cont_btn, 375, 45);
        lv_obj_add_style (px_cont_btn, LV_CONT_PART_MAIN, &x_style_cont_page);

        /* Style for "Got it" button */
        lv_style_init (&x_style_btn_gotit);
        lv_style_set_bg_color (&x_style_btn_gotit, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_style_set_text_color (&x_style_btn_gotit, LV_STATE_DEFAULT, LV_COLOR_BLACK);
        lv_style_set_pad_left (&x_style_btn_gotit, LV_STATE_DEFAULT, 15);
        lv_style_set_pad_right (&x_style_btn_gotit, LV_STATE_DEFAULT, 15);
        lv_style_set_pad_top (&x_style_btn_gotit, LV_STATE_DEFAULT, 7);
        lv_style_set_pad_bottom (&x_style_btn_gotit, LV_STATE_DEFAULT, 7);

        /* "Got it" button */
        g_px_btn_gotit = lv_btn_create (px_cont_btn, NULL);
        lv_btn_set_fit (g_px_btn_gotit, LV_FIT_TIGHT);
        lv_obj_set_event_cb (g_px_btn_gotit, v_GUI_Btn_Gotit_Event_Cb);
        lv_label_create (g_px_btn_gotit, NULL);
    }

    /* Default "Got it" button */
    lv_obj_add_style (g_px_btn_gotit, LV_BTN_PART_MAIN, &x_style_btn_gotit);
    lv_label_set_text (lv_obj_get_child (g_px_btn_gotit, NULL), "Got it");

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs the notify message box
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Run_Notify_Msgbox (void)
{
    static uint32_t u32_run_timer = 0;

    /* Check if the message box is to be auto-closed */
    if ((g_u32_wait_time > 0) && (GUI_TIMER_ELAPSED (u32_run_timer) >= 1000))
    {
        GUI_TIMER_RESET (u32_run_timer);
        g_u32_wait_time -= 1000;
        if (g_u32_wait_time < 1000)
        {
            /* Time's up. Automatically click the default button */
            lv_event_send (g_px_btn_gotit, LV_EVENT_CLICKED, NULL);
        }
        else
        {
            /* Update the remaining wait time on "Got it" button */
            lv_label_set_text_fmt (lv_obj_get_child (g_px_btn_gotit, NULL), "Got it (%d)", g_u32_wait_time / 1000);
        }
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of button "Got it" events
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Btn_Gotit_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Delete the message box */
        lv_obj_del_async (g_px_msgbox);
        g_px_msgbox = NULL;
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
