/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_lvgl.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jul 5
**  @brief      : Public header of Srvc_LVGL module
**  @namespace  : LVGL
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_LVGL
** @{
*/

#ifndef __SRVC_LVGL_H__
#define __SRVC_LVGL_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"             /* Use common definitions */
#include "lvgl.h"                   /* Public API of LVGL library */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Status returned by APIs of Srvc_LVGL module */
enum
{
    LVGL_OK                         = 0,        //!< The function executed successfully
    LVGL_ERR                        = -1,       //!< There is unknown error while executing the function
    LVGL_ERR_NOT_YET_INIT           = -2,       //!< The given instance is not initialized yet
    LVGL_ERR_BUSY                   = -3,       //!< The function failed because the given instance is busy
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Initializes Srvc_LVGL module */
extern int8_t s8_LVGL_Init (void);

/* Manually runs Srvc_LVGL module. This function must be called periodically */
extern int8_t s8_LVGL_Run (uint32_t u32_ms_elapsed);

/* Enables or disables idle mode of Srvc_LVGL module */
extern int8_t s8_LVGL_Set_Idle_Mode (bool b_idle);

#endif /* __SRVC_LVGL_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
