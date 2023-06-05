/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : hwa_gpio.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 22
**  @brief      : Public header of Hwa_GPIO module
**  @namespace  : GPIO
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Hwa_GPIO
** @{
*/

#ifndef __HWA_GPIO_H__
#define __HWA_GPIO_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"         /* Use common definitions */
#include "hwa_gpio_ext.h"       /* Table of GPIO instances */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Handle to manage a GPIO instance */
typedef struct GPIO_obj *       GPIO_inst_t;

/** @brief  Status returned by APIs of Hwa_GPIO module */
enum
{
    GPIO_OK                     = 0,        //!< The function executed successfully
    GPIO_ERR                    = -1,       //!< There is unknown error while executing the function
    GPIO_ERR_NOT_YET_INIT       = -2,       //!< The given instance is not initialized yet
    GPIO_ERR_BUSY               = -3,       //!< The function failed because the given instance is busy
};

/** @brief  Expand an entry in GPIO_INST_TABLE as enumeration of instance ID */
#define GPIO_INST_TABLE_EXPAND_AS_INST_ID(INST_ID, ...)         INST_ID,
typedef enum
{
    GPIO_INST_TABLE (GPIO_INST_TABLE_EXPAND_AS_INST_ID)
    GPIO_NUM_INST
} GPIO_inst_id_t;

/** @brief  GPIO pin direction */
typedef enum
{
    GPIO_DIR_INPUT,                         //!< Input GPIO
    GPIO_DIR_OUTPUT,                        //!< Output GPIO
    GPIO_NUM_DIRS
} GPIO_dir_t;

/** @brief  GPIO external interrupt mode */
typedef enum
{
    GPIO_INT_RISING_EDGE,                   //!< Interrupt on detection of rising edge
    GPIO_INT_FALLING_EDGE,                  //!< Interrupt on detection of falling edge
    GPIO_INT_BOTH_EDGE,                     //!< Interrupt on detection of either falling edge of raising edge
    GPIO_NUM_INT_MODES
} GPIO_int_mode_t;

/** @brief  Context data of the events fired by the module */
typedef struct
{
    GPIO_inst_t     x_inst;                 //!< The instance that fires the event
    void *          pv_arg;                 //!< Argument passed when the associated callback function was registered

    /** @brief  Events fired by the module */
    enum
    {
        GPIO_EVT_EDGE_DETECTED,             //!< An edge is detected at the GPIO pin
    } enm_evt;

} GPIO_evt_data_t;

/**
** @brief   Callback invoked when there is an edge at an input GPIO
** @note    The callback function will be invoked in GPIO interrupt context
*/
typedef void (*GPIO_callback_t) (GPIO_evt_data_t * pstru_evt_data);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets instance of a GPIO. This instance will be used for the other functions in this module */
extern int8_t s8_GPIO_Get_Inst (GPIO_inst_id_t enm_inst_id, GPIO_inst_t * px_inst);

/* Changes direction of a GPIO */
extern int8_t s8_GPIO_Change_Dir (GPIO_inst_t x_inst, GPIO_dir_t enm_dir);

/* Changes active level of a GPIO pin */
extern int8_t s8_GPIO_Change_Active_Level (GPIO_inst_t x_inst, uint8_t u8_active_level);

/* Writes an output pin to either level 0 or 1 */
extern int8_t s8_GPIO_Write_Level (GPIO_inst_t x_inst, uint8_t u8_level);

/*
** Writes an output pin to its active level
** Active level may be either 0 or 1, depending on which level is considered active.
*/
extern int8_t s8_GPIO_Write_Active (GPIO_inst_t x_inst, bool b_active);

/* Inverts level of an output pin, i.e if current output level is 0 it shall be 1 and vice versa */
extern int8_t s8_GPIO_Write_Inverted (GPIO_inst_t x_inst);

/* Gets current level of an input or output pin */
extern int8_t s8_GPIO_Read_Level (GPIO_inst_t x_inst, uint8_t * pu8_level);

/*
** Checks if an input or output pin is at its active level or not.
** Active level may be either logic 0 or logic 1, depending on which logic is considered active.
*/
extern int8_t s8_GPIO_Read_Active (GPIO_inst_t x_inst, bool * pb_active);

/* Enables external interrupt of an input pin */
extern int8_t s8_GPIO_Enable_Interrupt (GPIO_inst_t x_inst, GPIO_int_mode_t enm_mode,
                                        GPIO_callback_t pfnc_cb, void * pv_arg);

/* Disables external interrupt of an input pin */
extern int8_t s8_GPIO_Disable_Interrupt (GPIO_inst_t x_inst);

#endif /* __HWA_GPIO_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
