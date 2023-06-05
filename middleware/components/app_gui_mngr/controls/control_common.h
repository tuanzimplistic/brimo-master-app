/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : control_common.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jul 29
**  @brief      : Common types, structures, etc. used for all UI user controls
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @{
*/

#ifndef __CONTROL_COMMON_H__
#define __CONTROL_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "gui_common.h"                 /* Use common definitions of GUI */
#include "control_common_ext.h"         /* Use GUI_CONTROL_TABLE */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Expand entries in GUI_CONTROL_TABLE as enumeration of control IDs */
#define GUI_CONTROL_TABLE_EXPAND_AS_CONTROL_ID(CONTROL_ID, ...)     CONTROL_ID,
typedef enum
{
    GUI_CONTROL_TABLE (GUI_CONTROL_TABLE_EXPAND_AS_CONTROL_ID)
    GUI_NUM_CONTROLS

} GUI_control_id_t;

/** @brief  Structure wrapping information of an user control */
typedef struct GUI_control
{
    GUI_action_t    pfnc_run;           //!< Function invoked periodically to run the control

} GUI_control_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets data structure wrapping an user control */
extern int8_t s8_GUI_Get_Control (GUI_control_id_t enm_control_id, GUI_control_t ** ppstru_control);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __CONTROL_COMMON_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
