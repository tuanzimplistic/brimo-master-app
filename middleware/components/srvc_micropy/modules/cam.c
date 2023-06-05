/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : cam.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Dec 18
**  @brief      : C-implementation of cam MP module
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Provides interface so that MicroPython script can interract with camera
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "cam.h"                        /* Public header of this MP module */
#include "srvc_micropy.h"               /* Use common return code */
#include "srvc_cam.h"                   /* Use camera service */
#include "srvc_param.h"                 /* Use Parameter service */
#include "app_gui_mngr.h"
#include "quirc.h"

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Quality of pictures taken from camera */
#define MP_PICTURE_QUALITY              90

#define MP_PICTURE_WIDTH                240
#define MP_PICTURE_HEIGHT               240

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Micropy";

static CAM_inst_t   g_x_cam_instance = NULL;
static struct quirc *qr = NULL;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Creates a folder and all intermediate folders given an absolute path
**
** @details
**      For example, if path is:
**      + "/a/b/c/d"  : folder "/a", "/a/b", and "/a/b/c" shall be created (if not existing), "d" is regarded as a file
**      + "/a/b/c/d/" : folder "/a", "/a/b", "/a/b/c", and "/a/b/c/d" shall be created (if not existing)
**      + "a/b/c/d/"  : folder "/a", "/a/b", "/a/b/c", and "/a/b/c/d" shall be created (if not existing)
**
** @param [in]
**      pstri_path: Path of the folder to create
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MP_Create_Folder (const char * pstri_path)
{
    /* Allocate temporary memory for path string */
    uint16_t u16_path_len = strlen (pstri_path) + 1;
    char * pstri_path_tmp = malloc (u16_path_len);
    if (pstri_path_tmp == NULL)
    {
        LOGE ("Failed to allocate memory for path string");
        return;
    }
    memcpy (pstri_path_tmp, pstri_path, u16_path_len);

    /* Create folder(s) */
    for (uint16_t u16_idx = 0; u16_idx < u16_path_len; u16_idx++)
    {
        if ((pstri_path_tmp[u16_idx] == '/') && (u16_idx != 0))
        {
            pstri_path_tmp[u16_idx] = '\0';
            lfs2_mkdir (g_px_lfs2, pstri_path_tmp);
            pstri_path_tmp[u16_idx] = '/';
        }
    }

    /* Cleanup */
    free (pstri_path_tmp);
}

static bool cam_init(void)
{
    bool b_success = true;
    if (s8_CAM_Get_Inst (&g_x_cam_instance) != CAM_OK)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to access the camera");
        b_success = false;
    }
    sensor_t* s = esp_camera_sensor_get();
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_aec_value(s, 300);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
    return b_success;
}

static bool cam_take_shot(CAM_shot_t * stru_shot)
{
    stru_shot->pu8_data = NULL;
    if (!g_x_cam_instance || s8_CAM_Take_Shot (g_x_cam_instance, stru_shot) != CAM_OK)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to take picture from the camera");
        return false;
    }
    return true;
}

static void cam_release_shot(CAM_shot_t * stru_shot)
{
    if (stru_shot->pu8_data != NULL)
    {
        s8_CAM_Release_Shot (g_x_cam_instance, stru_shot);
    }
}

static void dummy_shot (void)
{
    CAM_shot_t stru_shot;
    cam_take_shot(&stru_shot);
    cam_release_shot(&stru_shot);
}

static bool save_grayscale(uint8_t *gray_buf, int len, int width, 
                                int height, const char *pstri_filepath)
{
    bool b_success = true;
    size_t x_jpg_len;
    uint8_t * pu8_jpg_buf;

    uint8_t *pu8_rgb_buf = (uint8_t *)malloc(3 * len);
    for(int i=0; i < len; i++)
    {
        uint8_t pixel = gray_buf[i];
        pu8_rgb_buf[3*i] = pixel;
        pu8_rgb_buf[3*i+1] = pixel;
        pu8_rgb_buf[3*i+2] = pixel;
    }
    bool jpeg_converted = fmt2jpg(pu8_rgb_buf, 3 * len, width, height, 
                            PIXFORMAT_RGB888, MP_PICTURE_QUALITY, &pu8_jpg_buf, &x_jpg_len);
    if(!jpeg_converted)
    {
        LOGE("JPEG compression failed");
        b_success = false;
    }

    /* Save the JPG picture into a file */
    if (b_success)
    {
        lfs2_file_t     x_file;

        /* Create destination folder(s) */
        v_MP_Create_Folder (pstri_filepath);

        /* Open destination file to save the picture */
        if (lfs2_file_open (g_px_lfs2, &x_file, pstri_filepath, 
                        LFS2_O_WRONLY | LFS2_O_CREAT | LFS2_O_TRUNC) < 0)
        {
            mp_raise_msg (&mp_type_OSError, "Failed to open file for writing");
            b_success = false;
        }

        /* Store the JPG buffer to file */
        if (b_success)
        {
            if (lfs2_file_write (g_px_lfs2, &x_file, pu8_jpg_buf, x_jpg_len) != x_jpg_len)
            {
                mp_raise_msg (&mp_type_OSError, "Failed to write picture data into file");
                lfs2_remove (g_px_lfs2, pstri_filepath);
                b_success = false;
            }
        }

        if (lfs2_file_close (g_px_lfs2, &x_file) < 0)
        {
            mp_raise_msg (&mp_type_OSError, "Failed to save picture file");
        }
    }
    free(pu8_jpg_buf);
    free(pu8_rgb_buf);
    return b_success;
}

static bool load_grayscale(uint8_t ** gray_buf, uint16_t u16_width, 
                            uint16_t u16_height, const char *pstri_path)
{
    uint8_t *   pu8_jpg_buf = NULL;
    uint8_t *   rgb_buf = NULL;
    bool        b_success = true;

    /* Ensure that the given file exists */
    struct lfs2_info stru_file_info;
    if (lfs2_stat (g_px_lfs2, pstri_path, &stru_file_info) < 0)
    {
        LOGE ("File %s doesn't exist", pstri_path);
        b_success = false;
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

    if (b_success)
    {
        rgb_buf = (uint8_t *)malloc(u16_width * u16_height * 3);
        if (fmt2rgb888(pu8_jpg_buf, stru_file_info.size, PIXFORMAT_JPEG, rgb_buf) == false)
        {
            LOGE ("Failed to decode the given JPG picture to RGB565 format");
            b_success = false;
        }
    }

    if (pu8_jpg_buf != NULL)
    {
        free (pu8_jpg_buf);
    } 

    if (b_success)
    {  
        *gray_buf = (uint8_t *)malloc(u16_width * u16_height);
        for(int i = 0; i < u16_width * u16_height; i++)
        {
            (*gray_buf)[i] = rgb_buf[3 * i];
        }
    }

    if (rgb_buf != NULL)
    {
        free (rgb_buf);
    } 
    return b_success;
}

static bool take_picture (const char * pstri_filepath)
{
    CAM_shot_t stru_shot;

    dummy_shot();

    cam_take_shot(&stru_shot);

    save_grayscale(stru_shot.pu8_data, stru_shot.u32_len, 
                    stru_shot.u16_width, stru_shot.u16_height, pstri_filepath);

    cam_release_shot(&stru_shot);
    return true;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Takes a picture from the camera (JPG format) and saves it as a file in filesystem
**
** @param [in]
**      x_filepath: Full path and name of the picture file (for example, "/camera/picture1.jpg"). If any folders in
**                  the path do not exist, they will be created
**
** @return
**      @arg    false: failed to take and save the picture
**      @arg    true: the picture has been taken and saved successfully
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_CV_Take_Picture (mp_obj_t x_filepath)
{
    /* Validate data type */
    if (!mp_obj_is_str (x_filepath))
    {
        mp_raise_msg (&mp_type_TypeError, "Filename must be a string");
        return mp_const_false;
    }
    const char *    pstri_filepath = mp_obj_str_get_str (x_filepath);
    take_picture(pstri_filepath);
    return mp_const_true;
}

extern mp_obj_t x_MP_CV_Take_Picture_Exposure (mp_obj_t x_filepath, mp_obj_t exp_value)
{
    /* Validate data type */
    if (!mp_obj_is_str (x_filepath) && !mp_obj_is_int (exp_value))
    {
        mp_raise_msg (&mp_type_TypeError, "wrong inputs");
        return mp_const_false;
    }
    const char *    pstri_filepath = mp_obj_str_get_str (x_filepath);
    int exposure_value = mp_obj_int_get_truncated (exp_value);
    LOGI("exposure_val: %d", exposure_value);
    sensor_t * s = esp_camera_sensor_get();
    s->set_exposure_ctrl(s, 1);  
    s->set_aec_value(s, exposure_value);  
    take_picture(pstri_filepath);
    s8_GUI_Set_Data (GUI_DATA_DEBUG_PICTURE, pstri_filepath, 0);
    return mp_const_true;
}

static void dump_data(const struct quirc_data *data) 
{
    LOGI("Version: %d", data->version);
	LOGI("ECC level: %c", "MLHQ"[data->ecc_level]);
	LOGI("Length: %d", data->payload_len);
	LOGI("Payload: %s", data->payload);
}

mp_obj_t x_MP_CV_Scan_QR(void)
{
    CAM_shot_t stru_shot;
    quirc_decode_error_t err;
    struct quirc_code *code;
    struct quirc_data *data;
    int w;
    int h;

    dummy_shot();

    cam_take_shot(&stru_shot);

    uint8_t *buf = quirc_begin(qr, &w, &h);

    memcpy(buf, stru_shot.pu8_data, stru_shot.u32_len);

    quirc_end(qr);

    save_grayscale(stru_shot.pu8_data, stru_shot.u32_len, 
                    stru_shot.u16_width, stru_shot.u16_height, "qr.jpg");
    s8_GUI_Set_Data (GUI_DATA_DEBUG_PICTURE, "qr.jpg", 0);

    cam_release_shot(&stru_shot);

    int id_count = quirc_count(qr);

	if (id_count == 0) 
    {
		LOGE("not a valid qrcode");
        return mp_const_false;
	}

    for (int i = 0; i < id_count; i++) 
    {
        code = (struct quirc_code *)malloc(sizeof(struct quirc_code));
        data = (struct quirc_data *)malloc(sizeof(struct quirc_data));
		
		quirc_extract(qr, i, code);

		err = quirc_decode(code, data);

		if (err) 
        {
			LOGE("Decoding FAILED: %s", quirc_strerror(err));
		} 
        else 
        {
            dump_data(data);
        }
        free(code);
        free(data);
	}

    return mp_const_true;
}

mp_obj_t x_MP_CV_Init (void)
{
    LOGI("init camera");

    cam_init();

    qr = quirc_new();
	if (!qr) 
    {
		LOGE("couldn't allocate QR decoder");
		return mp_const_false;
	}

	if (quirc_resize(qr, MP_PICTURE_WIDTH, MP_PICTURE_HEIGHT) < 0) 
    {
		LOGE("couldn't allocate QR buffer");
		return mp_const_false;
	}

    return mp_const_true;
}

mp_obj_t x_MP_CV_Release (void)
{
    LOGI("release camera");

    quirc_destroy(qr);

    return mp_const_true;
}


/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
