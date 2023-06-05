/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : screen_common.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 May 24
**  @brief      : Common functions used by all UI screens
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       This files provides helper APIs for all the screens to use
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "screen_common.h"              /* Common header of all screens */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Expand entries in GUI_SCREEN_TABLE as prototypes of get screen function */
#define GUI_SCREEN_TABLE_EXPAND_AS_GET_SCREEN_PROTOTYPE(SCREEN_ID, GET_SCREEN_FNC)          \
    extern int8_t GET_SCREEN_FNC (GUI_screen_t ** ppstru_screen);

/** @brief  Expand entries in GUI_SCREEN_TABLE as initialization values for array of get screen function pointers */
#define GUI_SCREEN_TABLE_EXPAND_AS_GET_SCREEN_FNC_ARRAY_INIT(SCREEN_ID, GET_SCREEN_FNC)     \
    GET_SCREEN_FNC,

/** @brief  Pointer of get screen function */
typedef int8_t (*GUI_Get_Screen_Cb_t) (GUI_screen_t ** ppstru_screen);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

GUI_SCREEN_TABLE (GUI_SCREEN_TABLE_EXPAND_AS_GET_SCREEN_PROTOTYPE)

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "App_Gui_Mngr";

/** @brief  Array of get screen function pointers */
static GUI_Get_Screen_Cb_t g_apfnc_get_screen_cb [] =
{
    GUI_SCREEN_TABLE (GUI_SCREEN_TABLE_EXPAND_AS_GET_SCREEN_FNC_ARRAY_INIT)
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets data structure wrapping a GUI screen
**
** @param [in]
**      enm_screen_id: ID of the screen to get
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
int8_t s8_GUI_Get_Screen (GUI_screen_id_t enm_screen_id, GUI_screen_t ** ppstru_screen)
{
    ASSERT_PARAM ((ppstru_screen != NULL) && (enm_screen_id < GUI_NUM_SCREENS));

    if (g_apfnc_get_screen_cb [enm_screen_id] != NULL)
    {
        return (g_apfnc_get_screen_cb [enm_screen_id] (ppstru_screen));
    }

    return GUI_ERR;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
