/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : screen_common_ext.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 July 28
**  @brief      : Configuration of screen functions
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @{
*/

#ifndef __SCREEN_COMMON_EXT_H__
#define __SCREEN_COMMON_EXT_H__

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
** @brief   Table of all GUI screens
** @details
**
** - Screen_ID              : Alias of screen
**
** - Function_to_get_screen : Name of the function to get screen structure
**
*/
#define GUI_SCREEN_TABLE(X)                                                      \
/*-----------------------------------------------------------------------------*/\
/*  Screen_ID                       Function_to_get_screen                     */\
/*-----------------------------------------------------------------------------*/\
                                                                                 \
/*  Splash screen */                                                             \
X(  GUI_SCREEN_SPLASH,              s8_GUI_Get_Splash_Screen                    )\
                                                                                 \
/*  Wifi setting screen */                                                       \
X(  GUI_SCREEN_WIFI_SETTING,        s8_GUI_Get_Wifi_Setting_Screen              )\
                                                                                 \
/*  Roti making screen */                                                        \
X(  GUI_SCREEN_ROTI_MAKING,         s8_GUI_Get_Roti_Making_Screen               )\
                                                                                 \
/*  Virtual keyboard screen */                                                   \
X(  GUI_SCREEN_VIRTUAL_KEYBOARD,    s8_GUI_Get_Virtual_Keyboard_Screen          )\
                                                                                 \
/*  Menu screen */                                                               \
X(  GUI_SCREEN_MENU,                s8_GUI_Get_Menu_Screen                      )\
                                                                                 \
/*  Developer screen */                                                          \
X(  GUI_SCREEN_DEVELOPER,           s8_GUI_Get_Developer_Screen                 )\
                                                                                 \
/*  Camera screen */                                                             \
X(  GUI_SCREEN_CAM,                 s8_GUI_Get_Cam_Screen                       )\
                                                                                 \
/*-----------------------------------------------------------------------------*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#endif /* __SCREEN_COMMON_EXT_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
