/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : virtual_keyboard.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 June 1
**  @brief      : Public header of virtual keyboard screen
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @{
*/

#ifndef __VIRTUAL_KEYBOARD_H__
#define __VIRTUAL_KEYBOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"                 /* Use common definitions */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Structure wrapping configuration of virtual keyboard */
typedef struct
{
    bool            b_password_mode;        //!< Indicates if the text input is a password
    uint16_t        u16_max_text_len;       //!< Maximum number of characters input, 0 for no limit
    const char *    pstri_init_text;        //!< Initialized input text, NULL to initialize with empty string
    const char *    pstri_brief;            //!< Brief description about the text to input, NULL for empty string
    const char *    pstri_accepted_chars;   //!< Accepted characters, e.g "0123456789.+-", NULL to accept all
    const char *    pstri_placeholder;      //!< Placeholder text when no character is input, NULL for no placeholder

} GUI_virkb_cfg_t;

/** @brief  Macro providing default initialization value of configuration structure */
#define GUI_VIRKB_DEFAULT_CFG               \
{                                           \
    /* b_password_mode      */  false,      \
    /* u16_max_text_len     */  0,          \
    /* pstri_init_text      */  NULL,       \
    /* pstri_brief          */  NULL,       \
    /* pstri_accepted_chars */  NULL,       \
    /* pstri_placeholder    */  NULL,       \
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Configures the virtual keyboard before it's displayed */
extern int8_t s8_GUI_Virkb_Set_Config (const GUI_virkb_cfg_t * pstru_config);

/* Gets the text that has been input */
extern int8_t s8_GUI_Virkb_Get_Text (const char ** ppstri_text);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __VIRTUAL_KEYBOARD_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
