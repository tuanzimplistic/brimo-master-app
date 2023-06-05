/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_lcd_st7796s_demo.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 30
**  @brief      : Public header of Srvc_Lcd_ST7796s_Demo module
**  @namespace  : ST7796S
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Lcd_ST7796s_Demo
** @{
*/

#ifndef __SRVC_LCD_ST7796S_DEMO_H__
#define __SRVC_LCD_ST7796S_DEMO_H__

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

/** @brief  Handle to manage a ST7796S controller object */
typedef struct ST7796S_obj *        ST7796S_inst_t;

/** @brief  Status returned by APIs of Srvc_Lcd_ST7796s_Demo module */
enum
{
    ST7796S_OK                      = 0,        //!< The function executed successfully
    ST7796S_ERR                     = -1,       //!< There is unknown error while executing the function
    ST7796S_ERR_NOT_YET_INIT        = -2,       //!< The given instance is not initialized yet
    ST7796S_ERR_BUSY                = -3,       //!< The function failed because the given instance is busy
};

/** @brief  Structure of a pixel (RGB565) */
typedef uint16_t                    ST7796S_pixel_t;

/**
** @brief
**      Macro converting RGB888 to swapped RGB565 (ST7796S_pixel_t)
**
** @details
**      Structure of a swapped RGB565
**           15       13 12         8 7         3 2          0
**          +-----------+------------+-----------+------------+
**          | Green_low |    Blue    |    Red    | Green_high |
**          +-----------+------------+-----------+------------+
**            (3 bits)     (5 bits)    (5 bits)     (3 bits)
**
** @param [in]
**      u8_red: red in RGB888
**
** @param [in]
**      u8_green: green in RGB888
**
** @param [in]
**      u8_blue: blue in RGB888
**
** @return
**      Swapped RGB565 in ST7796S_pixel_t type
*/
#define ST7796S_rgb888to565(u8_red, u8_green, u8_blue)      \
    (ST7796S_pixel_t)(((((u8_green) >> 2) >> 3) & 0x07) |   \
                      ((((u8_red) >> 3) & 0x1F) << 3)   |   \
                      ((((u8_blue) >> 3) & 0x1F) << 8)  |   \
                      ((((u8_green) >> 2) & 0x07) << 13))

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets the single instance of ST7796S controller */
extern int8_t s8_ST7796S_Get_Inst (ST7796S_inst_t * px_inst);

/*
** Writes a buffer of pixels to ST7796S controller from point (xs, ys) to point (xe, ye)
** Note
**      pstru_buffer MUST be in DMA capable memory
*/
extern int8_t s8_ST7796S_Write_Pixels (ST7796S_inst_t x_inst, uint16_t u16_xs, uint16_t u16_ys,
                                       uint16_t u16_xe, uint16_t u16_ye, const ST7796S_pixel_t * pstru_buffer);

/* Toggles LCD's LED backlight */
extern int8_t s8_ST7796S_Toggle_Backlight (ST7796S_inst_t x_inst, bool b_on);

#endif /* __SRVC_LCD_ST7796S_DEMO_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
