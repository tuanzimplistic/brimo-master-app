/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_master_transport.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Dec 16
**  @brief      : Implementation of Srvc_Master_Transport module
**  @namespace  : MTP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Master_Transport
** @brief       Abstracts transport layer (client side) of Bootloader protocol
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_master_transport.h"      /* Public header of this module */
#include "srvc_master_datalink.h"       /* Use Bootloader Data-link layer */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/event_groups.h"      /* Use FreeRTOS event group */

#include <string.h>                     /* Use memcpy() */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Maximum number of callback functions */
#define MTP_NUM_CB          1

/** @brief  Maximum length in bytes of a Master transport message */
#define MTP_MAX_MSG_LEN     247

/** @brief  Structure wrapping data of a Master transport channel */
struct MTP_obj
{
    bool                    b_initialized;              //!< Specifies whether the object has been initialized or not
    MDL_inst_t              x_datalink_inst;            //!< Instance of the data-link channel

    EventGroupHandle_t      x_os_evt_group;                     //!< FreeRTOS event group
    uint8_t                 au8_response [MTP_MAX_MSG_LEN];     //!< Data of response message received
    uint16_t                u16_response_len;                   //!< Length in bytes of response message received

    MTP_cb_t                apfnc_cb [MTP_NUM_CB];      //!< Callback function invoked when an event occurs
    uint8_t                 u8_request_eid;             //!< Current exchange ID of request message
    uint8_t                 u8_post_eid;                //!< Current exchange ID of post message
    uint8_t                 u8_notify_eid;              //!< Current exchange ID of notification message
};

/** @brief  Transport message type */
enum
{
    MTP_MSG_REQUEST         = 0,                        //!< Request message
    MTP_MSG_RESPONSE        = 1,                        //!< Response message
    MTP_MSG_POST            = 2,                        //!< Post message
    MTP_MSG_NOTIFY          = 3,                        //!< Notification message
};

/** @brief  Structure of Master transport message */
typedef struct
{
    uint8_t                 u8_eid;                     //!< Exchange ID
    uint8_t                 u8_type;                    //!< Message type
    uint8_t                 au8_payload[];              //!< Transport payload

} MTP_msg_t;

/** @brief  FreeRTOS event fired when a response message is received */
#define MTP_RESPONSE_EVT_BIT            (BIT0)

/** @brief  Number of request retries */
#define MTP_NUM_REQUEST_RETRIES         3

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Master_Transport";

/** @brief  Single instance of Master transport channel */
static struct MTP_obj g_stru_mtp_obj =
{
    .b_initialized          = false,
    .x_datalink_inst        = NULL,

    .x_os_evt_group         = NULL,
    .u16_response_len       = 0,

    .apfnc_cb               = { NULL },
    .u8_request_eid         = 255,
    .u8_post_eid            = 255,
    .u8_notify_eid          = 0,
};

/** @brief  Indicates if this module has been initialized or not */
static bool g_b_initialized = false;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifdef USE_MODULE_ASSERT
 static bool b_MTP_Is_Valid_Inst (MTP_inst_t x_inst);
#endif

static int8_t s8_MTP_Init_Module (void);
static int8_t s8_MTP_Init_Inst (MTP_inst_t x_inst);
static void v_MTP_Datalink_Cb (MDL_inst_t x_datalink_inst, MDL_evt_t enm_evt, const void * pv_data, uint16_t u16_len);
static void v_MTP_Process_Msg_Received (MTP_inst_t x_inst, MTP_msg_t * pstru_msg, uint16_t u16_msg_len);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets instance of Master transport channel
**
** @param [out]
**      px_inst: Container to store the retrieved instance
**
** @return
**      @arg    MTP_OK
**      @arg    MTP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MTP_Get_Inst (MTP_inst_t * px_inst)
{
    MTP_inst_t  x_inst = NULL;
    int8_t      s8_result = MTP_OK;

    /* Validation */
    ASSERT_PARAM (px_inst != NULL);

    /* Initialize */
    *px_inst = NULL;

    /* If this module has not been initialized, do that now */
    if (s8_result >= MTP_OK)
    {
        if (!g_b_initialized)
        {
            s8_result = s8_MTP_Init_Module ();
            if (s8_result >= MTP_OK)
            {
                g_b_initialized = true;
            }
        }
    }

    /* If the retrieved instance has not been initialized yet, do that now */
    if (s8_result >= MTP_OK)
    {
        x_inst = &g_stru_mtp_obj;
        if (!x_inst->b_initialized)
        {
            s8_result = s8_MTP_Init_Inst (x_inst);
            if (s8_result >= MTP_OK)
            {
                x_inst->b_initialized = true;
            }
        }
    }

    /* Return instance of the Master transport channel */
    if (s8_result >= MTP_OK)
    {
        *px_inst = x_inst;
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs Master transport channel
**
** @note
**      This function must be called periodically
**
** @param [in]
**      x_inst: Specific instance
**
** @return
**      @arg    MTP_OK
**      @arg    MTP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MTP_Run_Inst (MTP_inst_t x_inst)
{
    ASSERT_PARAM (b_MTP_Is_Valid_Inst (x_inst));

    /* Run data-link channel */
    int8_t s8_result = s8_MDL_Run_Inst (x_inst->x_datalink_inst);

    /* Done */
    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Registers callack function to a Master transport channel
**
** @note
**      This function is not thread-safe
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pfnc_cb: Pointer of the callback function
**
** @return
**      @arg    MTP_OK
**      @arg    MTP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MTP_Register_Cb (MTP_inst_t x_inst, MTP_cb_t pfnc_cb)
{
    ASSERT_PARAM (b_MTP_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized && (pfnc_cb != NULL));

    /* Store callback function pointer */
    for (uint8_t u8_idx = 0; u8_idx < MTP_NUM_CB; u8_idx++)
    {
        if (x_inst->apfnc_cb [u8_idx] == NULL)
        {
            x_inst->apfnc_cb [u8_idx] = pfnc_cb;
            return MTP_OK;
        }
    }

    LOGE ("Failed to register callback function");
    return MTP_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends request message to a Master transport channel and waits for a response message
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pv_request: The request data to send
**
** @param [in]
**      u16_request_len: Length in bytes of pv_request
**
** @param [out]
**      ppu8_response: Pointer to the buffer containing response data received
**
** @param [out]
**      pu16_response_len: Length in bytes of the response received
**
** @param [out]
**      u16_timeout: Interval in milliseconds waiting for the reponse before retrying sending the request
**
** @return
**      @arg    MTP_OK
**      @arg    MTP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MTP_Send_Request (MTP_inst_t x_inst, const void * pv_request, uint16_t u16_request_len,
                            uint8_t ** ppu8_response, uint16_t * pu16_response_len, uint16_t u16_timeout)
{
    static uint8_t  au8_message [MTP_MAX_MSG_LEN];
    MTP_msg_t *     pstru_msg = (MTP_msg_t *)au8_message;

    /* Validation */
    ASSERT_PARAM (b_MTP_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized && (pv_request != NULL) && (u16_request_len != 0) &&
                  (ppu8_response != NULL) && (pu16_response_len != NULL));
    if (u16_request_len > MTP_MAX_MSG_LEN - sizeof (MTP_msg_t))
    {
        LOGE ("Invalid request length %d", u16_request_len);
        return MTP_ERR;
    }

    /* Construct the transport message to send */
    pstru_msg->u8_eid = ++x_inst->u8_request_eid;
    pstru_msg->u8_type = MTP_MSG_REQUEST;
    memcpy (pstru_msg->au8_payload, pv_request, u16_request_len);

    /* Prepare to receive reponse from client */
    xEventGroupClearBits (x_inst->x_os_evt_group, MTP_RESPONSE_EVT_BIT);
    x_inst->u16_response_len = 0;

    /* Send request and wait for response. If response is not received, retry sending the request */
    for (uint8_t u8_retry = 0; u8_retry < MTP_NUM_REQUEST_RETRIES; u8_retry++)
    {
        /* Send the request */
        if (s8_MDL_Send (x_inst->x_datalink_inst, pstru_msg, sizeof (MTP_msg_t) + u16_request_len) < MDL_OK)
        {
            LOGE ("Failed to send request");
            return MTP_ERR;
        }

        /* Wait for response */
        EventBits_t x_event_bits = xEventGroupWaitBits (x_inst->x_os_evt_group, MTP_RESPONSE_EVT_BIT,
                                                        pdTRUE, pdFALSE, pdMS_TO_TICKS (u16_timeout));
        if (x_event_bits & MTP_RESPONSE_EVT_BIT)
        {
            /* A response message has been received */
            *ppu8_response = x_inst->au8_response;
            *pu16_response_len = x_inst->u16_response_len;
            return MTP_OK;
        }
    }

    /* No reponse is received */
    return MTP_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends post message to a Master transport channel
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pv_post: The post data to send
**
** @param [in]
**      u16_post_len: Length in bytes of pv_post
**
** @return
**      @arg    MTP_OK
**      @arg    MTP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MTP_Send_Post (MTP_inst_t x_inst, const void * pv_post, uint16_t u16_post_len)
{
    static uint8_t  au8_message [MTP_MAX_MSG_LEN];
    MTP_msg_t *     pstru_msg = (MTP_msg_t *)au8_message;

    /* Validation */
    ASSERT_PARAM (b_MTP_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized && (pv_post != NULL) && (u16_post_len != 0));
    if (u16_post_len > MTP_MAX_MSG_LEN - sizeof (MTP_msg_t))
    {
        return MTP_ERR;
    }

    /* Construct the transport message to send */
    pstru_msg->u8_eid = ++x_inst->u8_post_eid;
    pstru_msg->u8_type = MTP_MSG_POST;
    memcpy (pstru_msg->au8_payload, pv_post, u16_post_len);

    /* Send the post message */
    if (s8_MDL_Send (x_inst->x_datalink_inst, pstru_msg, sizeof (MTP_msg_t) + u16_post_len) < MDL_OK)
    {
        return MTP_ERR;
    }

    return MTP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_Master_Transport module
**
** @return
**      @arg    MTP_OK
**      @arg    MTP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MTP_Init_Module (void)
{
    /* Do nothing */
    return MTP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes a Master transport channel
**
** @param [in]
**      x_inst: Specific instance
**
** @return
**      @arg    MTP_OK
**      @arg    MTP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MTP_Init_Inst (MTP_inst_t x_inst)
{
    /* Get instance of the associated data-link channel */
    if (s8_MDL_Get_Inst (&x_inst->x_datalink_inst) < MDL_OK)
    {
        LOGE ("Failed to get instance of data-link channel");
        return MTP_ERR;
    }

    /* Initialize callback function pointers */
    for (uint8_t u8_idx = 0; u8_idx < MTP_NUM_CB; u8_idx++)
    {
        x_inst->apfnc_cb [u8_idx] = NULL;
    }

    /* Create FreeRTOS event group */
    x_inst->x_os_evt_group = xEventGroupCreate ();

    /* Register callback function to event from data-link layer */
    if (s8_MDL_Register_Cb (x_inst->x_datalink_inst, v_MTP_Datalink_Cb) < MDL_OK)
    {
        LOGE ("Failed to register callback function to data-link channel");
        return MTP_ERR;
    }

    return MTP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Callback invoked when an event from data-link layer fires
**
** @param [in]
**      x_datalink_inst: Specific instance of data-link layer
**
** @param [in]
**      enm_evt: The event from data-link layer fired
**
** @param [in]
**      pv_data: Context data of the event
**
** @param [in]
**      u16_len: Length in bytes of the data in pv_data buffer
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MTP_Datalink_Cb (MDL_inst_t x_datalink_inst, MDL_evt_t enm_evt, const void * pv_data, uint16_t u16_len)
{
    MTP_inst_t x_inst = &g_stru_mtp_obj;

    /* Process the event of receiving message */
    if (enm_evt == MDL_EVT_MSG_RECEIVED)
    {
        v_MTP_Process_Msg_Received (x_inst, (MTP_msg_t *)pv_data, u16_len);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Processes a transport message received from client
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pstru_msg: The message received
**
** @param [in]
**      u16_msg_len: Length in bytes of the whole message (including header)
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MTP_Process_Msg_Received (MTP_inst_t x_inst, MTP_msg_t * pstru_msg, uint16_t u16_msg_len)
{
    /* Process notification message */
    if (pstru_msg->u8_type == MTP_MSG_NOTIFY)
    {
        /* Check if this is a new notification */
        if ((pstru_msg->u8_eid == 0) || (pstru_msg->u8_eid != x_inst->u8_notify_eid))
        {
            x_inst->u8_notify_eid = pstru_msg->u8_eid;

            /* Pass the notification to higher layer for further processing */
            for (uint8_t u8_idx = 0; u8_idx < MTP_NUM_CB; u8_idx++)
            {
                if (x_inst->apfnc_cb [u8_idx] != NULL)
                {
                    x_inst->apfnc_cb [u8_idx] (x_inst, MTP_EVT_NOTIFY,
                                               pstru_msg->au8_payload, u16_msg_len - sizeof (MTP_msg_t));
                }
            }
        }
    }

    /* Process response message */
    else if (pstru_msg->u8_type == MTP_MSG_RESPONSE)
    {
        /*
        ** Check the following conditions:
        ** + If response receiving is ready
        ** + If the response matches with the current request
        ** + If the response size is valid
        */
        if ((x_inst->u16_response_len == 0) &&
            (pstru_msg->u8_eid == x_inst->u8_request_eid) &&
            (u16_msg_len <= MTP_MAX_MSG_LEN))
        {
            /* Get the response */
            x_inst->u16_response_len = u16_msg_len - sizeof (MTP_msg_t);
            memcpy (x_inst->au8_response, pstru_msg->au8_payload, x_inst->u16_response_len);
            xEventGroupSetBits (x_inst->x_os_evt_group, MTP_RESPONSE_EVT_BIT);
        }
    }
}

#ifdef USE_MODULE_ASSERT

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Check if an instance is a vaild instance of this module
**
** @param [in]
**      x_inst: instance to check
**
** @return
**      Result
**      @arg    true: Valid instance
**      @arg    false: Invalid instance
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static bool b_MTP_Is_Valid_Inst (MTP_inst_t x_inst)
{
    if (x_inst == &g_stru_mtp_obj)
    {
        return true;
    }

    LOGE ("Invalid instance");
    return false;
}

#endif

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
