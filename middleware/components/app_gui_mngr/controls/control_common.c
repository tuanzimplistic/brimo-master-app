/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : control_common.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 July 29
**  @brief      : Common functions used by all UI controls
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       This files provides helper APIs for all the controls to use
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "control_common.h"             /* Common header of all controls */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Expand entries in GUI_CONTROL_TABLE as prototypes of get control function */
#define GUI_CONTROL_TABLE_EXPAND_AS_GET_CONTROL_PROTOTYPE(CONTROL_ID, GET_CONTROL_FNC)          \
    extern int8_t GET_CONTROL_FNC (GUI_control_t ** ppstru_control);

/** @brief  Expand entries in GUI_CONTROL_TABLE as initialization values for array of get control function pointers */
#define GUI_CONTROL_TABLE_EXPAND_AS_GET_CONTROL_FNC_ARRAY_INIT(CONTROL_ID, GET_CONTROL_FNC)     \
    GET_CONTROL_FNC,

/** @brief  Pointer of get control function */
typedef int8_t (*GUI_Get_Control_Cb_t) (GUI_control_t ** ppstru_control);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

GUI_CONTROL_TABLE (GUI_CONTROL_TABLE_EXPAND_AS_GET_CONTROL_PROTOTYPE)

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "App_Gui_Mngr";

/** @brief  Array of get control function pointers */
static GUI_Get_Control_Cb_t g_apfnc_get_control_cb [] =
{
    GUI_CONTROL_TABLE (GUI_CONTROL_TABLE_EXPAND_AS_GET_CONTROL_FNC_ARRAY_INIT)
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
**      Gets data structure wrapping an user control
**
** @param [in]
**      enm_control_id: ID of the user control to get
**
** @param [out]
**      ppstru_control: Pointer to the structure wrapping the user control
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Get_Control (GUI_control_id_t enm_control_id, GUI_control_t ** ppstru_control)
{
    ASSERT_PARAM ((ppstru_control != NULL) && (enm_control_id < GUI_NUM_CONTROLS));

    if (g_apfnc_get_control_cb [enm_control_id] != NULL)
    {
        return (g_apfnc_get_control_cb [enm_control_id] (ppstru_control));
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
