/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_master_datalink.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Dec 13
**  @brief      : Public header of Srvc_Master_Datalink module
**  @namespace  : MDL
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Master_Datalink
** @{
*/

#ifndef __SRVC_MASTER_DATALINK_H__
#define __SRVC_MASTER_DATALINK_H__

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

/** @brief  Handle to manage a Master data-link channel */
typedef struct MDL_obj *                MDL_inst_t;

/** @brief  Status returned by APIs of Srvc_Master_Datalink module */
enum
{
    MDL_OK                  = 0,        //!< The function executed successfully
    MDL_ERR                 = -1,       //!< There is unknown error while executing the function
    MDL_ERR_BUSY            = -2,       //!< The function failed because the given instance is busy
};

/** @brief  Constant used for s8_MDL_Receive_Raw() and s8_MDL_Transceive_Raw() in case of waiting forever */
#define MDL_WAIT_FOREVER                0xFFFF

/** @brief  Events fired by Srvc_Master_Datalink module */
typedef enum
{
    MDL_EVT_MSG_RECEIVED,               //!< A message has been received, context is data-link payload data

} MDL_evt_t;

/** @brief  Callback invoked when an event occurs */
typedef void (*MDL_cb_t) (MDL_inst_t x_inst, MDL_evt_t enm_evt, const void * pv_data, uint16_t u16_len);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets instance of Master data-link channel */
extern int8_t s8_MDL_Get_Inst (MDL_inst_t * px_inst);

/* Runs Master data-link channel */
extern int8_t s8_MDL_Run_Inst (MDL_inst_t x_inst);

/* Registers callack function to a Master data-link channel */
extern int8_t s8_MDL_Register_Cb (MDL_inst_t x_inst, MDL_cb_t pfnc_cb);

/* Sends data to a Master data-link channel */
extern int8_t s8_MDL_Send (MDL_inst_t x_inst, const void * pv_data, uint16_t u16_len);

/* Enables or disables raw mode of a channel */
extern int8_t s8_MDL_Toggle_Raw_Mode (MDL_inst_t x_inst, bool b_enabled);

/* Sends data over a channel in raw mode */
extern int8_t s8_MDL_Send_Raw (MDL_inst_t x_inst, const void * pv_data, uint16_t u16_len);

/* Receives data from a channel in raw mode */
extern int8_t s8_MDL_Receive_Raw (MDL_inst_t x_inst, void * pv_data, uint16_t * pu16_len, uint16_t u16_timeout);

/* Sends data over a channel in raw mode and waits for incoming data */
extern int8_t s8_MDL_Transceive_Raw (MDL_inst_t x_inst, const void * pv_tx_data, uint16_t u16_tx_len,
                                     void * pv_rx_data, uint16_t * pu16_rx_len, uint16_t u16_rx_timeout);

#endif /* __SRVC_MASTER_DATALINK_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
