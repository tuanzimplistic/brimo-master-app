/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_cam.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Jan 12
**  @brief      : Implementation of Srvc_Cam module
**  @namespace  : CAM
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Cam
** @brief       Srvc_Cam module encapsulates provides APIs to work with camera module
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_cam.h"               /* Public header of this module */
#include "srvc_io_tca9534.h"        /* Use TCA9534A to control camera module */
#include "hwa_i2c_master.h"         /* Use I2C Master to communicate with camera module */
#include "esp_camera.h"             /* Use ESP32-camera library */

#include "freertos/FreeRTOS.h"      /* Use FreeRTOS */
#include "freertos/task.h"          /* Use FreeRTOS's vTaskDelay() */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Pin mapping of camera module */
#define CAM_PIN_D0              21
#define CAM_PIN_D1              18
#define CAM_PIN_D2              5
#define CAM_PIN_D3              19
#define CAM_PIN_D4              34
#define CAM_PIN_D5              36
#define CAM_PIN_D6              39
#define CAM_PIN_D7              35
#define CAM_PIN_VSYNC           33
#define CAM_PIN_HREF            32
#define CAM_PIN_PCLK            23
#define CAM_PIN_XCLK            22

/**
** @brief   Resolution of frames taken from camera
** @remark  Supported frame sizes:
**          + FRAMESIZE_96X96:      W = 96,     H = 96
**          + FRAMESIZE_QQVGA:      W = 160,    H = 120
**          + FRAMESIZE_QCIF:       W = 176,    H = 144
**          + FRAMESIZE_HQVGA:      W = 240,    H = 176
**          + FRAMESIZE_240X240:    W = 240,    H = 240
**          + FRAMESIZE_QVGA:       W = 320,    H = 240
**          + FRAMESIZE_CIF:        W = 400,    H = 296
**          + FRAMESIZE_HVGA:       W = 480,    H = 320
*/
#define CAM_FRAME_SIZE          FRAMESIZE_QVGA

/**
** @brief   Format of frames taken from the camera
** @remark  Supported frame formats:
**          + PIXFORMAT_GRAYSCALE:  1 bytes/pixel
**          + PIXFORMAT_RGB565:     2 bytes/pixel
*/
#define CAM_FRAME_FORMAT        PIXFORMAT_GRAYSCALE

/** @brief  Structure to manage a camera object */
struct CAM_obj
{
    bool                        b_initialized;          //!< Specifies whether the object has been initialized or not
    uint16_t                    u16_frame_width;        //!< Frame buffer's width in pixels
    uint16_t                    u16_frame_height;       //!< Frame buffer's height in pixels
    CAM_frame_format_t          enm_frame_format;       //!< Frame buffer format
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Cam";

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Single instance of camera module */
static struct CAM_obj g_stru_cam_obj =
{
    .b_initialized      = false,
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifdef USE_MODULE_ASSERT
 static bool b_CAM_Is_Valid_Inst (CAM_inst_t x_inst);
#endif

static int8_t s8_CAM_Init_Module (void);
static int8_t s8_CAM_Init_Inst (CAM_inst_t x_inst);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets the single instance of camera module
**
** @param [out]
**      px_inst: Container to store the retrieved instance
**
** @return
**      @arg    CAM_OK
**      @arg    CAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_CAM_Get_Inst (CAM_inst_t * px_inst)
{
    CAM_inst_t      x_inst = NULL;
    int8_t          s8_result = CAM_OK;

    /* Validation */
    ASSERT_PARAM (px_inst != NULL);

    /* Initialize */
    *px_inst = NULL;

    /* If this module has not been initialized, do that now */
    if (s8_result >= CAM_OK)
    {
        if (!g_b_initialized)
        {
            s8_result = s8_CAM_Init_Module ();
            if (s8_result >= CAM_OK)
            {
                g_b_initialized = true;
            }
        }
    }

    /* If the retrieved instance has not been initialized yet, do that now */
    if (s8_result >= CAM_OK)
    {
        x_inst = &g_stru_cam_obj;
        if (!x_inst->b_initialized)
        {
            s8_result = s8_CAM_Init_Inst (x_inst);
            if (s8_result >= CAM_OK)
            {
                x_inst->b_initialized = true;
            }
        }
    }

    /* Return instance of the camera module */
    if (s8_result >= CAM_OK)
    {
        *px_inst = x_inst;
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets a shot from a camera.
**
** @note
**      The shot buffer must be returned to the camera with s8_CAM_Release_Shot()
**
** @param [in]
**      x_inst: A specific camera instance
**
** @param [out]
**      pstru_shot: Information of the shot
**
** @return
**      @arg    CAM_OK
**      @arg    CAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_CAM_Take_Shot (CAM_inst_t x_inst, CAM_shot_t * pstru_shot)
{
    /* Validation */
    ASSERT_PARAM (b_CAM_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM (pstru_shot != NULL);

    /* Get current frame buffer of the camera */
    camera_fb_t * pstru_fb = esp_camera_fb_get ();
    if (pstru_fb == NULL)
    {
        LOGE ("Failed to get the current frame buffer from the camera");
        return CAM_ERR;
    }

    /* Information of the shot */
    pstru_shot->pu8_data = pstru_fb->buf;
    pstru_shot->u32_len = pstru_fb->len;
    pstru_shot->u16_width = pstru_fb->width;
    pstru_shot->u16_height = pstru_fb->height;
    pstru_shot->pv_internal_fb = pstru_fb;

    return CAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Returns to a camera a shot buffer taken with s8_CAM_Take_Shot()
**
** @param [in]
**      x_inst: A specific camera instance
**
** @param [in]
**      pstru_shot: The shot taken previously
**
** @return
**      @arg    CAM_OK
**      @arg    CAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_CAM_Release_Shot (CAM_inst_t x_inst, const CAM_shot_t * pstru_shot)
{
    /* Validation */
    ASSERT_PARAM (b_CAM_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM (pstru_shot != NULL);

    /* Return the frame buffer to camera module for later reuse */
    esp_camera_fb_return ((camera_fb_t *)pstru_shot->pv_internal_fb);

    return CAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets frame buffer size in pixels of a camera
**
** @param [in]
**      x_inst: A specific camera instance
**
** @param [out]
**      pu16_width: Frame buffer's width in pixels
**
** @param [out]
**      pu16_height: Frame buffer's height in pixels
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CAM_Get_Frame_Size (CAM_inst_t x_inst, uint16_t * pu16_width, uint16_t * pu16_height)
{
    /* Validation */
    ASSERT_PARAM (b_CAM_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM ((pu16_width != NULL) && (pu16_height != NULL));

    /* Return frame buffer size */
    *pu16_width = x_inst->u16_frame_width;
    *pu16_height = x_inst->u16_frame_height;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets frame format of a camera
**
** @param [in]
**      x_inst: A specific camera instance
**
** @return
**      Camera frame format
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
CAM_frame_format_t enm_CAM_Get_Frame_Format (CAM_inst_t x_inst)
{
    /* Validation */
    ASSERT_PARAM (b_CAM_Is_Valid_Inst (x_inst) && x_inst->b_initialized);

    /* Return frame format */
    return x_inst->enm_frame_format;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_Cam module
**
** @return
**      @arg    CAM_OK
**      @arg    CAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_CAM_Init_Module (void)
{
    /* Do nothing */
    return CAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes a camera instance
**
** @param [in]
**      x_inst: A specific OV2640 instance
**
** @return
**      @arg    CAM_OK
**      @arg    CAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_CAM_Init_Inst (CAM_inst_t x_inst)
{
    /* Determine frame buffer size */
    x_inst->u16_frame_width =  (CAM_FRAME_SIZE == FRAMESIZE_96X96)    ? 96  :
                               (CAM_FRAME_SIZE == FRAMESIZE_QQVGA)    ? 160 :
                               (CAM_FRAME_SIZE == FRAMESIZE_QCIF)     ? 176 :
                               (CAM_FRAME_SIZE == FRAMESIZE_HQVGA)    ? 240 :
                               (CAM_FRAME_SIZE == FRAMESIZE_240X240)  ? 240 :
                               (CAM_FRAME_SIZE == FRAMESIZE_QVGA)     ? 320 :
                               (CAM_FRAME_SIZE == FRAMESIZE_CIF)      ? 400 :
                               (CAM_FRAME_SIZE == FRAMESIZE_HVGA)     ? 480 : 0;
    x_inst->u16_frame_height = (CAM_FRAME_SIZE == FRAMESIZE_96X96)    ? 96  :
                               (CAM_FRAME_SIZE == FRAMESIZE_QQVGA)    ? 120 :
                               (CAM_FRAME_SIZE == FRAMESIZE_QCIF)     ? 144 :
                               (CAM_FRAME_SIZE == FRAMESIZE_HQVGA)    ? 176 :
                               (CAM_FRAME_SIZE == FRAMESIZE_240X240)  ? 240 :
                               (CAM_FRAME_SIZE == FRAMESIZE_QVGA)     ? 240 :
                               (CAM_FRAME_SIZE == FRAMESIZE_CIF)      ? 296 :
                               (CAM_FRAME_SIZE == FRAMESIZE_HVGA)     ? 320 : 0;
    if ((x_inst->u16_frame_width == 0) || (x_inst->u16_frame_height == 0))
    {
        LOGE ("Unsupported camera frame buffer size");
        return CAM_ERR;
    }

    /* Frame buffer format */
    x_inst->enm_frame_format = (CAM_FRAME_FORMAT == PIXFORMAT_GRAYSCALE) ? CAM_FORMAT_GRAYSCALE :
                               (CAM_FRAME_FORMAT == PIXFORMAT_RGB565)    ? CAM_FORMAT_RGB565    : CAM_NUM_FORMATS;
    if (x_inst->enm_frame_format == CAM_NUM_FORMATS)
    {
        LOGE ("Unsupported camera frame format");
        return CAM_ERR;
    }

    /* Turn on power supply for the camera module */
    GPIOX_inst_t x_pwr_en;
    s8_GPIOX_Get_Inst (GPIOX_LCD_CAM_PWR, &x_pwr_en);
    s8_GPIOX_Write_Active (x_pwr_en, true);

    /* Disable power down mode of the camera module */
    GPIOX_inst_t x_cam_pwdn;
    s8_GPIOX_Get_Inst (GPIOX_CSI_PWDN, &x_cam_pwdn);
    s8_GPIOX_Write_Active (x_cam_pwdn, false);

    /* Reset the camera module */
    GPIOX_inst_t x_cam_rst;
    s8_GPIOX_Get_Inst (GPIOX_CAMERA_RST, &x_cam_rst);
    s8_GPIOX_Write_Active (x_cam_rst, true);
    vTaskDelay (pdMS_TO_TICKS (10));
    s8_GPIOX_Write_Active (x_cam_rst, false);
    vTaskDelay (pdMS_TO_TICKS (10));

    /* Configure the camera */
    camera_config_t stru_cam_cfg =
    {
        /* Pins which will be controlled manually */
        .pin_pwdn       = -1,
        .pin_reset      = -1,
        .pin_sscb_sda   = -1,
        .pin_sscb_scl   = -1,

        /* Pins which will be controlled by camera driver */
        .pin_d0         = CAM_PIN_D0,
        .pin_d1         = CAM_PIN_D1,
        .pin_d2         = CAM_PIN_D2,
        .pin_d3         = CAM_PIN_D3,
        .pin_d4         = CAM_PIN_D4,
        .pin_d5         = CAM_PIN_D5,
        .pin_d6         = CAM_PIN_D6,
        .pin_d7         = CAM_PIN_D7,
        .pin_vsync      = CAM_PIN_VSYNC,
        .pin_href       = CAM_PIN_HREF,
        .pin_pclk       = CAM_PIN_PCLK,
        .pin_xclk       = CAM_PIN_XCLK,

        .xclk_freq_hz   = 20000000,
        .ledc_timer     = LEDC_TIMER_0,
        .ledc_channel   = LEDC_CHANNEL_0,

        .pixel_format   = CAM_FRAME_FORMAT,
        .frame_size     = CAM_FRAME_SIZE,
        .fb_count       = 1,

        .jpeg_quality   = 12,
        .grab_mode      = CAMERA_GRAB_WHEN_EMPTY,
    };
    if (esp_camera_init (&stru_cam_cfg) != ESP_OK)
    {
        LOGE ("Failed to initialize camera module");
        return CAM_ERR;
    }

    return CAM_OK;
}

#ifdef USE_MODULE_ASSERT

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Check if an instance is a vaild instance of this module
**
** @param [in]
**      x_inst: instance to check
**
** @return
**      Result
**      @arg    true: Valid instance
**      @arg    false: Invalid instance
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static bool b_CAM_Is_Valid_Inst (CAM_inst_t x_inst)
{
    return (x_inst == &g_stru_cam_obj);
}

#endif

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
