/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : hwa_i2c_master.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 23
**  @brief      : Public header of Hwa_I2C_Master module
**  @namespace  : I2C
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Hwa_I2C_Master
** @{
*/

#ifndef __HWA_I2C_MASTER_H__
#define __HWA_I2C_MASTER_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"             /* Use common definitions */
#include "hwa_i2c_master_ext.h"     /* Table of I2C Master instances */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Handle to manage an I2C Master instance */
typedef struct I2C_obj *        I2C_inst_t;

/** @brief  Status returned by APIs of Hwa_I2C_Master module */
enum
{
    I2C_OK                      = 0,        //!< The function executed successfully
    I2C_ERR                     = -1,       //!< There is unknown error while executing the function
    I2C_ERR_NOT_YET_INIT        = -2,       //!< The given instance is not initialized yet
    I2C_ERR_BUSY                = -3,       //!< The function failed because the given instance is busy
};

/** @brief  Expand an entry in I2C_INST_TABLE as enumeration of instance ID */
#define I2C_INST_TABLE_EXPAND_AS_INST_ID(INST_ID, ...)          INST_ID,
typedef enum
{
    I2C_INST_TABLE (I2C_INST_TABLE_EXPAND_AS_INST_ID)
    I2C_NUM_INST
} I2C_inst_id_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets instance of an I2C Master. This instance will be used for the other functions in this module */
extern int8_t s8_I2C_Get_Inst (I2C_inst_id_t enm_inst_id, I2C_inst_t * px_inst);

/*
** Writes a block of data to the corresponding slave
** To check if the slave respond to a write command, pu8_data can be NULL (or u16_len can be zero)
*/
extern int8_t s8_I2C_Write (I2C_inst_t x_inst, const uint8_t * pu8_data, uint16_t u16_len);

/* Writes a block of data to the corresponding slave in memory access mode */
extern int8_t s8_I2C_Write_Mem (I2C_inst_t x_inst, const uint8_t * pu8_mem_addr, uint16_t u16_addr_len,
                                                   const uint8_t * pu8_data, uint16_t u16_data_len);

/*
** Reads a block of data from the corresponding slave
** To check if the slave respond to a read command, pu8_data can be NULL (or u16_len can be zero)
*/
extern int8_t s8_I2C_Read (I2C_inst_t x_inst, uint8_t * pu8_data, uint16_t u16_len);

/* Reads a block of data from the corresponding slave in memory access mode */
extern int8_t s8_I2C_Read_Mem (I2C_inst_t x_inst, const uint8_t * pu8_mem_addr, uint16_t u16_addr_len,
                                                  uint8_t * pu8_data, uint16_t u16_data_len);

/* Changes I2C address of the corresponding slave */
extern int8_t s8_I2C_Set_Slave_Addr (I2C_inst_t x_inst, uint16_t u16_slave_addr);

#endif /* __HWA_I2C_MASTER_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
