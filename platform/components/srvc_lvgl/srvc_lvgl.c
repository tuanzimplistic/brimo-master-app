/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_lvgl.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jul 5
**  @brief      : Implementation of Srvc_LVGL module
**  @namespace  : LVGL
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_LVGL
** @brief       Provides a light wrapper that helps LVGL library to be connected with other modules of the firmware
**              such as GUI manager, LCD abstraction module, touch screen abstraction module, etc.
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_lvgl.h"                  /* Public header of this module */
#include "srvc_lcd_st7796s_demo.h"      /* Use LCD ST7796S */
#include "srvc_touch_gt911.h"           /* Use touch screen GT911 */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/semphr.h"            /* Use FreeRTOS mutex */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @brief   Size (in pixels) of display buffer
** @details
**          LVGL_DISP_BUF_SIZE value doesn't have an special meaning, but it's the size of the buffer(s) passed
**          to LVGL as display buffers. As LVGL supports partial display updates the LVGL_DISP_BUF_SIZE doesn't
**          necessarily need to be equal to the display size.
**          When using RGB displays the display buffer size will also depends on the color format being used,
**          for RGB565 each pixel needs 2 bytes.
*/
#define LVGL_DISP_BUF_SIZE              (LV_HOR_RES_MAX * LV_VER_RES_MAX / 20)


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_LVGL";

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Mutex protecting concurrent access to LVGL core */
static SemaphoreHandle_t g_x_lvgl_mutex;

/** @brief  Instance of LCD display */
static ST7796S_inst_t g_x_lcd_inst;

/** @brief  Instance of touch screen */
static GT911_inst_t g_x_touch_inst;

/** @brief  Indicates if this module is idling or not */
static bool g_b_idle = false;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static void v_LVGL_Display_Flush (lv_disp_drv_t * pstru_drv, const lv_area_t * pstru_area, lv_color_t * px_color_map);
static bool v_LVGL_Touch_Read (lv_indev_drv_t * pstru_drv, lv_indev_data_t * pstru_data);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_LVGL module
**
** @return
**      @arg    LVGL_OK
**      @arg    LVGL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_LVGL_Init (void)
{
    static lv_disp_buf_t x_disp_buf;

    /* Do nothing if this module has been initialized */
    if (g_b_initialized)
    {
        return LVGL_OK;
    }

    LOGD ("Initializing Srvc_LVGL module");

    /* Get instance of touch screen GT911 */
    if (s8_GT911_Get_Inst (&g_x_touch_inst) != GT911_OK)
    {
        LOGE ("Failed to initialize touch screen GT911");
        return LVGL_ERR;
    }

    /* Get instance of LCD ST7796S */
    if (s8_ST7796S_Get_Inst (&g_x_lcd_inst) != ST7796S_OK)
    {
        LOGE ("Failed to initialize LCD ST7796S");
        return LVGL_ERR;
    }

    /* Initialize LVGL library */
    lv_init ();

    /* Allocate double display buffer */
    lv_color_t * pv_buf1 = heap_caps_malloc (LVGL_DISP_BUF_SIZE * sizeof (lv_color_t), MALLOC_CAP_DMA);
    lv_color_t * pv_buf2 = heap_caps_malloc (LVGL_DISP_BUF_SIZE * sizeof (lv_color_t), MALLOC_CAP_DMA);
    if ((pv_buf1 == NULL) || (pv_buf2 == NULL))
    {
        LOGE ("Failed to allocate display buffer");
        if (pv_buf1 != NULL)
        {
            heap_caps_free (pv_buf1);
        }
        if (pv_buf2 != NULL)
        {
            heap_caps_free (pv_buf2);
        }
        return LVGL_ERR;
    }
    lv_disp_buf_init (&x_disp_buf, pv_buf1, pv_buf2, LVGL_DISP_BUF_SIZE);

    /* Initialize LVGL display driver */
    lv_disp_drv_t stru_disp_drv;
    lv_disp_drv_init (&stru_disp_drv);
    stru_disp_drv.flush_cb = v_LVGL_Display_Flush;
    stru_disp_drv.buffer = &x_disp_buf;
    lv_disp_drv_register (&stru_disp_drv);

    /* Register touch device as an input device */
    lv_indev_drv_t stru_indev_drv;
    lv_indev_drv_init (&stru_indev_drv);
    stru_indev_drv.read_cb = v_LVGL_Touch_Read;
    stru_indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register (&stru_indev_drv);

    /* Create mutex protecting concurrent access to LVGL core */
    g_x_lvgl_mutex = xSemaphoreCreateMutex ();

    /* Done */
    LOGD ("Initialization of Srvc_LVGL module is done");
    g_b_initialized = true;
    return LVGL_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Manually runs Srvc_LVGL module. This function must be called periodically
**
** @param [in]
**      u32_ms_elapsed: Time in milliseconds elapsed since the previous invocation of this function
**
** @return
**      @arg    LVGL_OK
**      @arg    LVGL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_LVGL_Run (uint32_t u32_ms_elapsed)
{
    /* Protect LVGL core from concurrent access */
    if (xSemaphoreTake (g_x_lvgl_mutex, portMAX_DELAY) == pdTRUE)
    {
        /* Notify LVGL of the time elapsed */
        lv_tick_inc (u32_ms_elapsed);

        /* Run LVGL core */
        lv_task_handler ();
        xSemaphoreGive (g_x_lvgl_mutex);
    }

    return LVGL_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Enables or disables idle mode of Srvc_LVGL module
**
** @note
**      In idle mode, LCD backlight is turned off. Srvc_LVGL modes automatically switches into active mode if user
**      touches the LCD.
**
** @param [in]
**      b_idle
**      @arg    true: Put the module into idle mode
**      @arg    false: Put the module into active mode
**
** @return
**      @arg    LVGL_OK
**      @arg    LVGL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_LVGL_Set_Idle_Mode (bool b_idle)
{
    /* Change mode accordingly */
    if (b_idle != g_b_idle)
    {
        g_b_idle = b_idle;

        /* Turn off LCD backlight while idling, turn it on otherwise */
        ST7796S_inst_t x_lcd;
        if (s8_ST7796S_Get_Inst (&x_lcd) == ST7796S_OK)
        {
            s8_ST7796S_Toggle_Backlight (x_lcd, !g_b_idle);
        }
    }

    return LVGL_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Writes the display buffer to the LCD
**
** @param [in]
**      pstru_drv: Display driver
**
** @param [in]
**      pstru_area: Coordinates the display area to write
**
** @param [in]
**      px_color_map: Display data
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_LVGL_Display_Flush (lv_disp_drv_t * pstru_drv, const lv_area_t * pstru_area, lv_color_t * px_color_map)
{
    ASSERT_PARAM ((pstru_drv != NULL) && (pstru_area != NULL) && (px_color_map != NULL));

    /* Send the display data to the LCD */
    s8_ST7796S_Write_Pixels (g_x_lcd_inst, pstru_area->x1, pstru_area->y1,
                                           pstru_area->x2, pstru_area->y2, (ST7796S_pixel_t *)px_color_map);

    /* Notify LVGL that flushing is done */
    lv_disp_flush_ready (pstru_drv);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets current state of the touch screen
**
** @param [in]
**      pstru_drv: Touch screen driver
**
** @param [out]
**      pstru_data: Current state of the touch screen
**
** @return
**      Indicates if there is more data to be read
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static bool v_LVGL_Touch_Read (lv_indev_drv_t * pstru_drv, lv_indev_data_t * pstru_data)
{
    ASSERT_PARAM ((pstru_drv != NULL) && (pstru_data != NULL));

    /* Get touch coordinates */
    int16_t s16_touch_x = -1;
    int16_t s16_touch_y = -1;
    pstru_data->state = LV_INDEV_STATE_REL;
    if (s8_GT911_Get_Touch (g_x_touch_inst, &s16_touch_x, &s16_touch_y) == GT911_OK)
    {
        if ((s16_touch_x != -1) && (s16_touch_y != -1))
        {
            /* While idling, first touch wakes the module up */
            if (g_b_idle)
            {
                /* Switch into active mode */
                s8_LVGL_Set_Idle_Mode (false);

                /* Manually trigger an activity on the display */
                lv_disp_trig_activity (NULL);

                /* Ignore a few touches on waking up */
                vTaskDelay (pdMS_TO_TICKS (250));
                return false;
            }

            /* Rotate to landscape */
            int16_t s16_temp = s16_touch_x;
            s16_touch_x = s16_touch_y;
            s16_touch_y = s16_temp;

            /* Invert X */
            s16_touch_x = LV_HOR_RES - s16_touch_x;

            /* Report to LVGL */
            pstru_data->point.x = s16_touch_x;
            pstru_data->point.y = s16_touch_y;
            pstru_data->state = LV_INDEV_STATE_PR;
        }
    }

    /* No more data to read */
    return false;
}


/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
