/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_micropy.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 May 7
**  @brief      : Public header of Srvc_Micropy module
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @{
*/

#ifndef __SRVC_MICROPY_H__
#define __SRVC_MICROPY_H__

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

/** @brief  Status returned by APIs of Srvc_Micropy module */
enum
{
    MP_OK                   = 0,        //!< The function executed successfully
    MP_ERR                  = -1,       //!< There is unknown error while executing the function
    MP_ERR_NOT_YET_INIT     = -2,       //!< The given instance is not initialized yet
    MP_ERR_BUSY             = -3,       //!< The function failed because the given instance is busy
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Initializes Srvc_Micropy module */
extern int8_t s8_MP_Init (void);

/* Executes a Python script */
extern int8_t s8_MP_Execute_File (const char * pstri_file_path);

/* Sends a message from C environment to MicroPython environment */
extern int8_t s8_MP_Que_Send_To_MP (const void * pv_msg, uint16_t u16_len);

/* Waits and receives a message sent from MicroPython environment */
extern int8_t s8_MP_Que_Receive_From_MP (void * pv_msg, uint16_t * pu16_len);

/* Runs WebREPL mode. WebREPL mode can be stopped by pressing Ctrl+D in WebREPL console */
extern int8_t s8_MP_Run_WebRepl (void);

#endif /* __SRVC_MICROPY_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
