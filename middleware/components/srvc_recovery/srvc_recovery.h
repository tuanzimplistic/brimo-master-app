/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_recovery.h
**  @author     : Nguyen Ngoc Tung
**  @date       : 2022 Dec 29
**  @brief      : Public header of Srvc_Recovery module
**  @namespace  : RCVR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Recovery
** @brief       Provides a cache in volatile memory for cooking script to store its state data. Upon power interruption,
**              that data will be stored onto non-volatile storage and restored on the next power-on. The cooking
**              script, therefore, can resume its operation from a power interruption.
** @details
**              During normal operation, the cooking script via binding function regularly calls enm_RCVR_Set_Data()
**              function to store its internal state onto a reseved cache in RAM.
**
**              The detection of power interruption is performed by Slave Board. When that is detected, Slave Board will
**              send a realtime message to Master Board to inform about the event. Srvc_Rt_Log mode of Master Board
**              will capture the message and invoke v_RCVR_Backup_Data() function of this module to start the backing
**              up data from RAM into flash.
**
**              On the next power-on, the cooking script can call u16_RCVR_Get_Data_Pointer() to get the backup data
**              and resume the interrupted operation if needed.
** @{
*/

#ifndef __SRVC_RECOVERY_H__
#define __SRVC_RECOVERY_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"             /* Common definitions */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Status returned by APIs of Srvc_Recovery module */
typedef enum
{
    RCVR_OK             = 0,        //!< The function executed successfully
    RCVR_ERR            = -1,       //!< There is unknown error while executing the function
} RCVR_status_t;

/** @brief  Minimum size in bytes of the backup data (this setting must be > 1) */
#define RCVR_MIN_DATA_LEN           2

/** @brief  Maximum size in bytes of the backup data */
#define RCVR_MAX_DATA_LEN           128

/**
** @brief   Callback invoked when backup process is triggered
** @note    The callback is invoked in the context of Srvc_Recovery task
*/
typedef void (*RCVR_callback_t)(void * pv_arg);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** Initializes Srvc_Recovery module.
** This reads the data (if available) from non-volatile memory back onto volatile memory
*/
extern RCVR_status_t enm_RCVR_Init (void);

/*
** Stores a block of data onto cache in volatile memory
** NOTE: Length in bytes of the data must be from (including) RCVR_MIN_DATA_LEN to RCVR_MAX_DATA_LEN
*/
extern RCVR_status_t enm_RCVR_Set_Data (const void * pv_data, uint16_t u16_len);

/* Gets pointer to the data in cache */
extern uint16_t u16_RCVR_Get_Data_Pointer (uint8_t ** ppu8_value);

/* Registers a callback function which will be invoked when backup process is triggered */
extern RCVR_status_t enm_RCVR_Register_Cb (RCVR_callback_t pfnc_cb, void * pv_arg);

/*
** Dumps the data currently stored in cache onto non-volatile memory
** NOTE: This function will invoke callbacks registered with enm_RCVR_Register_Cb() before dumping the data
*/
extern void v_RCVR_Backup_Data (void);

#endif /* __SRVC_RECOVERY_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
