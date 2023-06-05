/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : hwa_i2c_master_ext.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 23
**  @brief      : Extended header of Hwa_I2C_Master module. This header is public to other modules for use.
**  @namespace  : I2C
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Hwa_I2C_Master
** @{
*/

#ifndef __HWA_I2C_MASTER_EXT_H__
#define __HWA_I2C_MASTER_EXT_H__

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
** @brief   This table configures instances of I2C Masters
** @details
**
** Each I2C Master instance encapsulates the communication with an I2C slave component and has the following properties:
**
** - I2C_Inst_ID            : Alias of the I2C Master instance
**
** - I2C_Port_Number        : I2C port that the Master uses. This is either 0 or 1
**
** - Slave address          : I2C address (7-bit, not including Read/Write bit) of the slave device that the Master
**                            communicates with
*/
#define I2C_INST_TABLE(X)                                                        \
                                                                                 \
/*-----------------------------------------------------------------------------*/\
/*  I2C_Inst_ID             I2C_Port_Number     Slave_address                  */\
/*-----------------------------------------------------------------------------*/\
                                                                                 \
/*  IO expander TCA9534 (for TCA9534A of EB1.1 Master, slave address is 0x3F)  */\
X(  I2C_TCA9534,            0,                  0x27                            )\
                                                                                 \
/*  Touch screen GT911                                                         */\
X(  I2C_GT911,              0,                  0x5D                            )\
                                                                                 \
/*  Camera module (slave address will be changed by the camera driver)         */\
X(  I2C_CAMERA,             0,                  0x00                            )\
                                                                                 \
/*-----------------------------------------------------------------------------*/


/**
** @brief   This table configures I2C ports
** @details
**
** ESP32 has 2 I2C ports (ie. I2C controller), each port has the following configuration:
**
** - I2C_Port_Number        : Number of I2C port which is either 0 or 1
**
** - SDA pin number         : GPIO number for SDA signal (0 -> 39)
**
** - SDA internal pull-up   : Specifies if internal pull-up resistor (45 Kohm) is used for SDA pin or not.
**                            Note that pins 34-39 don't have internal pull resistors
**      + ENABLE            : Enable internal pull-up resistor
**      + DISABLE           : Disable internal pull-up resistor
**
** - SCL pin number         : GPIO number for SCL signal (0 -> 39)
**
** - SCL internal pull-up   : Specifies if internal pull-up resistor (45 Kohm) is used for SCL or not
**                            Note that pins 34-39 don't have internal pull resistors
**      + ENABLE            : Enable internal pull-up resistor
**      + DISABLE           : Disable internal pull-up resistor
**
** - Clock speed KHz        : I2C master clock frequency in KHz, not higher than 1000 KHz.
**                            Recommended values are 100 and 400
**
*/
#define I2C_PORT_TABLE(X)                                                        \
                                                                                 \
/*-----------------------------------------------------------------------------*/\
/*  I2C_Port_Number         Configuration                                      */\
/*-----------------------------------------------------------------------------*/\
                                                                                 \
X(  0,                      /* SDA pin number       */      26                  ,\
                            /* SDA internal pull-up */      DISABLE             ,\
                            /* SCL pin number       */      27                  ,\
                            /* SCL internal pull-up */      DISABLE             ,\
                            /* Clock speed KHz      */      100                 )\
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


#endif /* __HWA_I2C_MASTER_EXT_H__ */

/**
** @}
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
