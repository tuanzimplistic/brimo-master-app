/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_master_datalink.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Dec 16
**  @brief      : Public header of Srvc_Master_Transport module
**  @namespace  : MTP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Master_Transport
** @{
*/

#ifndef __SRVC_MASTER_TRANSPORT_H__
#define __SRVC_MASTER_TRANSPORT_H__

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

/** @brief  Handle to manage a Master transport channel */
typedef struct MTP_obj *            MTP_inst_t;

/** @brief  Status returned by APIs of Srvc_Master_Transport module */
enum
{
    MTP_OK              = 0,        //!< The function executed successfully
    MTP_ERR             = -1,       //!< There is unknown error while executing the function
    MTP_ERR_BUSY        = -2,       //!< The function failed because the given instance is busy
};

/** @brief  Events fired by Srvc_Master_Transport module */
typedef enum
{
    MTP_EVT_NOTIFY,                 //!< A notification message has been received, context is transport payload data

} MTP_evt_t;

/** @brief  Callback invoked when an event occurs */
typedef void (*MTP_cb_t) (MTP_inst_t x_inst, MTP_evt_t enm_evt, const void * pv_data, uint16_t u16_len);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets instance of Master transport channel */
extern int8_t s8_MTP_Get_Inst (MTP_inst_t * px_inst);

/* Runs Master transport channel */
extern int8_t s8_MTP_Run_Inst (MTP_inst_t x_inst);

/* Registers callack function to a Master transport channel */
extern int8_t s8_MTP_Register_Cb (MTP_inst_t x_inst, MTP_cb_t pfnc_cb);

/* Sends request message to a Master transport channel and waits for a response message */
extern int8_t s8_MTP_Send_Request (MTP_inst_t x_inst, const void * pv_request, uint16_t u16_request_len,
                                   uint8_t ** ppu8_response, uint16_t * pu16_response_len, uint16_t u16_timeout);

/* Sends post message to a Master transport channel */
extern int8_t s8_MTP_Send_Post (MTP_inst_t x_inst, const void * pv_post, uint16_t u16_post_len);

#endif /* __SRVC_MASTER_TRANSPORT_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
