/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : gui.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Aug 3
**  @brief      : Exports functions of gui MP module for binding into MicroPython
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @{
*/

#ifndef __GUI_H__
#define __GUI_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "py/runtime.h"             /* Declaration of MicroPython interpreter */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Type of notify and query message */
typedef enum
{
    MP_MSG_INFO,                   //!< Information message
    MP_MSG_WARNING,                //!< Warning message
    MP_MSG_ERROR,                  //!< Error message
} MP_msg_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Sets value of a GUI binding data */
extern mp_obj_t x_MP_Set_Gui_Data (mp_obj_t x_data_alias, mp_obj_t x_value);

/* Gets value of a GUI binding data */
extern mp_obj_t x_MP_Get_Gui_Data (mp_obj_t x_data_alias);

/* Displays a notify message on GUI */
extern mp_obj_t x_MP_Display_Notify (MP_msg_t enm_type, mp_obj_t x_brief, mp_obj_t x_detail, int32_t s32_wait_time);

/* Displays a query message on GUI with a list of options and waits for user to select an option */
extern mp_obj_t x_MP_Display_Query (MP_msg_t enm_type, mp_obj_t x_brief, mp_obj_t x_detail,
                                    int32_t s32_wait_time, mp_obj_t x_options, int32_t s32_default_opt);

/* Gets elapsed time (in millisecond) since last user activity on GUI */
extern mp_obj_t x_MP_Get_Idle_Time (void);

/* Triggers a GUI activity (do-nothing) to keep the GUI active */
extern mp_obj_t x_MP_Keep_Active (void);

#endif /* __GUI_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
