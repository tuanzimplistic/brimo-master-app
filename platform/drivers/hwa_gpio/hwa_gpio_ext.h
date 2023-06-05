/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : hwa_gpio_ext.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 22
**  @brief      : Extended header of Hwa_GPIO module. This header is public to other modules for use.
**  @namespace  : GPIO
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Hwa_GPIO
** @{
*/

#ifndef __HWA_GPIO_EXT_H__
#define __HWA_GPIO_EXT_H__

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
** @brief   This table configures instances of GPIO pins
** @details
**
** Each GPIO instance encapsulates a GPIO pin and has the following properties:
**
** - GPIO_Inst_ID           : Alias of the GPIO instance
**
** - GPIO Number            : GPIO number (0 -> 39). For example, GPIO number of "IO14" is "14"
**
** - Direction              : Initial direction of the GPIO pin, direction could be changed during run-time
**      + INPUT             : Input pin
**      + OUTPUT            : Output pin
**
** - Active Level           : Specifies the logic level at which the component connected the GPIO pin is "active".
**                            For example, if the GPIO pin is driving a LED, active level is the level makes the LED
**                            to turn on; if the GPIO pin is connected a button, active level is the level when the
**                            button is pressed.
**      + 0                 : The GPIO level is "0" when active and "1" when not active
**      + 1                 : The GPIO level is "1" when active and "0" when not active
**
** - Pull Mode              : Internal pull resistor (45 Kohm). Note that GPIOs 34-39 don't have internal pull resistors
**      + PULLUP_ONLY       : Pad pull up
**      + PULLDOWN_ONLY     : Pad pull down
**      + PULLUP_PULLDOWN   : Pad pull up + pull down
**      + FLOATING          : Pad floating
**
** - Is Open Drain          : This option is applied for output pin only
**      + true              : The output pin is in open drain mode
**      + false             : The output pin is in push-pull mode
**
** - Drive strength         : Pad drive capacity, for output pins only
**      + 0                 : Weak
**      + 1                 : Stronger
**      + 2                 : Medium
**      + 3                 : Strongest
**      + DEFAULT           : Default drive capacity (2)
**
*/
#define GPIO_INST_TABLE(X)                                                       \
                                                                                 \
/*-----------------------------------------------------------------------------*/\
/*  GPIO_Inst_ID             Configuration                                     */\
/*-----------------------------------------------------------------------------*/\
                                                                                 \
/*  IO expander (TCA9534) interrupt (active when interrupt is triggered)       */\
X(  GPIO_TCA9534_INT,       /* GPIO Number      */      25                      ,\
                            /* Direction        */      INPUT                   ,\
                            /* Active Level     */      0                       ,\
                            /* Pull Mode        */      FLOATING                ,\
                            /* Is Open Drain    */      false                   ,\
                            /* Drive strength   */      DEFAULT                 )\
                                                                                 \
/*  Touch screen interrupt (active when interrupt is triggered)                */\
X(  GPIO_TOUCH_INT,         /* GPIO Number      */      15                      ,\
                            /* Direction        */      INPUT                   ,\
                            /* Active Level     */      0                       ,\
                            /* Pull Mode        */      FLOATING                ,\
                            /* Is Open Drain    */      false                   ,\
                            /* Drive strength   */      DEFAULT                 )\
                                                                                 \
/*  ST7796S LCD Data/Command selector (D/C)                                    */\
X(  GPIO_ST7796S_DC,        /* GPIO Number      */      2                       ,\
                            /* Direction        */      OUTPUT                  ,\
                            /* Active Level     */      1                       ,\
                            /* Pull Mode        */      FLOATING                ,\
                            /* Is Open Drain    */      false                   ,\
                            /* Drive strength   */      DEFAULT                 )\
                                                                                 \
/*  Buzzer control (active to turn on buzzer)                                  */\
X(  GPIO_BUZZER,            /* GPIO Number      */      13                      ,\
                            /* Direction        */      OUTPUT                  ,\
                            /* Active Level     */      1                       ,\
                            /* Pull Mode        */      FLOATING                ,\
                            /* Is Open Drain    */      false                   ,\
                            /* Drive strength   */      DEFAULT                 )\
                                                                                 \
/*  CSI_VSYNC pin of camera module                                             */\
X(  GPIO_CSI_VSYNC,         /* GPIO Number      */      33                      ,\
                            /* Direction        */      INPUT                   ,\
                            /* Active Level     */      0                       ,\
                            /* Pull Mode        */      FLOATING                ,\
                            /* Is Open Drain    */      false                   ,\
                            /* Drive strength   */      DEFAULT                 )\
                                                                                 \
/*-----------------------------------------------------------------------------*/

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


#endif /* __HWA_GPIO_EXT_H__ */

/**
** @}
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
