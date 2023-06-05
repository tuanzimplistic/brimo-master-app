/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_io_tca9534_ext.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 25
**  @brief      : Extended header of Srvc_IO_TCA9534 module. This header is public to other modules for use.
**  @namespace  : GPIOX
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_IO_TCA9534
** @{
*/

#ifndef __SRVC_IO_TCA9534_EXT_H__
#define __SRVC_IO_TCA9534_EXT_H__

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
** @brief   This table configures instances of expanded GPIO (GPIOX).
** @details
**
** Each GPIOX instance encapsulates an IO pin of one TCA9534 chip. Each has the following properties:
**
** - GPIOX_Inst_ID      : Alias of the GPIOX instance
**
** - Port Number        : Port number (0 -> 7) of the GPIOX pin on TCA9534.
**                        For example, port number of pin "P2" is "2"
**
** - Direction          : Initial direction of the GPIOX pin, direction could be changed during run-time
**      + INPUT         : Input pin
**      + OUTPUT        : Output pin
**
** - Active Level       : Specifies the logic level at which the component connected the GPIOX pin is "active".
**                        For example, if the GPIOX pin is driving a LED, active level is the level makes the LED
**                        to turn on; if the GPIOX pin is connected a button, active level is the level when the
**                        button is pressed.
**      + 0             : The GPIOX level is "0" when active and "1" when not active
**      + 1             : The GPIOX level is "1" when active and "0" when not active
**
*/
#define GPIOX_INST_TABLE(X)                                                     \
                                                                                \
/*----------------------------------------------------------------------------*/\
/*  GPIOX_Inst_ID               Port_Number     Direction       Active_Level  */\
/*                                                                            */\
/*----------------------------------------------------------------------------*/\
                                                                                \
/*  Sense door state (active when door is closed)                             */\
X(  GPIOX_DOOR_SENSE,           0,              INPUT,          0              )\
                                                                                \
/*  Reset LCD touch screen (active to reset)                                  */\
X(  GPIOX_TOUCH_RST,            1,              OUTPUT,         0              )\
                                                                                \
/*  Reset LCD (active to reset)                                               */\
X(  GPIOX_LCD_RST,              2,              OUTPUT,         0              )\
                                                                                \
/*  Toggle camera's power supply (active to enable power down mode)           */\
X(  GPIOX_CSI_PWDN,             3,              OUTPUT,         1              )\
                                                                                \
/*  Enable SPI communication with LCD (active to enable communication)        */\
X(  GPIOX_LCD_CS,               4,              OUTPUT,         0              )\
                                                                                \
/*  Toggle LCD backlight (active to turn on the backlight)                    */\
X(  GPIOX_LCD_BL,               5,              OUTPUT,         1              )\
                                                                                \
/*  Toggling power supply of LCD, touch, and camera (active to enable power)  */\
X(  GPIOX_LCD_CAM_PWR,          6,              OUTPUT,         1              )\
                                                                                \
/*  Reset camera (active to reset)                                            */\
X(  GPIOX_CAMERA_RST,           7,              OUTPUT,         0              )\
                                                                                \
/*----------------------------------------------------------------------------*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


#endif /* __SRVC_IO_TCA9534_EXT_H__ */

/**
** @}
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
