/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_ws_server.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Apr 9
**  @brief      : Implementation of Srvc_WS_Server module
**  @namespace  : WSS
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_WS_Server
**
** @brief       This module provides one Websocket server which has multiple communication channels. Each channel is
**              represented by and accessed via a URI. Multiple Websocket clients can concurrently connect to the same
**              channel.
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_ws_server.h"         /* Public header of this module */
#include "esp_http_server.h"        /* Use ESP-IDF HTTP Server component */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Structure encapsulating connection with a Websocket client */
typedef struct
{
    bool                b_active;               //!< Indicate if this connection is active
    int                 x_socket_fd;            //!< Socket descriptor of the client connection
} WSS_client_t;

/** @brief  Structure encapsulating a Websocket Server's channel object */
struct WSS_obj
{
    bool                b_initialized;          //!< Specifies whether the object has been initialized or not
    WSS_inst_id_t       enm_inst_id;            //!< Instance ID of this object

    WSS_callback_t      pfnc_cb;                //!< Callback functions invoked when data is received
    void *              pv_cb_arg;              //!< Argument passed when the callback function was registered

    const char *        pstri_uri;              //!< URI of the channel
    uint8_t             u8_num_clients;         //!< Maximum number of clients that can connect to the channel at a time
    WSS_client_t *      pstru_clients;          //!< Pointer to the array of Websocket clients
};

/** @brief  Macro expanding WSS_INST_TABLE as initialization value for WSS_obj struct */
#define INST_TABLE_EXPAND_AS_STRUCT_INIT(INST_ID, URI, CLIENTS)     \
{                                                                   \
    .b_initialized      = false,                                    \
    .enm_inst_id        = INST_ID,                                  \
                                                                    \
    .pfnc_cb            = NULL,                                     \
    .pv_cb_arg          = NULL,                                     \
                                                                    \
    .pstri_uri          = URI,                                      \
    .u8_num_clients     = CLIENTS,                                  \
    .pstru_clients      = g_astru_clients_##INST_ID,                \
},

/** @brief  Macro expanding WSS_INST_TABLE as array of Websocket clients */
#define INST_TABLE_EXPAND_AS_CLIENT_ARRAY(INST_ID, URI, CLIENTS)    \
    static WSS_client_t g_astru_clients_##INST_ID [CLIENTS];

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_WS_Server";

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Arrays of all Websocket clients */
WSS_INST_TABLE (INST_TABLE_EXPAND_AS_CLIENT_ARRAY)

/** @brief  Array of all channel objects of the Websocket server */
static struct WSS_obj g_astru_channel_objs[WSS_NUM_INST] =
{
    WSS_INST_TABLE (INST_TABLE_EXPAND_AS_STRUCT_INIT)
};

/** @brief  Handle of the Websocket server */
static httpd_handle_t g_x_server;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static int32_t s32_WSS_Init_Module (void);
static int32_t s32_WSS_Init_Inst (WSS_inst_t x_inst);
static esp_err_t enm_WSS_Channel_Handler (httpd_req_t * stru_request);
static bool b_WSS_Is_Client_Active (WSS_inst_t x_inst, uint8_t u8_client_id);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets instance handle of a Websocket Server's channel. This instance will be used for other functions in this
**      module.
**
** @param [in]
**      enm_inst_id: Index of the channel instance to get. The indexes are expanded from Instance_ID column of
**                   WSS_INST_TABLE (srvc_ws_server_ext.h)
**
** @return
**      @arg    NULL: Failed to get instance of the channel
**      @arg    Otherwise: Instance handle of the channel
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
WSS_inst_t x_WSS_Get_Inst (WSS_inst_id_t enm_inst_id)
{
    WSS_inst_t  x_inst = NULL;
    int32_t     s32_result = STATUS_OK;

    /* Validation */
    if (enm_inst_id >= WSS_NUM_INST)
    {
        return NULL;
    }

    /* If this module has not been initialized, do that now */
    if (!g_b_initialized)
    {
        s32_result = s32_WSS_Init_Module ();
        g_b_initialized = (s32_result == STATUS_OK);
    }

    /* If the retrieved instance has not been initialized yet, do that now */
    if (s32_result == STATUS_OK)
    {
        x_inst = &g_astru_channel_objs[enm_inst_id];
        if (!x_inst->b_initialized)
        {
            s32_result = s32_WSS_Init_Inst (x_inst);
            x_inst->b_initialized = (s32_result == STATUS_OK);
        }
    }

    /* Return instance of the required Websocket Server */
    return (s32_result == STATUS_OK) ? x_inst : NULL;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Registers a callback function invoked when an event occurs to a Websocket Server's channel
**
** @param [in]
**      x_inst: Channel instance returned by x_WSS_Get_Inst() function
**
** @param [in]
**      pfnc_cb: Callback function to register
**
** @param [in]
**      pv_arg: Optional argument which will be forwarded to the data of callback function when it's invoked
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_WSS_Register_Callback (WSS_inst_t x_inst, WSS_callback_t pfnc_cb, void * pv_arg)
{
    /* Validation */
    ASSERT_PARAM ((x_inst != NULL) && x_inst->b_initialized);

    /* Store the callback function */
    x_inst->pfnc_cb = pfnc_cb;
    x_inst->pv_cb_arg = pv_arg;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends data to a Websocket Client
**
** @note
**      This function cannot be called inside the context of event callback handler of this module
**
** @param [in]
**      x_inst: Channel instance returned by x_WSS_Get_Inst() function
**
** @param [in]
**      u8_client_id: Index number of the client to send data to. Use WSS_ALL_CLIENTS to send data to all clients
**                    connected to the given channel
**
** @param [in]
**      pv_data: Pointer to the data to send
**
** @param [in]
**      u16_len: Length in bytes of the data to send
**
** @return
**      @arg    WSS_OK
**      @arg    WSS_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
WSS_status_t enm_WSS_Send (WSS_inst_t x_inst, uint8_t u8_client_id, const void * pv_data, uint16_t u16_len)
{
    /* Validation */
    ASSERT_PARAM ((x_inst != NULL) && (x_inst->b_initialized) && (pv_data != NULL) && (u16_len > 0));

    /* Validate client ID */
    if ((u8_client_id >= x_inst->u8_num_clients) && (u8_client_id != WSS_ALL_CLIENTS))
    {
        LOGE ("Invalid Websocket client index %d", u8_client_id);
        return WSS_ERR;
    }

    /* Construct data to send */
    httpd_ws_frame_t stru_tx_frame =
    {
        .final          = true,
        .fragmented     = false,
        .type           = HTTPD_WS_TYPE_BINARY,
        .len            = u16_len,
        .payload        = (uint8_t *)pv_data,
    };

    /* Sends data to the specified websocket channel(s) synchronously */
    if (u8_client_id < x_inst->u8_num_clients)
    {
        WSS_client_t * pstru_client = &x_inst->pstru_clients[u8_client_id];
        if (!pstru_client->b_active)
        {
            LOGE ("The client index %d is not active", u8_client_id);
            return WSS_ERR;
        }

        esp_err_t x_err = httpd_ws_send_data (g_x_server, pstru_client->x_socket_fd, &stru_tx_frame);
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to send data to client index %d (%s)", u8_client_id, esp_err_to_name (x_err));

            /* This client may be not active any more */
            b_WSS_Is_Client_Active (x_inst, u8_client_id);
            return WSS_ERR;
        }
    }
    else
    {
        /* Broadcast the data to all clients connecting with the channels */
        for (uint8_t u8_idx = 0; u8_idx < x_inst->u8_num_clients; u8_idx++)
        {
            /* Only send data to active client */
            WSS_client_t * pstru_client = &x_inst->pstru_clients[u8_idx];
            if (pstru_client->b_active)
            {
                esp_err_t x_err = httpd_ws_send_data (g_x_server, pstru_client->x_socket_fd, &stru_tx_frame);
                if (x_err != ESP_OK)
                {
                    LOGE ("Failed to send data to client index %d (%s)", u8_idx, esp_err_to_name (x_err));

                    /* This client may be not active any more */
                    b_WSS_Is_Client_Active (x_inst, u8_idx);
                }
            }
        }
    }

    return WSS_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_WS_Server module
**
** @return
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int32_t s32_WSS_Init_Module (void)
{
    /* Max number of sockets/clients connected at any time */
    uint8_t u8_max_clients = 0;
    for (uint8_t u8_idx = 0; u8_idx < WSS_NUM_INST; u8_idx++)
    {
        WSS_inst_t x_inst = &g_astru_channel_objs[u8_idx];
        u8_max_clients += x_inst->u8_num_clients;
    }

    /* Configure and start the Websocket server */
    httpd_config_t stru_config = HTTPD_DEFAULT_CONFIG ();
    stru_config.max_open_sockets = u8_max_clients;
    stru_config.max_uri_handlers = WSS_NUM_INST;

    esp_err_t x_err = httpd_start (&g_x_server, &stru_config);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to start the Websocket server (%s)", esp_err_to_name (x_err));
        return STATUS_ERR;
    }

    return STATUS_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes a Websocket Server's channel instance
**
** @param [in]
**      x_inst: Channel instance returned by x_WSS_Get_Inst() function
**
** @return
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int32_t s32_WSS_Init_Inst (WSS_inst_t x_inst)
{
    /* Register handler processing requests from the channel */
    httpd_uri_t stru_uri_config =
    {
        .uri            = x_inst->pstri_uri,
        .method         = HTTP_GET,
        .handler        = enm_WSS_Channel_Handler,
        .user_ctx       = x_inst,
        .is_websocket   = true
    };
    esp_err_t x_err = httpd_register_uri_handler (g_x_server, &stru_uri_config);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to register handler processing websocket requests (%s)", esp_err_to_name (x_err));
        return STATUS_ERR;
    }

    /* Initialize Websocket client data of the channel */
    for (uint8_t u8_idx = 0; u8_idx < x_inst->u8_num_clients; u8_idx++)
    {
        WSS_client_t * pstru_client = &x_inst->pstru_clients[u8_idx];
        pstru_client->b_active = false;
        pstru_client->x_socket_fd = -1;
    }

    return STATUS_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Callback handler invoked when the server receives a request from client
**
** @param [in]
**      pstru_request: the request received
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static esp_err_t enm_WSS_Channel_Handler (httpd_req_t * pstru_request)
{
    esp_err_t x_err = ESP_OK;

    /* The channel instance associated with the request */
    WSS_inst_t x_inst = (WSS_inst_t)pstru_request->user_ctx;
    int x_sock_fd = httpd_req_to_sockfd (pstru_request);

    /* If method is HTTP_GET, handshake for the channel is done */
    if (pstru_request->method == HTTP_GET)
    {
        LOGI ("Handshake for URI \"%s\" done, the new connection was opened", pstru_request->uri);

        /* Add this client connection to the channel */
        bool b_client_added = false;
        for (uint8_t u8_client_id = 0; u8_client_id < x_inst->u8_num_clients; u8_client_id++)
        {
            WSS_client_t * pstru_client = &x_inst->pstru_clients[u8_client_id];
            if (b_WSS_Is_Client_Active (x_inst, u8_client_id) == false)
            {
                pstru_client->b_active = true;
                pstru_client->x_socket_fd = x_sock_fd;
                b_client_added = true;

                /* Invoke callback */
                if (x_inst->pfnc_cb != NULL)
                {
                    WSS_evt_data_t stru_evt_data =
                    {
                        .x_inst             = x_inst,
                        .pv_arg             = x_inst->pv_cb_arg,
                        .u8_client_id       = u8_client_id,
                        .enm_evt            = WSS_EVT_CLIENT_CONNECTED,
                    };
                    x_inst->pfnc_cb (&stru_evt_data);
                }
                break;
            }
        }

        /* If there is no available room to add the client */
        if (!b_client_added)
        {
            LOGE ("Number of clients exceeds the maximum number. Closing the new connection");
            httpd_sess_trigger_close (g_x_server, x_sock_fd);
            return ESP_ERR_NO_MEM;
        }
    }

    /* Otherwise, there is incoming data from the channel */
    else
    {
        httpd_ws_frame_t stru_rx_frame =
        {
            .payload = NULL,
            .len = 0,
        };

        /* Get data length of the received frame */
        x_err = httpd_ws_recv_frame (pstru_request, &stru_rx_frame, 0);
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to get data length of the received frame (%s)", esp_err_to_name (x_err));
            return x_err;
        }

        /* Get data of the received frame and invoke callback */
        if (stru_rx_frame.len)
        {
            /* Allocate buffer for the receive data */
            stru_rx_frame.payload = malloc (stru_rx_frame.len);
            if (stru_rx_frame.payload == NULL)
            {
                LOGE ("Failed to allocate memory (%d bytes) for receive data", stru_rx_frame.len);
                return ESP_ERR_NO_MEM;
            }

            /* Get the frame payload */
            x_err = httpd_ws_recv_frame (pstru_request, &stru_rx_frame, stru_rx_frame.len);
            if (x_err != ESP_OK)
            {
                LOGE ("Failed to get data of the received frame (%s)", esp_err_to_name (x_err));
                free (stru_rx_frame.payload);
                return x_err;
            }

            /* Determine which client the frame is received from */
            uint8_t u8_client_id;
            for (u8_client_id = 0; u8_client_id < x_inst->u8_num_clients; u8_client_id++)
            {
                WSS_client_t * pstru_client = &x_inst->pstru_clients[u8_client_id];
                if (pstru_client->b_active && (pstru_client->x_socket_fd == x_sock_fd))
                {
                    break;
                }
            }
            if (u8_client_id == x_inst->u8_num_clients)
            {
                LOGE ("There is no client corresponding with the received data");
                return ESP_ERR_NOT_FOUND;
            }

            /* Invoke callback */
            if (x_inst->pfnc_cb != NULL)
            {
                WSS_evt_data_t stru_evt_data =
                {
                    .x_inst             = x_inst,
                    .pv_arg             = x_inst->pv_cb_arg,
                    .u8_client_id       = u8_client_id,
                    .enm_evt            = WSS_EVT_DATA_RECEIVED,
                    .stru_receive       =
                    {
                        .pu8_data       = stru_rx_frame.payload,
                        .u16_len        = stru_rx_frame.len,
                    },
                };
                x_inst->pfnc_cb (&stru_evt_data);
                free (stru_rx_frame.payload);
            }
        }
    }

    return x_err;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Checks if a client is active. If an active client becomes inactive, invoke the relevant event callback
**
** @param [in]
**      x_inst: Channel instance returned by x_WSS_Get_Inst() function
**
** @param [in]
**      u8_client_id: Index of the client to check
**
** @return
**      @arg    true: the client is still active
**      @arg    true: the client is not active
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static bool b_WSS_Is_Client_Active (WSS_inst_t x_inst, uint8_t u8_client_id)
{
    WSS_client_t * pstru_client = &x_inst->pstru_clients[u8_client_id];

    /* Validation */
    ASSERT_PARAM (u8_client_id < x_inst->u8_num_clients);

    /* Only check active client */
    if (pstru_client->b_active)
    {
        /* Check if client's socket descriptor is an active websocket */
        if (httpd_ws_get_fd_info (g_x_server, pstru_client->x_socket_fd) != HTTPD_WS_CLIENT_WEBSOCKET)
        {
            LOGW ("Client with socket descriptor %d is not active any more", pstru_client->x_socket_fd);
            pstru_client->b_active = false;

            /* Invoke callback */
            if (x_inst->pfnc_cb != NULL)
            {
                WSS_evt_data_t stru_evt_data =
                {
                    .x_inst             = x_inst,
                    .pv_arg             = x_inst->pv_cb_arg,
                    .u8_client_id       = u8_client_id,
                    .enm_evt            = WSS_EVT_CLIENT_DISCONNECTED,
                };
                x_inst->pfnc_cb (&stru_evt_data);
            }
        }
    }

    return pstru_client->b_active;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
