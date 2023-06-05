/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : cam_screen.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Jan 12
**  @brief      : Implementation of camera screen
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       This screen shows video captured by the camera module
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "screen_common.h"              /* Common header of all screens */
#include "srvc_cam.h"                   /* Use camera module */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Refesh cycle (in milliseconds) of the video playback */
#define GUI_CAMERA_REFRESH_CYCLE        50

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static void v_GUI_Cam_Img_Update_Task (lv_task_t * px_task);
static void v_GUI_Cam_Image_Event_Cb (lv_obj_t * obj, lv_event_t evt);

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
    .pstri_name         = "Camera",
    .px_icon            = NULL,
    .pfnc_start         = NULL,
    .pfnc_stop          = NULL,
    .pfnc_run           = NULL,
    .enm_result         = GUI_SCREEN_RESULT_NONE,
};

/** @brief  Image object to display images captured from the camera */
static lv_obj_t * g_px_img_obj;

/** @brief  Descriptor of the image captured from the camera */
static lv_img_dsc_t g_stru_img_dsc =
{
    .header =
    {
        .reserved = 0,
        .always_zero = 0,
        .cf = LV_IMG_CF_TRUE_COLOR,
    },
};

/** @brief  Instance of the camera module */
static CAM_inst_t g_x_cam_inst = NULL;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets structure wrapping camera screen
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
int8_t s8_GUI_Get_Cam_Screen (GUI_screen_t ** ppstru_screen)
{
    ASSERT_PARAM (ppstru_screen != NULL);

    /* If this screen has not been initialized, initialize it */
    if (!g_b_initialized)
    {
        /* Initialize and get instance of the camera module */
        if (s8_CAM_Get_Inst (&g_x_cam_inst) != CAM_OK)
        {
            LOGE ("Failed to initialize camera module");
        }

        /* Create the screen*/
        lv_obj_t * px_screen = lv_obj_create (NULL, NULL);

        /* Title */
        lv_obj_t * px_lbl_title =  lv_label_create (px_screen, NULL);
        lv_label_set_text (px_lbl_title, "Camera test");
        lv_obj_align (px_lbl_title, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);

        /* Image object for video playback */
        g_px_img_obj = lv_img_create (px_screen, NULL);
        lv_obj_set_auto_realign (g_px_img_obj, true);
        lv_obj_align (g_px_img_obj, NULL, LV_ALIGN_CENTER, 0, 30);
        lv_obj_set_event_cb (g_px_img_obj, v_GUI_Cam_Image_Event_Cb);

        /* Create LVGL task updating image from the camera */
        lv_task_t * px_task = lv_task_create (v_GUI_Cam_Img_Update_Task, GUI_CAMERA_REFRESH_CYCLE,
                                              LV_TASK_PRIO_LOWEST, NULL);
        lv_task_ready (px_task);

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
**      LVGL task updating images from the camera module for video playback
**
** @param [in]
**      px_task: The LVGL task
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Cam_Img_Update_Task (lv_task_t * px_task)
{
    if (g_x_cam_inst != NULL)
    {
        CAM_shot_t stru_shot;
        if (s8_CAM_Take_Shot (g_x_cam_inst, &stru_shot) == CAM_OK)
        {
            g_stru_img_dsc.header.w = stru_shot.u16_width;
            g_stru_img_dsc.header.h = stru_shot.u16_height;
            g_stru_img_dsc.data = stru_shot.pu8_data;
            g_stru_img_dsc.data_size = stru_shot.u32_len;
            lv_event_send (g_px_img_obj, LV_EVENT_VALUE_CHANGED, NULL);
            s8_CAM_Release_Shot (g_x_cam_inst, &stru_shot);
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handles events of the image object for video playback
**
** @param [in]
**      px_obj: The object on which the event occurred
**
** @param [in]
**      enm_event: The occurred event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Cam_Image_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event)
{
    if (enm_event == LV_EVENT_VALUE_CHANGED)
    {
        /* Update the image captured from the camera */
        lv_img_set_src (g_px_img_obj, &g_stru_img_dsc);
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
