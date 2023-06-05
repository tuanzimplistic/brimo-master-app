/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_ws_server.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Apr 9
**  @brief      : Public header of Srvc_WS_Server module. This file is the only header file to include in order to
**                use the module
**  @namespace  : WSS
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_WS_Server
** @{
*/

#ifndef __SRVC_WS_SERVER_H__
#define __SRVC_WS_SERVER_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"                 /* Use common definitions */
#include "srvc_ws_server_ext.h"         /* Table of Websocket Server instances */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Handle to manage a channel of the Websocket Server */
typedef struct WSS_obj *                WSS_inst_t;

/** @brief  Status returned by APIs of Srvc_WS_Server module */
typedef enum
{
    WSS_OK              = 0,            //!< The function executed successfully
    WSS_ERR             = -1,           //!< There is unknown error while executing the function
} WSS_status_t;

/** @brief  Expand an entry in WSS_INST_TABLE as enumeration of instance ID */
#define WSS_INST_TABLE_EXPAND_AS_INST_ID(INST_ID, ...)         INST_ID,
typedef enum
{
    WSS_INST_TABLE (WSS_INST_TABLE_EXPAND_AS_INST_ID)
    WSS_NUM_INST
} WSS_inst_id_t;

/** @brief  Context data of the events fired by the module */
typedef struct
{
    WSS_inst_t          x_inst;         //!< Instance of the channel that fires the event
    void *              pv_arg;         //!< Argument passed when the associated callback function was registered
    uint8_t             u8_client_id;   //!< Index of the Websocket client that triggered the event

    /** @brief  Events fired by the module */
    enum
    {
        WSS_EVT_CLIENT_CONNECTED,       //!< A Websocket Client has been connected with the Server
        WSS_EVT_CLIENT_DISCONNECTED,    //!< A Websocket Client has been disconnected from the Server
        WSS_EVT_DATA_RECEIVED,          //!< The Server just received data from a Client
    } enm_evt;

    /** @brief  Context data specific for WSS_EVT_DATA_RECEIVED */
    struct
    {
        const uint8_t * pu8_data;       //!< Pointer to the data received
        uint16_t        u16_len;        //!< Length in bytes of the received data
    } stru_receive;

} WSS_evt_data_t;

/** @brief  Callback invoked when an event occurs */
typedef void (*WSS_callback_t) (WSS_evt_data_t * pstru_evt_data);

/** @brief  All clients of one channel. This is used for enm_WSS_Send() */
#define WSS_ALL_CLIENTS                 0xFF

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets instance handle of a Websocket Server's channel. This instance will be used for other functions in this module */
extern WSS_inst_t x_WSS_Get_Inst (WSS_inst_id_t enm_inst_id);

/* Registers a callback function invoked when an event occurs to a Websocket Server's channel */
extern void v_WSS_Register_Callback (WSS_inst_t x_inst, WSS_callback_t pfnc_cb, void * pv_arg);

/*
** Sends data to a Websocket Client of a channel. Use WSS_ALL_CLIENTS for u8_client_id to broadcast to all clients
** Note: This function cannot be called inside the context of event callback handler of this module
*/
extern WSS_status_t enm_WSS_Send (WSS_inst_t x_inst, uint8_t u8_client_id, const void * pv_data, uint16_t u16_len);

#endif /* __SRVC_WS_SERVER_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
