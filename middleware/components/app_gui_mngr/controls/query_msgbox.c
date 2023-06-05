/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : query_msgbox.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 July 31
**  @brief      : Implementation of query message box
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       Implements query message box
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

static int8_t s8_GUI_Create_Empty_Query_Msgbox (void);
static int8_t s8_GUI_Run_Query_Msgbox (void);
static void v_GUI_Btn_Option_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);

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
    .pfnc_run = s8_GUI_Run_Query_Msgbox,
};

/** @brief  Info icon */
LV_IMG_DECLARE (img_info);

/** @brief  Warning icon */
LV_IMG_DECLARE (img_warning);

/** @brief  Error icon */
LV_IMG_DECLARE (img_error);

/** @brief  Wait time to automatically close the message box */
static uint32_t g_u32_wait_time = 0;

/** @brief  Index of option to be selected by default if g_u32_wait_time expires */
static uint8_t g_u8_default_option = 0;

/** @brief  Pointer to array of option strings */
static const char ** g_papstri_options;

/** @brief  Background of the message box */
static lv_obj_t * g_px_msgbox;

/** @brief  Window of the message box */
static lv_obj_t * g_px_window;

/** @brief  Icon of the message box */
static lv_obj_t * g_px_img_icon;

/** @brief  Detail description label */
static lv_obj_t * g_px_lbl_detail = NULL;

/** @brief  Array of selection buttons */
static lv_obj_t * g_apx_btn_options [GUI_MAX_QUERY_OPTIONS];

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets structure wrapping information of query message box control
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
int8_t s8_GUI_Get_Query_Msgbox_Control (GUI_control_t ** ppstru_control)
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
**      Shows the query message box with the given message
**
** @note
**      The structure pointed by pstru_query must be in static memory
**
** @param [in]
**      pstru_query: The query message
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Show_Query_Msgbox (const GUI_query_t * pstru_query)
{
    ASSERT_PARAM ((pstru_query != NULL) && (pstru_query->u8_num_options <= GUI_MAX_QUERY_OPTIONS) &&
                  (pstru_query->u8_default_option < GUI_MAX_QUERY_OPTIONS));

    /* Clear user selection */
    int8_t s8_option = -1;
    s8_GUI_Set_Data (GUI_DATA_USER_QUERY, &s8_option, sizeof (s8_option));

    /* Create an empty query message box */
    s8_GUI_Create_Empty_Query_Msgbox ();

    /* Store message parameters */
    g_u8_default_option = pstru_query->u8_default_option;
    g_papstri_options = (const char **)pstru_query->apstri_options;
    g_u32_wait_time = pstru_query->u32_wait_time;
    if (g_u32_wait_time != 0)
    {
        /* Round it to multiple of 1000 and offset it with 1000 ms */
        g_u32_wait_time = ((g_u32_wait_time + 1500) / 1000) * 1000;

        /* Change background color and text color of the default button */
        static lv_style_t x_style_btn_default;
        lv_style_init (&x_style_btn_default);
        lv_style_set_bg_color (&x_style_btn_default, LV_STATE_DEFAULT, LV_THEME_DEFAULT_COLOR_PRIMARY);
        lv_style_set_text_color (&x_style_btn_default, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_obj_add_style (g_apx_btn_options[g_u8_default_option], LV_LABEL_PART_MAIN, &x_style_btn_default);
    }

    /* Change color of the title bar correspondingly with query type */
    static lv_style_t x_style_header;
    lv_style_init (&x_style_header);
    lv_style_set_bg_color (&x_style_header, LV_STATE_DEFAULT,
                           (pstru_query->enm_type == GUI_MSG_INFO) ? LV_COLOR_LIME :
                           (pstru_query->enm_type == GUI_MSG_WARNING) ? LV_COLOR_YELLOW : LV_COLOR_RED);
    lv_obj_add_style (g_px_window, LV_WIN_PART_HEADER, &x_style_header);

    /* Change color of message box's border correspondingly with query type */
    static lv_style_t x_style_border;
    lv_style_init (&x_style_border);
    lv_style_set_outline_width (&x_style_border, LV_STATE_DEFAULT, 1);
    lv_style_set_bg_color (&x_style_border, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_outline_color (&x_style_border, LV_STATE_DEFAULT,
                                (pstru_query->enm_type == GUI_MSG_INFO) ? LV_COLOR_LIME :
                                (pstru_query->enm_type == GUI_MSG_WARNING) ? LV_COLOR_YELLOW : LV_COLOR_RED);
    lv_obj_add_style (g_px_window, LV_WIN_PART_BG, &x_style_border);

    /* Set icon of the message box correspondingly with query type */
    lv_img_set_src (g_px_img_icon, (pstru_query->enm_type == GUI_MSG_INFO) ? &img_info :
                                   (pstru_query->enm_type == GUI_MSG_WARNING) ? &img_warning : &img_error);

    /* Brief description of the message */
    if (pstru_query->pstri_brief != NULL)
    {
        lv_win_set_title (g_px_window, pstru_query->pstri_brief);
    }
    else
    {
        lv_win_set_title (g_px_window, "");
    }

    /* Detail description of the message */
    if (pstru_query->pstri_detail != NULL)
    {
        lv_label_set_text (g_px_lbl_detail, pstru_query->pstri_detail);
    }
    else
    {
        lv_label_set_text (g_px_lbl_detail, "");
    }

    /* Configure option buttons */
    for (uint8_t u8_idx = 0; u8_idx < pstru_query->u8_num_options; u8_idx++)
    {
        lv_label_set_text (lv_obj_get_child (g_apx_btn_options[u8_idx], NULL), g_papstri_options[u8_idx]);
        lv_obj_set_hidden (g_apx_btn_options[u8_idx], false);
    }

    /* Reduce width of the button container if there are only 2 buttons */
    if (pstru_query->u8_num_options == 2)
    {
        lv_obj_t * px_cont_btn = lv_obj_get_parent (g_apx_btn_options[0]);
        lv_obj_set_width (px_cont_btn, 300);
    }

    /* Done */
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Creates an empty query message box
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Create_Empty_Query_Msgbox (void)
{
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

        /* Common style for buttons */
        static lv_style_t x_style_btn;
        lv_style_init (&x_style_btn);
        lv_style_set_pad_left (&x_style_btn, LV_STATE_DEFAULT, 15);
        lv_style_set_pad_right (&x_style_btn, LV_STATE_DEFAULT, 15);
        lv_style_set_pad_top (&x_style_btn, LV_STATE_DEFAULT, 7);
        lv_style_set_pad_bottom (&x_style_btn, LV_STATE_DEFAULT, 7);

        /* Create all option buttons, make them hidden */
        for (uint8_t u8_idx = 0; u8_idx < GUI_MAX_QUERY_OPTIONS; u8_idx++)
        {
            g_apx_btn_options[u8_idx] = lv_btn_create (px_cont_btn, NULL);
            lv_btn_set_fit (g_apx_btn_options[u8_idx], LV_FIT_TIGHT);
            lv_obj_set_event_cb (g_apx_btn_options[u8_idx], v_GUI_Btn_Option_Event_Cb);
            lv_obj_add_style (g_apx_btn_options[u8_idx], LV_BTN_PART_MAIN, &x_style_btn);
            lv_label_set_text (lv_label_create (g_apx_btn_options[u8_idx], NULL), "");
            lv_obj_set_hidden (g_apx_btn_options[u8_idx], true);
        }
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs the query message box
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Run_Query_Msgbox (void)
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
            lv_event_send (g_apx_btn_options[g_u8_default_option], LV_EVENT_CLICKED, NULL);
        }
        else
        {
            /* Update the remaining wait time on the default button */
            lv_label_set_text_fmt (lv_obj_get_child (g_apx_btn_options[g_u8_default_option], NULL), "%s (%d)",
                                   g_papstri_options[g_u8_default_option], g_u32_wait_time / 1000);
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
static void v_GUI_Btn_Option_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    if (enm_event == LV_EVENT_CLICKED)
    {
        /* Determine user selection */
        for (uint8_t u8_idx = 0; u8_idx < GUI_MAX_QUERY_OPTIONS; u8_idx++)
        {
            if (px_obj == g_apx_btn_options[u8_idx])
            {
                int8_t s8_option = u8_idx;
                s8_GUI_Set_Data (GUI_DATA_USER_QUERY, &s8_option, sizeof(s8_option));
                break;
            }
        }

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
