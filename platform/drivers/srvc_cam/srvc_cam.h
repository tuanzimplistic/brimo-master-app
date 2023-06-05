/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_cam.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Jan 12
**  @brief      : Public header of Srvc_Cam module
**  @namespace  : CAM
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Cam
** @{
*/

#ifndef __SRVC_CAM_H__
#define __SRVC_CAM_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"         /* Use common definitions */
#include "img_converters.h"     /* Public APIs of image converting functions */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Handle to manage an camera instance */
typedef struct CAM_obj *        CAM_inst_t;

/** @brief  Status returned by APIs of Srvc_Cam module */
enum
{
    CAM_OK                      = 0,    //!< The function executed successfully
    CAM_ERR                     = -1,   //!< There is unknown error while executing the function
    CAM_ERR_NOT_YET_INIT        = -2,   //!< The given instance is not initialized yet
    CAM_ERR_BUSY                = -3,   //!< The function failed because the given instance is busy
};

/** @brief  Structure wrapping data of a shot taken from camera module */
typedef struct
{
    uint8_t *       pu8_data;           //!< Pointer to the pixel data
    uint32_t        u32_len;            //!< Length of the buffer in bytes
    uint16_t        u16_width;          //!< Width of the shot in pixels
    uint16_t        u16_height;         //!< Height of the shot in pixels
    void *          pv_internal_fb;     //!< Camera frame buffer, don't use this
} CAM_shot_t;

/** @brief  Frame format */
typedef enum
{
    CAM_FORMAT_GRAYSCALE,               //!< Grayscale format (1 bytes/pixel)
    CAM_FORMAT_RGB565,                  //!< RGB565 format (2 bytes/pixel)
    CAM_NUM_FORMATS
} CAM_frame_format_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets the single instance of camera module */
extern int8_t s8_CAM_Get_Inst (CAM_inst_t * px_inst);

/*
** Gets a shot from a camera.
** The shot buffer must be returned to the camera with s8_CAM_Release_Shot()
*/
extern int8_t s8_CAM_Take_Shot (CAM_inst_t x_inst, CAM_shot_t * pstru_shot);

/* Returns to a camera a shot buffer taken with s8_CAM_Take_Shot() */
extern int8_t s8_CAM_Release_Shot (CAM_inst_t x_inst, const CAM_shot_t * pstru_shot);

/* Gets frame buffer size in pixels of a camera */
extern void v_CAM_Get_Frame_Size (CAM_inst_t x_inst, uint16_t * pu16_width, uint16_t * pu16_height);

/* Gets frame format of a camera */
extern CAM_frame_format_t enm_CAM_Get_Frame_Format (CAM_inst_t x_inst);

#endif /* __SRVC_CAM_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/