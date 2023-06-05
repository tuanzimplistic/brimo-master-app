/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_touch_gt911.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 28
**  @brief      : Public header of Srvc_Touch_GT911 module
**  @namespace  : GT911
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Touch_GT911
** @{
*/

#ifndef __SRVC_TOUCH_GT911_H__
#define __SRVC_TOUCH_GT911_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"             /* Use common definitions */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Handle to manage a GT911 controller */
typedef struct GT911_obj *          GT911_inst_t;

/** @brief  Status returned by APIs of Srvc_Touch_GT911 module */
enum
{
    GT911_OK                        = 0,        //!< The function executed successfully
    GT911_ERR                       = -1,       //!< There is unknown error while executing the function
    GT911_ERR_NOT_YET_INIT          = -2,       //!< The given instance is not initialized yet
    GT911_ERR_BUSY                  = -3,       //!< The function failed because the given instance is busy
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets the single instance of GT911 controller. */
extern int8_t s8_GT911_Get_Inst (GT911_inst_t * px_inst);

/* Gets coordinates of current touch position. There is no touch if X or Y coordinate is -1 */
extern int8_t s8_GT911_Get_Touch (GT911_inst_t x_inst, int16_t * ps16_touch_x, int16_t * ps16_touch_y);

#endif /* __SRVC_TOUCH_GT911_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
