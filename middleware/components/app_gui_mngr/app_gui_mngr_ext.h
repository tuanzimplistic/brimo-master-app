/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : app_gui_mngr_ext.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 July 28
**  @brief      : Extended header of App_Gui_Mngr module
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @{
*/

#ifndef __APP_GUI_MNGR_EXT_H__
#define __APP_GUI_MNGR_EXT_H__

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
** @brief   Table of all GUI binding data
** @details
**
** - Data_ID        : Alias of binding data. This is used to access the data.
**
** - Data_type      : The following types are supported
**                      + uint8_t, int8_t
**                      + uint16_t, int16_t
**                      + uint32_t, int32_t
**                      + float
**                      + string: NUL-terminated string
**                      + blob: variable length binary data
**
** - Init_Value     : Initialized value of the data when it's first created. Some examples:
**                         Type     |   Default
**                      ------------+-----------------------
**                         uint8_t  |   12
**                         int16_t  |   -1234
**                         string   |   "Default string"
**                         blob     |   {1, 2, 3, 4, 5, 6}
**
*/
#define GUI_BINDING_DATA_TABLE(X)                                                                                      \
/*-------------------------------------------------------------------------------------------------------------------*/\
/*  Data_ID                         Data_type       Init_Value                                                       */\
/*-------------------------------------------------------------------------------------------------------------------*/\
                                                                                                                       \
/*  Index of the button user selects in query message box */                                                           \
X(  GUI_DATA_USER_QUERY,            int8_t,         -1                                                                )\
                                                                                                                       \
/*  [Roti making screen] Number of Roti's to be made */                                                                \
X(  GUI_DATA_ROTI_COUNT,            uint8_t,        1                                                                 )\
                                                                                                                       \
/*  [Roti making screen] Number of Roti's that have been made */                                                       \
X(  GUI_DATA_ROTI_MADE,             uint8_t,        0                                                                 )\
                                                                                                                       \
/*  [Roti making screen] Name of the recipe being used */                                                              \
X(  GUI_DATA_RECIPE_NAME,           string,         "ROTI"                                                            )\
                                                                                                                       \
/*  [Roti making screen] Name of the flour being used */                                                               \
X(  GUI_DATA_FLOUR_NAME,            string,         "Pillsbury gold wholewheat atta"                                  )\
                                                                                                                       \
/*  [Roti making screen] Roast level (1 -> 5) */                                                                       \
X(  GUI_DATA_ROAST_LEVEL,           uint8_t,        5                                                                 )\
                                                                                                                       \
/*  [Roti making screen] Thickness level (1 -> 5) */                                                                   \
X(  GUI_DATA_THICKNESS_LEVEL,       uint8_t,        3                                                                 )\
                                                                                                                       \
/*  [Roti making screen] Oil level (1 -> 2) */                                                                         \
X(  GUI_DATA_OIL_LEVEL,             uint8_t,        1                                                                 )\
                                                                                                                       \
/*  [Roti making screen] Indicates if cooking has been started by user (1) or not (0) */                               \
X(  GUI_DATA_COOKING_STARTED,       uint8_t,        0                                                                 )\
                                                                                                                       \
/*  [Roti making screen] Instantaneous cooking state: 0 = idle, 1 = cooking */                                         \
X(  GUI_DATA_COOKING_STATE,         uint8_t,        0                                                                 )\
                                                                                                                       \
/*  [Splash screen] Brief information about Python cooking script (max. 32 chars) */                                   \
X(  GUI_DATA_SCRIPT_BRIEF_INFO,     string,         "Loading..."                                                      )\
                                                                                                                       \
/*  [Menu screen] Detailed information about Python cooking script (max. 128 chars) */                                 \
X(  GUI_DATA_SCRIPT_DETAIL_INFO,    string,         "+ Script not loaded"                                             )\
                                                                                                                       \
/*  [Developer screen] Debug information (max. 96 chars) */                                                            \
X(  GUI_DATA_DEBUG_INFO,            string,         ""                                                                )\
                                                                                                                       \
/*  [Developer screen] Display a picture on the LCD (max. 96 chars) */                                                 \
/*  + "/dev/cam": the picture is taken from the camera  */                                                             \
/*  + "/dev/framebuf/0xAAAAAAAA": the picture is taken from a RAM buffer which has address 0xAAAAAAAA (0x%08X) */      \
/*  + Otherwise: path in filesystem of the picture file (JPG format) */                                                \
X(  GUI_DATA_DEBUG_PICTURE,         string,         "/dev/cam"                                                        )\
                                                                                                                       \
/*-------------------------------------------------------------------------------------------------------------------*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#endif /* __APP_GUI_MNGR_EXT_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
