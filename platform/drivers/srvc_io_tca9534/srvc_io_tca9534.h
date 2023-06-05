/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_io_tca9534.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 25
**  @brief      : Public header of Srvc_IO_TCA9534 module
**  @namespace  : GPIOX
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_IO_TCA9534
** @{
*/

#ifndef __SRVC_IO_TCA9534_H__
#define __SRVC_IO_TCA9534_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"             /* Use common definitions */
#include "srvc_io_tca9534_ext.h"    /* Table of GPIOX instances */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Handle to manage a GPIOX instance */
typedef struct GPIOX_obj *          GPIOX_inst_t;

/** @brief  Status returned by APIs of Srvc_IO_TCA9534 module */
enum
{
    GPIOX_OK                        = 0,        //!< The function executed successfully
    GPIOX_ERR                       = -1,       //!< There is unknown error while executing the function
    GPIOX_ERR_NOT_YET_INIT          = -2,       //!< The given instance is not initialized yet
    GPIOX_ERR_BUSY                  = -3,       //!< The function failed because the given instance is busy
};

/** @brief  Expand an entry in GPIOX_INST_TABLE as enumeration of instance ID */
#define GPIOX_INST_TABLE_EXPAND_AS_INST_ID(INST_ID, ...)        INST_ID,
typedef enum
{
    GPIOX_INST_TABLE (GPIOX_INST_TABLE_EXPAND_AS_INST_ID)
    GPIOX_NUM_INST
} GPIOX_inst_id_t;

/** @brief  GPIOX pin direction */
typedef enum
{
    GPIOX_DIR_INPUT,                            //!< Input GPIO
    GPIOX_DIR_OUTPUT,                           //!< Output GPIO
    GPIOX_NUM_DIRS
} GPIOX_dir_t;

/**
** @brief   Callback invoked when value of any GPIOX input pins changes
** @note    The callback function will be invoked in GPIO interrupt context
*/
typedef void (*GPIOX_cb_t) (GPIOX_inst_t x_inst);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets instance of a GPIOX. This instance will be used for the other functions in this module */
extern int8_t s8_GPIOX_Get_Inst (GPIOX_inst_id_t enm_inst_id, GPIOX_inst_t * px_inst);

/* Changes direction of a GPIOX */
extern int8_t s8_GPIOX_Change_Dir (GPIOX_inst_t x_inst, GPIOX_dir_t enm_dir);

/* Changes active level of a GPIOX pin */
extern int8_t s8_GPIOX_Change_Active_Level (GPIOX_inst_t x_inst, uint8_t u8_active_level);

/* Writes a GPIOX output pin to either level 0 or 1 */
extern int8_t s8_GPIOX_Write_Level (GPIOX_inst_t x_inst, uint8_t u8_level);

/*
** Writes a GPIOX output pin to its active level
** Active level may be either 0 or 1, depending on which level is considered active.
*/
extern int8_t s8_GPIOX_Write_Active (GPIOX_inst_t x_inst, bool b_active);

/* Inverts level of a GPIOX output pin, i.e if current output level is 0 it shall be 1 and vice versa */
extern int8_t s8_GPIOX_Write_Inverted (GPIOX_inst_t x_inst);

/* Gets current level of a GPIOX input or output pin */
extern int8_t s8_GPIOX_Read_Level (GPIOX_inst_t x_inst, uint8_t * pu8_level);

/*
** Checks if a GPIOX input or output pin is at its active level or not.
** Active level may be either logic 0 or logic 1, depending on which logic is considered active.
*/
extern int8_t s8_GPIOX_Read_Active (GPIOX_inst_t x_inst, bool * pb_active);

/* Enables interrupt firing when value of any GPIOX input pins changes */
extern int8_t s8_GPIOX_Enable_Interrupt (GPIOX_inst_t x_inst, GPIOX_cb_t pfnc_cb);

/* Disables interrupt firing when value of any GPIOX input pins changes */
extern int8_t s8_GPIOX_Disable_Interrupt (GPIOX_inst_t x_inst);

#endif /* __SRVC_IO_TCA9534_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
