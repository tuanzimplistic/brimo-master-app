/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : control_common_ext.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 July 29
**  @brief      : Configuration of control functions
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @{
*/

#ifndef __CONTROL_COMMON_EXT_H__
#define __CONTROL_COMMON_EXT_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @brief   Table of all user control
** @details
**
** - Control_ID                 : Alias of user control
**
** - Function_to_get_control    : Name of the function to get the control structure
**
*/
#define GUI_CONTROL_TABLE(X)                                                     \
/*-----------------------------------------------------------------------------*/\
/*  Control_ID                      Function_to_get_control                    */\
/*-----------------------------------------------------------------------------*/\
                                                                                 \
/*  Notify message box */                                                        \
X(  GUI_CONTROL_NOTIFY_MSGBOX,      s8_GUI_Get_Notify_Msgbox_Control            )\
                                                                                 \
/*  Query message box */                                                         \
X(  GUI_CONTROL_QUERY_MSGBOX,       s8_GUI_Get_Query_Msgbox_Control             )\
                                                                                 \
/*  Progress message box */                                                      \
X(  GUI_CONTROL_PROGRESS_MSGBOX,    s8_GUI_Get_Progress_Msgbox_Control          )\
                                                                                 \
/*-----------------------------------------------------------------------------*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#endif /* __CONTROL_COMMON_EXT_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
