/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : developer.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Jun 17
**  @brief      : Implementation of Developer screen
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       This screen provides places for photo captured from the camera to be shown. Debug information can
**              be shown in this screen as well.
** @details
**
** Developer screen is bound with the following data:
**
** @verbatim
** +----------------------------+------------+------------------------------------------------------------------+
** | GUI data                   | GUI access | Description                                                      |
** +----------------------------+------------+------------------------------------------------------------------+
** | GUI_DATA_DEBUG_INFO        | Write      | General purpose information for debugging                        |
** | GUI_DATA_DEBUG_PICTURE     | Write      | Trigger the camera to take a picture and display on the screen   |
** +----------------------------+------------+------------------------------------------------------------------+
** @endverbatim
**
** Example:
**
** @code{.py}
**
**  import gui
**
**  # Show some debug information on the screen
**  gui.set_data (gui.GUI_DATA_DEBUG_INFO, 'Roti making was done successfully')
**
**  # Take a picture from the camera and display it onto the LCD
**  gui.set_data (gui.GUI_DATA_DEBUG_PICTURE, '/dev/cam')
**
**  # Display a JPG picture in the filesystem onto the LCD
**  # Note that the JPG decoder doesn't support picture in GRAYSCALE format
**  gui.set_data (gui.GUI_DATA_DEBUG_PICTURE, '/picture.jpg')
**
**  # Take a picture from a buffer in RAM at address 0x12345678 and display it onto the LCD
**  # Structure of the picture is camera_fb_t (srvc_cam\driver\include\esp_camera.h)
**  gui.set_data (gui.GUI_DATA_DEBUG_PICTURE, '/dev/framebuf/0x12345678')
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
#include "srvc_cam.h"                   /* Use camera module */

#include "esp_heap_caps.h"              /* Use heap_caps_malloc() */
#include <stdio.h>                      /* Use sscanf() */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Cycle (in milliseconds) polling GUI data */
#define GUI_REFRESH_DATA_CYCLE          100

/** @brief  Maximum length in bytes of developer buffer */
#define GUI_DEV_BUF_LEN                 96

/** @brief  Path indicating that the picture to be displayed is taken from the camera */
#define GUI_CAMERA_PATH                 "/dev/cam"

/** @brief  Path indicating that the picture to be displayed is taken from a buffer in RAM  */
#define GUI_RAM_BUFFER_PATH             "/dev/framebuf"

/**
** @brief
**      Macro converting RGB888 to swapped RGB565
**
** @details
**      Structure of a swapped RGB565
**           15       13 12         8 7         3 2          0
**          +-----------+------------+-----------+------------+
**          | Green_low |    Blue    |    Red    | Green_high |
**          +-----------+------------+-----------+------------+
**            (3 bits)     (5 bits)    (5 bits)     (3 bits)
**
** @param [in]
**      u8_red: red in RGB888
**
** @param [in]
**      u8_green: green in RGB888
**
** @param [in]
**      u8_blue: blue in RGB888
**
** @return
**      Swapped RGB565 in uint16_t type
*/
#define GUI_rgb888to565(u8_red, u8_green, u8_blue)      \
    (uint16_t)(((((u8_green) >> 2) >> 3) & 0x07) |      \
               ((((u8_red) >> 3) & 0x1F) << 3)   |      \
               ((((u8_blue) >> 3) & 0x1F) << 8)  |      \
               ((((u8_green) >> 2) & 0x07) << 13))

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static int8_t s8_GUI_Start_Developer_Screen (void);
static int8_t s8_GUI_Stop_Developer_Screen (void);
static int8_t s8_GUI_Run_Developer_Screen (void);
static void v_GUI_Btn_Back_Event_Cb (lv_obj_t * px_obj, lv_event_t enm_event);
static int8_t s8_GUI_Init_Cam_Buffer (void);
static void v_GUI_Show_Pic_From_Cam (void);
static void v_GUI_Show_Pic_From_File (const char * pstri_path);
static void v_GUI_Show_Pic_From_Buffer (const camera_fb_t * pstru_buf);
static void v_GUI_Get_Pic_From_Shot (CAM_shot_t * pstru_shot, uint8_t * pu8_buf);

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
    .pstri_name         = "Developer Tools",
    .px_icon            = NULL,
    .pfnc_start         = s8_GUI_Start_Developer_Screen,
    .pfnc_stop          = s8_GUI_Stop_Developer_Screen,
    .pfnc_run           = s8_GUI_Run_Developer_Screen,
    .enm_result         = GUI_SCREEN_RESULT_NONE,
};

/** @brief  Image for back button */
LV_IMG_DECLARE (img_back);

/** @brief  Image object to display images captured from the camera */
static lv_obj_t * g_px_img_debug_pic;

/** @brief  Label of debug information */
static lv_obj_t * g_px_lbl_debug_info;

/** @brief  Image description for debug picture */
static lv_img_dsc_t g_stru_img_dsc =
{
    .header =
    {
        .reserved = 0,
        .always_zero = 0,
        .cf = LV_IMG_CF_TRUE_COLOR,     /* Current config of LVGL color depth is RGB565 */
    },
};

/** @brief  Timer refreshing the picture taken from the camera */
static uint32_t g_u32_cam_shot_timer = 0;

/** @brief  Handle of camera instance */
static CAM_inst_t g_x_cam_inst = NULL;

/** @brief  Shot taken from the camera */
static CAM_shot_t g_stru_shot;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets structure wrapping developer screen
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
int8_t s8_GUI_Get_Developer_Screen (GUI_screen_t ** ppstru_screen)
{
    ASSERT_PARAM (ppstru_screen != NULL);

    /* If this screen has not been initialized, initialize it */
    if (!g_b_initialized)
    {
        /* Create the screen*/
        lv_obj_t * px_screen = lv_obj_create (NULL, NULL);

        /* Style for image buttons, darken the buttons when pressed */
        static lv_style_t x_style_imgbtn;
        lv_style_init (&x_style_imgbtn);
        lv_style_set_image_recolor_opa (&x_style_imgbtn, LV_STATE_PRESSED, LV_OPA_30);
        lv_style_set_image_recolor (&x_style_imgbtn, LV_STATE_PRESSED, LV_COLOR_BLACK);

        /* Image object for debug picture */
        g_px_img_debug_pic = lv_img_create (px_screen, NULL);
        lv_obj_set_auto_realign (g_px_img_debug_pic, true);
        lv_obj_align (g_px_img_debug_pic, NULL, LV_ALIGN_CENTER, 0, 0);

        /* Back button */
        lv_obj_t * px_imgbtn_back = lv_imgbtn_create (px_screen, NULL);
        lv_obj_add_style (px_imgbtn_back, LV_IMGBTN_PART_MAIN, &x_style_imgbtn);
        lv_imgbtn_set_src (px_imgbtn_back, LV_BTN_STATE_RELEASED, &img_back);
        lv_obj_align (px_imgbtn_back, NULL, LV_ALIGN_IN_TOP_LEFT, 15, 15);
        lv_obj_set_event_cb (px_imgbtn_back, v_GUI_Btn_Back_Event_Cb);

        /* Style of debug information text */
        static lv_style_t x_style_debug_text;
        lv_style_init (&x_style_debug_text);
        lv_style_set_text_font (&x_style_debug_text, LV_STATE_DEFAULT, &lv_font_montserrat_18);
        lv_style_set_text_color (&x_style_debug_text, LV_STATE_DEFAULT, LV_COLOR_BLUE);

        /* Label for debug information */
        g_px_lbl_debug_info = lv_label_create (px_screen, NULL);
        lv_obj_set_auto_realign (g_px_lbl_debug_info, true);
        lv_obj_align (g_px_lbl_debug_info, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -15);
        lv_obj_add_style (g_px_lbl_debug_info, LV_LABEL_PART_MAIN, &x_style_debug_text);

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
**      Starts Developer screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Start_Developer_Screen (void)
{
    LOGD ("Developer screen started");
    g_stru_screen.enm_result = GUI_SCREEN_RESULT_NONE;

    /* Take a shot from the camera and show it on the LCD */
    s8_GUI_Set_Data (GUI_DATA_DEBUG_PICTURE, GUI_CAMERA_PATH, 0);

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Stops Developer screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Stop_Developer_Screen (void)
{
    LOGD ("Developer screen stopped");
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs Developer screen
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Run_Developer_Screen (void)
{
    static bool b_buf_init = false;
    char stri_buffer[GUI_DEV_BUF_LEN] = "";

    /* Initialize buffer for debug picture if not done that yet */
    if (!b_buf_init)
    {
        if (s8_GUI_Init_Cam_Buffer () == GUI_OK)
        {
            b_buf_init = true;
        }
    }

    /* Do nothing if it's not time to refresh the screen */
    static uint32_t u32_data_timer = 0;
    if (GUI_TIMER_ELAPSED (u32_data_timer) < GUI_REFRESH_DATA_CYCLE)
    {
        return GUI_OK;
    }
    else
    {
        GUI_TIMER_RESET (u32_data_timer);
    }

    /* Refresh debug information */
    uint16_t u16_buffer_len = sizeof (stri_buffer);
    if (s8_GUI_Get_Data_If_Changed (GUI_DATA_DEBUG_INFO, stri_buffer, &u16_buffer_len) == GUI_OK)
    {
        lv_label_set_text (g_px_lbl_debug_info, stri_buffer);
        lv_obj_align (g_px_lbl_debug_info, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -15);
    }

    /* Refresh camera image if requested */
    u16_buffer_len = sizeof (stri_buffer);
    if (s8_GUI_Get_Data_If_Changed (GUI_DATA_DEBUG_PICTURE, stri_buffer, &u16_buffer_len) == GUI_OK)
    {
        /* Check source of the picture to display onto the LCD */
        if (strcmp (GUI_CAMERA_PATH, stri_buffer) == 0)
        {
            /* The picture is taken from the camera */
            LOGI ("Taking picture from camera and show on screen");
            v_GUI_Show_Pic_From_Cam ();
        }
        else if (memcmp (GUI_RAM_BUFFER_PATH, stri_buffer, sizeof (GUI_RAM_BUFFER_PATH) - 1) == 0)
        {
            /* The picture is taken from a buffer on RAM */
            /* Parse address of the buffer */
            uint32_t u32_buf_addr;
            if (sscanf (&stri_buffer[sizeof (GUI_RAM_BUFFER_PATH)], "0x%08X", &u32_buf_addr) != 1)
            {
                LOGE ("Invalid frame buffer address %s", stri_buffer);
            }
            else
            {
                LOGI ("Taking picture from RAM buffer at address 0x%08X and show on screen", u32_buf_addr);
                v_GUI_Show_Pic_From_Buffer ((camera_fb_t *)u32_buf_addr);
            }
        }
        else
        {
            /* If the picture is taken from a file in the filesystem */
            LOGI ("Taking picture from file %s and show on screen", stri_buffer);
            v_GUI_Show_Pic_From_File (stri_buffer);
        }
    }

    /* If a picture has just been taken from the camera, continuously refresh it for a short time */
    if (GUI_TIMER_ELAPSED (g_u32_cam_shot_timer) < 1000)
    {
        /* Format conversion */
        v_GUI_Get_Pic_From_Shot (&g_stru_shot, (uint8_t *)g_stru_img_dsc.data);

        /* Refresh debug picture */
        lv_img_set_src (g_px_img_debug_pic, &g_stru_img_dsc);

        /* Ivalidate the image source in the LVGL cache to redraw it */
        lv_img_cache_invalidate_src (&g_stru_img_dsc);
    }

    return GUI_OK;
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
**      Allocates buffer storing picture data taken from camera
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GUI_Init_Cam_Buffer (void)
{
    /* Get camera instance */
    if (s8_CAM_Get_Inst (&g_x_cam_inst) != CAM_OK)
    {
        LOGE ("Failed to initialize camera module");
        return GUI_ERR;
    }

    /*
    ** Allocate a buffer in external memory to store data of the picture displayed in this screen. Each pixel of
    ** the picture will need 2 bytes to be displayed on the LCD (RGB565).
    ** This buffer is meant for displaying the picture captured from the camera, therefore the size of
    ** the buffer is: camera_frame_buffer_width * camera_frame_buffer_height * 2
    */
    uint16_t u16_frame_width;
    uint16_t u16_frame_height;
    v_CAM_Get_Frame_Size (g_x_cam_inst, &u16_frame_width, &u16_frame_height);
    g_stru_img_dsc.header.w = u16_frame_width;
    g_stru_img_dsc.header.h = u16_frame_height;
    g_stru_img_dsc.data_size = u16_frame_width * u16_frame_height * sizeof (uint16_t);
    g_stru_img_dsc.data = heap_caps_malloc (u16_frame_width * u16_frame_height * 2, MALLOC_CAP_SPIRAM);
    if (g_stru_img_dsc.data == NULL)
    {
        LOGE ("Failed to allocate buffer for debug picture");
        return GUI_ERR;
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Takes a picture from the camera and shows it onto the LCD
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Show_Pic_From_Cam (void)
{
    /* Take a picture from the camera */
    if (s8_CAM_Take_Shot (g_x_cam_inst, &g_stru_shot) == CAM_OK)
    {
        /* Ensure that the taken picture is aligned with the provided buffer */
        uint16_t u16_frame_width;
        uint16_t u16_frame_height;
        v_CAM_Get_Frame_Size (g_x_cam_inst, &u16_frame_width, &u16_frame_height);
        ASSERT_PARAM ((g_stru_shot.u16_width == u16_frame_width) &&
                      (g_stru_shot.u16_height == u16_frame_height));

        /* Because it takes time for the picture to be taken completely, start the timer refreshing the picture */
        GUI_TIMER_RESET (g_u32_cam_shot_timer);

        /* Release the camera shot */
        s8_CAM_Release_Shot (g_x_cam_inst, &g_stru_shot);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Shows a JPG image file onto the LCD
**
** @note
**      The JPG decoder doesn't support picture in GRAYSCALE format
**
** @param [in]
**      pstri_path: Path of the JPG image to display
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Show_Pic_From_File (const char * pstri_path)
{
    uint8_t *   pu8_jpg_buf = NULL;
    bool        b_success = true;

    /* Ensure that the given file exists */
    struct lfs2_info stru_file_info;
    if (lfs2_stat (g_px_lfs2, pstri_path, &stru_file_info) < 0)
    {
        LOGE ("File %s doesn't exist", pstri_path);
        return;
    }

    /* Allocate a buffer in external memory for the JPG picture read from file */
    if (b_success)
    {
        pu8_jpg_buf = heap_caps_malloc (stru_file_info.size, MALLOC_CAP_SPIRAM);
        if (pu8_jpg_buf == NULL)
        {
            LOGE ("Failed to allocate buffer for JPG picture read from file");
            b_success = false;
        }
    }

    /* Read data from JPG file into buffer */
    if (b_success)
    {
        /* Open the file for reading */
        lfs2_file_t x_file;
        if (lfs2_file_open (g_px_lfs2, &x_file, pstri_path, LFS2_O_RDONLY) < 0)
        {
            LOGE ("Failed to open file %s for reading", pstri_path);
            b_success = false;
        }
        else
        {
            /* Read data from the JPG file */
            size_t x_num_read = lfs2_file_read (g_px_lfs2, &x_file, pu8_jpg_buf, stru_file_info.size);
            if (x_num_read != stru_file_info.size)
            {
                LOGE ("Failed to picture data from file %s", pstri_path);
                b_success = false;
            }

            /* Close the JPG file */
            lfs2_file_close (g_px_lfs2, &x_file);
        }
    }

    /* Decode the given image from JPG format to RGB565 format to display on the LCD */
    uint16_t u16_width = 0;
    uint16_t u16_height = 0;
    if (b_success)
    {
        if (jpg2rgb565 (pu8_jpg_buf, stru_file_info.size,
                        (uint8_t *)g_stru_img_dsc.data, &u16_width, &u16_height, JPG_SCALE_NONE) == false)
        {
            LOGE ("Failed to decode the given JPG picture to RGB565 format");
            b_success = false;
        }
    }

    /* Display the image on the screen */
    if (b_success)
    {
        /* Each pixel in RGB565 format consumes 2 bytes, ensure that the decoded image is fit in the provided buffer */
        uint16_t u16_frame_width;
        uint16_t u16_frame_height;
        v_CAM_Get_Frame_Size (g_x_cam_inst, &u16_frame_width, &u16_frame_height);
        ASSERT_PARAM ((u16_width == u16_frame_width) && (u16_height == u16_frame_height));

        /* Refresh debug picture */
        lv_img_set_src (g_px_img_debug_pic, &g_stru_img_dsc);

        /* Ivalidate the image source in the LVGL cache to redraw it */
        lv_img_cache_invalidate_src (&g_stru_img_dsc);
    }

    /* Cleanup */
    if (pu8_jpg_buf != NULL)
    {
        free (pu8_jpg_buf);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Shows a picture stored in a buffer at a given address onto the LCD
**
** @param [in]
**      pstru_buf: Pointer to the frame buffer of the picture in RAM
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Show_Pic_From_Buffer (const camera_fb_t * pstru_buf)
{
    /* Validate size of the input buffer */
    uint16_t u16_frame_width;
    uint16_t u16_frame_height;
    v_CAM_Get_Frame_Size (g_x_cam_inst, &u16_frame_width, &u16_frame_height);
    if ((pstru_buf->width != u16_frame_width) || (pstru_buf->height != u16_frame_height))
    {
        LOGE ("Invalid frame width (%d pixels) or frame height (%d pixels)", pstru_buf->width, pstru_buf->height);
        return;
    }

    /* Convert the input buffer so that it can be display on the LCD */
    uint8_t * pu8_in_buf = pstru_buf->buf;
    uint8_t * pu8_out_buf = (uint8_t *)g_stru_img_dsc.data;
    if (pstru_buf->format == PIXFORMAT_GRAYSCALE)
    {
        /* Convert grayscale (1 byte/pixel) to RGB565 */
        uint32_t u32_buf_len = u16_frame_width * u16_frame_height;
        for (uint32_t u32_idx = 0; u32_idx < u32_buf_len; u32_idx++)
        {
            uint8_t u8_red = pu8_in_buf[u32_idx];
            uint8_t u8_green = pu8_in_buf[u32_idx];
            uint8_t u8_blue = pu8_in_buf[u32_idx];
            uint16_t u16_pixel = GUI_rgb888to565 (u8_red, u8_green, u8_blue);
            ENDIAN_PUT16 (pu8_out_buf, u16_pixel);
            pu8_out_buf += 2;
        }
    }
    else if (pstru_buf->format == PIXFORMAT_RGB565)
    {
        /* The format matches, just copy the frame buffer data */
        uint32_t u32_buf_len = u16_frame_width * u16_frame_height * 2;
        memcpy (pu8_out_buf, pu8_in_buf, u32_buf_len);
    }
    else if (pstru_buf->format == PIXFORMAT_RGB888)
    {
        /* Convert RGB888 (3 byte/pixel) to RGB565 */
        uint32_t u32_buf_len = u16_frame_width * u16_frame_height * 3;
        for (uint32_t u32_idx = 0; u32_idx < u32_buf_len; u32_idx += 3)
        {
            uint8_t u8_blue = pu8_in_buf[u32_idx + 0];
            uint8_t u8_green = pu8_in_buf[u32_idx + 1];
            uint8_t u8_red = pu8_in_buf[u32_idx + 2];
            uint16_t u16_pixel = GUI_rgb888to565 (u8_red, u8_green, u8_blue);
            ENDIAN_PUT16 (pu8_out_buf, u16_pixel);
            pu8_out_buf += 2;
        }
    }
    else
    {
        LOGE ("Frame format %d is not supported", pstru_buf->format);
        return;
    }

    /* Refresh debug picture */
    lv_img_set_src (g_px_img_debug_pic, &g_stru_img_dsc);

    /* Ivalidate the image source in the LVGL cache to redraw it */
    lv_img_cache_invalidate_src (&g_stru_img_dsc);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Converts picture data of a shot taken from the camera to what can be displayed on the LCD
**
** @details
**      Because LVGL is configured with color depth of RGB565, for a shot taken from the camera to be displayed on the
**      LCD, the taken picture must be converted to RGB565 format.
**
** @param [in]
**      pstru_shot: Shot taken from the camera
**
** @param [out]
**      pu8_buf: Buffer to store the converted RGB565 data, size of the buffer must be aligned with the take shot
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Get_Pic_From_Shot (CAM_shot_t * pstru_shot, uint8_t * pu8_buf)
{
    /* The conversion depends on the frame format of the camera */
    CAM_frame_format_t enm_format = enm_CAM_Get_Frame_Format (g_x_cam_inst);
    if (enm_format == CAM_FORMAT_GRAYSCALE)
    {
        /* Convert grayscale (1 byte/pixel) to RGB565 */
        for (uint32_t u32_idx = 0; u32_idx < pstru_shot->u32_len; u32_idx++)
        {
            uint8_t u8_red = pstru_shot->pu8_data[u32_idx];
            uint8_t u8_green = pstru_shot->pu8_data[u32_idx];
            uint8_t u8_blue = pstru_shot->pu8_data[u32_idx];
            uint16_t u16_pixel = GUI_rgb888to565 (u8_red, u8_green, u8_blue);
            ENDIAN_PUT16 (pu8_buf, u16_pixel);
            pu8_buf += 2;
        }
    }
    else if (enm_format == CAM_FORMAT_RGB565)
    {
        /* The format matches, just copy the frame buffer data */
        memcpy (pu8_buf, pstru_shot->pu8_data, pstru_shot->u32_len);
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
