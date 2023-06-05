/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : progress_msgbox.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Sep 3
**  @brief      : Implementation of progress message box
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       Implements progress message box
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "progress_msgbox.h"        /* Internal public header of this control */

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

static int8_t s8_GUI_Create_Empty_Progress_Msgbox (void);
static int8_t s8_GUI_Run_Progress_Msgbox (void);

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
    .pfnc_run = s8_GUI_Run_Progress_Msgbox,
};

/** @brief  Info icon */
LV_IMG_DECLARE (img_info);

/** @brief  System icon */
LV_IMG_DECLARE (img_system);

/** @brief  Background of the message box */
static lv_obj_t * g_px_msgbox;

/** @brief  Window of the message box */
static lv_obj_t * g_px_window;

/** @brief  Icon of the message box */
static lv_obj_t * g_px_img_icon;

/** @brief  Detail description label */
static lv_obj_t * g_px_lbl_detail = NULL;

/** @brief  Detail description label */
static lv_obj_t * g_px_lbl_status = NULL;

/** @brief  Progress bar */
static lv_obj_t * g_px_bar_progress;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets structure wrapping information of progress message box control
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
int8_t s8_GUI_Get_Progress_Msgbox_Control (GUI_control_t ** ppstru_control)
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
**      Shows the progress message box with the given progress information
**
** @param [in]
**      pstru_progress: The progress information
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Show_Progress_Msgbox (const GUI_progress_t * pstru_progress)
{
    static GUI_job_t enm_prev_type = GUI_NUM_JOBS;

    ASSERT_PARAM (pstru_progress != NULL);

    /* Check if the progress is to be displayed or disposed */
    if ((pstru_progress->s32_progress < pstru_progress->s32_min) ||
        (pstru_progress->s32_progress > pstru_progress->s32_max) ||
        (pstru_progress->s32_min == pstru_progress->s32_max))
    {
        /* Delete the message box */
        if (g_px_msgbox != NULL)
        {
            lv_obj_del_async (g_px_msgbox);
            g_px_msgbox = NULL;
        }

        enm_prev_type = GUI_NUM_JOBS;
        return GUI_OK;
    }

    /* Create an empty progress message box */
    s8_GUI_Create_Empty_Progress_Msgbox ();

    /* Check if progress type changes */
    if (pstru_progress->enm_type != enm_prev_type)
    {
        enm_prev_type = pstru_progress->enm_type;

        /* Change color of the title bar correspondingly with progress type */
        static lv_style_t x_style_header;
        lv_style_init (&x_style_header);
        lv_style_set_bg_color (&x_style_header, LV_STATE_DEFAULT,
                               (pstru_progress->enm_type == GUI_JOB_SYSTEM) ? LV_COLOR_YELLOW : LV_COLOR_LIME);
        lv_obj_add_style (g_px_window, LV_WIN_PART_HEADER, &x_style_header);

        /* Change color of message box's border correspondingly with progress type */
        static lv_style_t x_style_border;
        lv_style_init (&x_style_border);
        lv_style_set_outline_width (&x_style_border, LV_STATE_DEFAULT, 1);
        lv_style_set_bg_color (&x_style_border, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        lv_style_set_outline_color (&x_style_border, LV_STATE_DEFAULT,
                                    (pstru_progress->enm_type == GUI_JOB_SYSTEM) ? LV_COLOR_YELLOW : LV_COLOR_LIME);
        lv_obj_add_style (g_px_window, LV_WIN_PART_BG, &x_style_border);

        /* Set icon of the message box correspondingly with progress type */
        lv_img_set_src (g_px_img_icon, (pstru_progress->enm_type == GUI_JOB_SYSTEM) ? &img_system : &img_info);
    }

    /* Brief description of the message */
    if (pstru_progress->pstri_brief != NULL)
    {
        lv_win_set_title (g_px_window, pstru_progress->pstri_brief);
    }
    else
    {
        lv_win_set_title (g_px_window, "");
    }

    /* Detail description of the message */
    if (pstru_progress->pstri_detail != NULL)
    {
        lv_label_set_text (g_px_lbl_detail, pstru_progress->pstri_detail);
    }
    else
    {
        lv_label_set_text (g_px_lbl_detail, "");
    }

    /* Job progress */
    uint8_t u8_percents = (pstru_progress->s32_progress - pstru_progress->s32_min) * 100 /
                          (pstru_progress->s32_max - pstru_progress->s32_min);
    lv_bar_set_value (g_px_bar_progress, u8_percents, LV_ANIM_OFF);

    /* Progress status */
    if (pstru_progress->pstri_status != NULL)
    {
        lv_label_set_text (g_px_lbl_status, pstru_progress->pstri_status);
        lv_obj_align (g_px_lbl_status, NULL, LV_ALIGN_CENTER, 0, 0);
    }
    else
    {
        lv_label_set_text (g_px_lbl_status, "");
    }

    /* Done */
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Creates an empty progress message box
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Create_Empty_Progress_Msgbox (void)
{
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

        /* Progress bar container */
        lv_obj_t * px_cont_progress = lv_cont_create (g_px_window, NULL);
        lv_cont_set_layout (px_cont_progress, LV_LAYOUT_PRETTY_TOP);
        lv_obj_set_size (px_cont_progress, 375, 45);
        lv_obj_add_style (px_cont_progress, LV_CONT_PART_MAIN, &x_style_cont_page);

        /* Job progress bar */
        g_px_bar_progress = lv_bar_create (px_cont_progress, NULL);
        lv_obj_set_size (g_px_bar_progress, 300, 20);

        /* Progress status description */
        static lv_style_t x_style_desc;
        lv_style_init (&x_style_desc);
        lv_style_set_text_font (&x_style_desc, LV_STATE_DEFAULT, &lv_font_montserrat_14);
        lv_style_set_text_color (&x_style_desc, LV_STATE_DEFAULT, LV_COLOR_BLUE);

        g_px_lbl_status = lv_label_create (g_px_bar_progress, NULL);
        lv_obj_add_style (g_px_lbl_status, LV_LABEL_PART_MAIN, &x_style_desc);
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs the progress message box
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Run_Progress_Msgbox (void)
{
    return GUI_OK;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
