/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : screen_common.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 May 24
**  @brief      : Common types, structures, etc. used for all UI screens
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @{
*/

#ifndef __SCREEN_COMMON_H__
#define __SCREEN_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "gui_common.h"                 /* Use common definitions of GUI */
#include "screen_common_ext.h"          /* Use GUI_SCREEN_TABLE */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Expand entries in GUI_SCREEN_TABLE as enumeration of screen IDs */
#define GUI_SCREEN_TABLE_EXPAND_AS_SCREEN_ID(SCREEN_ID, ...)        SCREEN_ID,
typedef enum
{
    GUI_SCREEN_TABLE (GUI_SCREEN_TABLE_EXPAND_AS_SCREEN_ID)
    GUI_NUM_SCREENS

} GUI_screen_id_t;

/** @brief  Screen's result code when done */
typedef enum
{
    GUI_SCREEN_RESULT_NONE,                     //!< No result, the screen is still working
    GUI_SCREEN_RESULT_NEXT,                     //!< Go to the next screen specified by GUI_screen_t.pstru_next
    GUI_SCREEN_RESULT_BACK,                     //!< Go back to the previous screen

} GUI_screen_result_t;

/** @brief  Structure wrapping information of a screen */
typedef struct GUI_screen
{
    struct GUI_screen *     pstru_next;         //!< The next screen to display after this screen
    struct GUI_screen *     pstru_prev;         //!< The previous screen displayed before this screen
    lv_obj_t *              px_lv_screen;       //!< LVGL screen object
    const char *            pstri_name;         //!< Screen name
    const lv_img_dsc_t *    px_icon;            //!< Screen icon
    GUI_action_t            pfnc_start;         //!< Function invoked to start the screen
    GUI_action_t            pfnc_stop;          //!< Function invoked to stop the screen
    GUI_action_t            pfnc_run;           //!< Function invoked periodically to run the screen
    GUI_screen_result_t     enm_result;         //!< Result when this screen has done its job

} GUI_screen_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets data structure wrapping a GUI screen */
extern int8_t s8_GUI_Get_Screen (GUI_screen_id_t enm_screen_id, GUI_screen_t ** ppstru_screen);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SCREEN_COMMON_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
