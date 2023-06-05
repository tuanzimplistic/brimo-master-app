/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_mqtt.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 May 22
**  @brief      : Implementation of Srvc_Mqtt module
**  @namespace  : MQTT
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Mqtt
** @brief       Abstracts MQTT interface and provides API to send/receive messages to/from MQTT topics.
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_mqtt.h"          /* Public header of this module */
#include "mqtt_client.h"        /* Use ESP-IDF's MQTT component */
#include <string.h>             /* Use strlen() */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Structure wrapping properties of a publish topic */
typedef struct
{
    uint32_t                    u32_topic_id;       //!< Index of this topic in publish topic table
    uint8_t                     u8_qos;             //!< Quality of service, which is 0, 1 or 2
    bool                        b_retained;         //!< Specifies the retain flag
    const char *                pstri_topic;        //!< The topic
} MQTT_pub_topic_t;

/** @brief  Macro expanding an entry in publish topic table as initialization values for MQTT_pub_topic_t struct */
#define PUB_TOPIC_TABLE_EXPAND_AS_STRUCT_INIT(TOPIC_ID, QOS, RETAIN, TOPIC)     \
{                                                                               \
    .u32_topic_id               = TOPIC_ID,                                     \
    .u8_qos                     = QOS,                                          \
    .b_retained                 = RETAIN,                                       \
    .pstri_topic                = TOPIC,                                        \
},

/** @brief  Structure wrapping properties of a subcribe topic */
typedef struct
{
    uint32_t                    u32_topic_id;       //!< Index of this topic in subcribe topic table
    uint8_t                     u8_qos;             //!< Quality of service, which is 0, 1 or 2
    const char *                pstri_topic;        //!< The topic
} MQTT_sub_topic_t;

/** @brief  Macro expanding an entry in subcribe topic table as initialization values for MQTT_sub_topic_t struct */
#define SUB_TOPIC_TABLE_EXPAND_AS_STRUCT_INIT(TOPIC_ID, QOS, TOPIC)             \
{                                                                               \
    .u32_topic_id               = TOPIC_ID,                                     \
    .u8_qos                     = QOS,                                          \
    .pstri_topic                = TOPIC,                                        \
},

/** @brief  Structure wrapping data of an MQTT client object */
struct MQTT_obj
{
    bool                        b_initialized;      //!< Specifies whether the object has been initialized or not
    MQTT_inst_id_t              enm_inst_id;        //!< Instance ID of this object
    bool                        b_started;          //!< Indicates if MQTT client is running
    bool                        b_connected;        //!< Indicates if MQTT client has been connected with the broker
    MQTT_callback_t             pfnc_cb;            //!< Callback invoked when an event occurs
    void *                      pv_cb_arg;          //!< Argument passed when the callback function was registered

    esp_mqtt_client_config_t    stru_mqtt_cfg;      //!< MQTT client configuration structure
    esp_mqtt_client_handle_t    x_mqtt_inst;        //!< MQTT client instance
    uint32_t                    u32_lwt_topic_id;   //!< Index in publish topic table of last will and testament topic

    uint8_t                     u8_num_pub_topics;  //!< Number of publish topics
    MQTT_pub_topic_t *          pstru_pub_topics;   //!< Pointer to array of publish topics

    uint8_t                     u8_num_sub_topics;  //!< Number of subcribe topics
    MQTT_sub_topic_t *          pstru_sub_topics;   //!< Pointer to array of subcribe topics
};

/** @brief  Macro expanding MQTT_INST_TABLE as initialization values for MQTT_obj struct */
#define INST_TABLE_EXPAND_AS_INST_STRUCT_INIT(INST_ID, PUB_TOPICS, SUB_TOPICS, URI, IP, PORT,           \
                                              USERNAME, PASSWORD, LWT_MSG, LWT_ID, TX_BUF, RX_BUF)      \
{                                                                                                       \
    .b_initialized              = false,                                                                \
    .enm_inst_id                = INST_ID,                                                              \
    .b_started                  = false,                                                                \
    .b_connected                = false,                                                                \
    .pfnc_cb                    = NULL,                                                                 \
    .pv_cb_arg                  = NULL,                                                                 \
                                                                                                        \
    .stru_mqtt_cfg              =                                                                       \
    {                                                                                                   \
        .uri                    = URI,                                                                  \
        .host                   = IP,                                                                   \
        .port                   = PORT,                                                                 \
        .username               = USERNAME,                                                             \
        .password               = PASSWORD,                                                             \
        .lwt_msg                = LWT_MSG,                                                              \
        .out_buffer_size        = TX_BUF,                                                               \
        .buffer_size            = RX_BUF,                                                               \
    },                                                                                                  \
    .x_mqtt_inst                = NULL,                                                                 \
    .u32_lwt_topic_id           = LWT_ID,                                                               \
                                                                                                        \
    .u8_num_pub_topics          = sizeof (g_astru_pub_topic_##INST_ID) / sizeof (MQTT_pub_topic_t),     \
    .pstru_pub_topics           = g_astru_pub_topic_##INST_ID,                                          \
                                                                                                        \
    .u8_num_sub_topics          = sizeof (g_astru_sub_topic_##INST_ID) / sizeof (MQTT_sub_topic_t),     \
    .pstru_sub_topics           = g_astru_sub_topic_##INST_ID,                                          \
},

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Mqtt";

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/**
** @brief   Arrays of all publish topics of all MQTT clients, one array for one client
** @note    These arrays should not be used directly but via .pstru_pub_topics member of MQTT objects
*/
#define INST_TABLE_EXPAND_AS_PUB_TOPIC_ARRAY_DECLARE(INST_ID, PUB_TOPICS, SUB_TOPICS, ...)  \
    static MQTT_pub_topic_t g_astru_pub_topic_##INST_ID [] = { PUB_TOPICS (PUB_TOPIC_TABLE_EXPAND_AS_STRUCT_INIT) };
MQTT_INST_TABLE (INST_TABLE_EXPAND_AS_PUB_TOPIC_ARRAY_DECLARE)

/**
** @brief   Arrays of all subcribe topics of all MQTT clients, one array for one client
** @note    These arrays should not be used directly but via .pstru_sub_topics member of MQTT objects
*/
#define INST_TABLE_EXPAND_AS_SUB_TOPIC_ARRAY_DECLARE(INST_ID, PUB_TOPICS, SUB_TOPICS, ...)  \
    static MQTT_sub_topic_t g_astru_sub_topic_##INST_ID [] = { SUB_TOPICS (SUB_TOPIC_TABLE_EXPAND_AS_STRUCT_INIT) };
MQTT_INST_TABLE (INST_TABLE_EXPAND_AS_SUB_TOPIC_ARRAY_DECLARE)

/** @brief  Array of all MQTT client objects */
static struct MQTT_obj g_astru_mqtt_objs[MQTT_NUM_INST] =
{
    MQTT_INST_TABLE (INST_TABLE_EXPAND_AS_INST_STRUCT_INIT)
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static int32_t s32_MQTT_Init_Module (void);
static int32_t s32_MQTT_Init_Inst (MQTT_inst_t x_inst);
static void v_MQTT_Evt_Handler (void * pv_arg, esp_event_base_t x_evt_base, int32_t s32_evt_id, void * pv_evt_data);

#ifdef USE_MODULE_ASSERT
 static bool b_MQTT_Is_Valid_Inst (MQTT_inst_t x_inst);
#endif

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets instance of an MQTT client
**
** @note
**      The associated MQTT client is NOT automatically started after this function is called. To start it,
**      enm_MQTT_Start_Inst() must be invoked.
**
** @param [in]
**      enm_inst_id: Index of the MQTT client to get. The index is expanded from MQTT_INST_TABLE
**
** @return
**      @arg    NULL: Failed to get instance corresponding with the given instance ID
**      @arg    Otherwise: Instance handle of the MQTT client
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
MQTT_inst_t x_MQTT_Get_Inst (MQTT_inst_id_t enm_inst_id)
{
    MQTT_inst_t x_inst = NULL;
    int32_t     s32_result = STATUS_OK;

    /* Validation */
    if (enm_inst_id >= MQTT_NUM_INST)
    {
        return NULL;
    }

    /* If this module has not been initialized, do that now */
    if (!g_b_initialized)
    {
        s32_result = s32_MQTT_Init_Module ();
        g_b_initialized = (s32_result == STATUS_OK);
    }

    /* If the retrieved instance has not been initialized yet, do that now */
    if (s32_result == STATUS_OK)
    {
        x_inst = &g_astru_mqtt_objs[enm_inst_id];
        if (!x_inst->b_initialized)
        {
            s32_result = s32_MQTT_Init_Inst (x_inst);
            x_inst->b_initialized = (s32_result == STATUS_OK);
        }
    }

    /* Return instance of the required SPP Server */
    return (s32_result == STATUS_OK) ? x_inst : NULL;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets current configuration of an MQTT client
**
** @param [in]
**      x_inst: Instance of the MQTT client returned by x_MQTT_Get_Inst()
**
** @param [out]
**      pstru_config: Current configuration
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_MQTT_Get_Config (MQTT_inst_t x_inst, MQTT_config_t * pstru_config)
{
    ASSERT_PARAM (b_MQTT_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (pstru_config != NULL);

    /* Get current configration */
    pstru_config->stri_uri          = x_inst->stru_mqtt_cfg.uri;
    pstru_config->stri_ip           = x_inst->stru_mqtt_cfg.host;
    pstru_config->u16_port          = (uint16_t)x_inst->stru_mqtt_cfg.port;
    pstru_config->stri_username     = x_inst->stru_mqtt_cfg.username;
    pstru_config->stri_password     = x_inst->stru_mqtt_cfg.password;
    pstru_config->stri_lwt_msg      = x_inst->stru_mqtt_cfg.lwt_msg;
    pstru_config->u32_lwt_topic_id  = x_inst->u32_lwt_topic_id;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Configures an MQTT client
**
** @note
**      If a client has already started, it must be stopped with enm_MQTT_Stop_Inst() before reconfiguring. After the
**      reconfiguration is done, the client can be restarted with enm_MQTT_Start_Inst()
**
** @param [in]
**      x_inst: Instance of the MQTT client returned by x_MQTT_Get_Inst()
**
** @param [out]
**      pstru_config: Current configuration
**
** @return
**      @arg    MQTT_OK
**      @arg    MQTT_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
MQTT_status_t enm_MQTT_Set_Config (MQTT_inst_t x_inst, MQTT_config_t * pstru_config)
{
    ASSERT_PARAM (b_MQTT_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (pstru_config != NULL);

    /* Change configration */
    x_inst->stru_mqtt_cfg.uri       = pstru_config->stri_uri;
    x_inst->stru_mqtt_cfg.host      = pstru_config->stri_ip;
    x_inst->stru_mqtt_cfg.port      = pstru_config->u16_port;
    x_inst->stru_mqtt_cfg.username  = pstru_config->stri_username;
    x_inst->stru_mqtt_cfg.password  = pstru_config->stri_password;
    x_inst->stru_mqtt_cfg.lwt_msg   = pstru_config->stri_lwt_msg;
    x_inst->u32_lwt_topic_id        = pstru_config->u32_lwt_topic_id;

    /* Update last will and testament configuration if the feature is enabled */
    if (x_inst->stru_mqtt_cfg.lwt_msg != NULL)
    {
        if (pstru_config->u32_lwt_topic_id >= x_inst->u8_num_pub_topics)
        {
            LOGE ("Invalid index %d of LWT topic", pstru_config->u32_lwt_topic_id);
            return MQTT_ERR;
        }
        x_inst->stru_mqtt_cfg.lwt_topic     = x_inst->pstru_pub_topics[x_inst->u32_lwt_topic_id].pstri_topic;
        x_inst->stru_mqtt_cfg.lwt_qos       = x_inst->pstru_pub_topics[x_inst->u32_lwt_topic_id].u8_qos;
        x_inst->stru_mqtt_cfg.lwt_retain    = x_inst->pstru_pub_topics[x_inst->u32_lwt_topic_id].b_retained;
    }

    /* Apply the configuration to MQTT client */
    ESP_ERROR_CHECK (esp_mqtt_set_config (x_inst->x_mqtt_inst, &x_inst->stru_mqtt_cfg));

    return MQTT_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Changes a publish topic, pstri_topic must be static (i.e not in stack memory). This function could be called
**      after the associated MQTT client has been started.
**
** @param [in]
**      x_inst: Instance of the MQTT client returned by x_MQTT_Get_Inst()
**
** @param [in]
**      u32_pub_topic_id: Index of the topic inside publish topic table of the client
**
** @param [in]
**      pstri_topic: NULL-terminated string of the new topic
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_MQTT_Set_Publish_Topic (MQTT_inst_t x_inst, uint32_t u32_pub_topic_id, const char * pstri_topic)
{
    ASSERT_PARAM (b_MQTT_Is_Valid_Inst (x_inst));
    ASSERT_PARAM ((u32_pub_topic_id < x_inst->u8_num_pub_topics) && (pstri_topic != NULL));

    /* Set the publish topic */
    x_inst->pstru_pub_topics[u32_pub_topic_id].pstri_topic = pstri_topic;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Changes a subscribe topic, pstri_topic must be static (i.e not in stack memory)
**
** @note
**      This function must be called while the relevant MQTT client is not started. If the MQTT client has been started,
**      it could be stopped with enm_MQTT_Stop_Inst() before calling this function.
**
** @param [in]
**      x_inst: Instance of the MQTT client returned by x_MQTT_Get_Inst()
**
** @param [in]
**      u32_sub_topic_id: Index of the topic inside subcribe topic table of the client
**
** @param [in]
**      pstri_topic: NULL-terminated string of the new topic
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_MQTT_Set_Subscribe_Topic (MQTT_inst_t x_inst, uint32_t u32_sub_topic_id, const char * pstri_topic)
{
    ASSERT_PARAM (b_MQTT_Is_Valid_Inst (x_inst));
    ASSERT_PARAM ((u32_sub_topic_id < x_inst->u8_num_sub_topics) && (pstri_topic != NULL));
    ASSERT_PARAM (!x_inst->b_started);

    /* Set the subscribe topic */
    x_inst->pstru_sub_topics[u32_sub_topic_id].pstri_topic = pstri_topic;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Registers a callback function invoked when an event occurs to an MQTT client
**
** @param [in]
**      x_inst: Instance of the MQTT client returned by x_MQTT_Get_Inst()
**
** @param [in]
**      pfnc_cb: Callback function to register
**
** @param [in]
**      pv_arg: Optional argument which will be forwarded to the data of callback function when it's invoked
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_MQTT_Register_Callback (MQTT_inst_t x_inst, MQTT_callback_t pfnc_cb, void * pv_arg)
{
    /* Validation */
    ASSERT_PARAM ((x_inst != NULL) && (x_inst->b_initialized));

    /* Store the callback function */
    x_inst->pfnc_cb = pfnc_cb;
    x_inst->pv_cb_arg = pv_arg;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Starts an MQTT client
**
** @note
**      This function must be called after the network interface (e.g, wifi) is initialized. Otherwise the task of MQTT
**      client will fail and terminate.
**
** @param [in]
**      x_inst: Instance of the MQTT client returned by x_MQTT_Get_Inst()
**
** @return
**      @arg    MQTT_OK
**      @arg    MQTT_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
MQTT_status_t enm_MQTT_Start_Inst (MQTT_inst_t x_inst)
{
    /* Validation */
    ASSERT_PARAM (b_MQTT_Is_Valid_Inst (x_inst));
    LOGD ("Starting instance %d", x_inst->enm_inst_id);

    /* Start MQTT client and its task */
    if (!x_inst->b_started)
    {
        if (esp_mqtt_client_start (x_inst->x_mqtt_inst) != ESP_OK)
        {
            LOGE ("Failed to start client %d", x_inst->enm_inst_id);
            return MQTT_ERR;
        }
        x_inst->b_started = true;
    }

    LOGI ("MQTT instance %d has been started successfully", x_inst->enm_inst_id);
    return MQTT_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Stops an MQTT client
**
** @note
**      This function shall terminate the task of MQTT client. Therefore it cannot be called inside the context of
**      the MQTT client task (MQTT event handler)
**
** @param [in]
**      x_inst: Instance of the MQTT client returned by x_MQTT_Get_Inst()
**
** @return
**      @arg    MQTT_OK
**      @arg    MQTT_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
MQTT_status_t enm_MQTT_Stop_Inst (MQTT_inst_t x_inst)
{
    /* Validation */
    ASSERT_PARAM (b_MQTT_Is_Valid_Inst (x_inst));
    LOGD ("Stopping instance %d", x_inst->enm_inst_id);

    /* Stop MQTT client and its task */
    if (x_inst->b_started)
    {
        if (esp_mqtt_client_stop (x_inst->x_mqtt_inst) != ESP_OK)
        {
            LOGE ("Failed to stop MQTT client %d", x_inst->enm_inst_id);
            return MQTT_ERR;
        }
        x_inst->b_started = false;
    }

    LOGI ("MQTT instance %d has been stopped successfully", x_inst->enm_inst_id);
    return MQTT_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Publishes data to a topic
**
** @note
**      This API might block for several seconds, either due to network timeout (10s) or if publishing payloads
**      longer than internal buffer (due to message fragmentation)
**
** @param [in]
**      x_inst: Instance of the MQTT client returned by x_MQTT_Get_Inst()
**
** @param [in]
**      u32_pub_topic_id: Index of the topic inside publish topic table of the client
**
** @param [in]
**      pv_data: The data to publish
**
** @param [in]
**      u32_len: Length in bytes of pv_data. If u32_len is zero, length of pv_data will be determined using strlen()
**
** @return
**      @arg    MQTT_OK
**      @arg    MQTT_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
MQTT_status_t enm_MQTT_Publish (MQTT_inst_t x_inst, uint32_t u32_pub_topic_id, const void * pv_data, uint32_t u32_len)
{
    /* Validation */
    ASSERT_PARAM (b_MQTT_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (pv_data != NULL);
    ASSERT_PARAM (u32_pub_topic_id < x_inst->u8_num_pub_topics);

    /* Publish the data to the topic */
    if (esp_mqtt_client_publish (x_inst->x_mqtt_inst,
                                 x_inst->pstru_pub_topics[u32_pub_topic_id].pstri_topic,
                                 pv_data, u32_len,
                                 x_inst->pstru_pub_topics[u32_pub_topic_id].u8_qos,
                                 x_inst->pstru_pub_topics[u32_pub_topic_id].b_retained) < 0)
    {
        LOGE ("Failed to publish topic ID %d", u32_pub_topic_id);
        return MQTT_ERR;
    }

    return MQTT_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_MQTT module
**
** @return
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int32_t s32_MQTT_Init_Module (void)
{
    /* Do nothing */
    return STATUS_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes MQTT client
**
** @param [in]
**      x_inst: A specific MQTT client
**
** @return
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int32_t s32_MQTT_Init_Inst (MQTT_inst_t x_inst)
{
    int32_t s32_result = STATUS_OK;

    /* Validation */
    ASSERT_PARAM (b_MQTT_Is_Valid_Inst (x_inst));
    LOGD ("Initializing instance %d", x_inst->enm_inst_id);

    /* If last will and testament feature is used */
    if (x_inst->stru_mqtt_cfg.lwt_msg != NULL)
    {
        ASSERT_PARAM (x_inst->u32_lwt_topic_id < x_inst->u8_num_pub_topics);
        x_inst->stru_mqtt_cfg.lwt_topic     = x_inst->pstru_pub_topics[x_inst->u32_lwt_topic_id].pstri_topic;
        x_inst->stru_mqtt_cfg.lwt_qos       = x_inst->pstru_pub_topics[x_inst->u32_lwt_topic_id].u8_qos;
        x_inst->stru_mqtt_cfg.lwt_retain    = x_inst->pstru_pub_topics[x_inst->u32_lwt_topic_id].b_retained;
    }

    /* Create the MQTT client handle based on the configuration */
    if (s32_result == STATUS_OK)
    {
        x_inst->x_mqtt_inst = esp_mqtt_client_init (&x_inst->stru_mqtt_cfg);
        if (x_inst->x_mqtt_inst == NULL)
        {
            s32_result = STATUS_ERR;
        }
    }

    /* Register MQTT event */
    if (s32_result == STATUS_OK)
    {
        ESP_ERROR_CHECK (esp_mqtt_client_register_event (x_inst->x_mqtt_inst, ESP_EVENT_ANY_ID,
                                                         v_MQTT_Evt_Handler, x_inst));
    }

    /* Done */
    return s32_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**     Handler of subscribed events
**
** @param [in]
**      pv_arg: Argument of the event fired
**
** @param [in]
**      x_evt_base: Group of the event fired
**
** @param [in]
**      s32_evt_id: ID of the event fired
**
** @param [in]
**      pv_evt_data: Context data of the event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTT_Evt_Handler (void * pv_arg, esp_event_base_t x_evt_base, int32_t s32_evt_id, void * pv_evt_data)
{
    MQTT_inst_t                 x_inst = (MQTT_inst_t) pv_arg;
    esp_mqtt_event_handle_t     stru_mqtt_event = (esp_mqtt_event_handle_t) pv_evt_data;

    /* Validation */
    ASSERT_PARAM (b_MQTT_Is_Valid_Inst (x_inst));

    /* Process MQTT events */
    switch (stru_mqtt_event->event_id)
    {
        /* Before connecting to the broker */
        case MQTT_EVENT_BEFORE_CONNECT:
        {
            LOGD ("Event MQTT_EVENT_BEFORE_CONNECT on client %d", x_inst->enm_inst_id);
            break;
        }

        /* Connected with the MQTT broker */
        case MQTT_EVENT_CONNECTED:
        {
            LOGD ("Event MQTT_EVENT_CONNECTED on client %d", x_inst->enm_inst_id);

            /* Subcribe to topics */
            for (uint32_t u32_idx = 0; u32_idx < x_inst->u8_num_sub_topics; u32_idx++)
            {
                if (esp_mqtt_client_subscribe (x_inst->x_mqtt_inst, x_inst->pstru_sub_topics[u32_idx].pstri_topic,
                                               x_inst->pstru_sub_topics[u32_idx].u8_qos) < 0)
                {
                    LOGE ("Client %d failed to subcribe topic %s", x_inst->enm_inst_id,
                                    x_inst->pstru_sub_topics[u32_idx].pstri_topic);
                }
            }

            /* Notify via callback */
            if (!x_inst->b_connected)
            {
                LOGI ("Client %d has been connected with MQTT broker", x_inst->enm_inst_id);
                x_inst->b_connected = true;
                if (x_inst->pfnc_cb)
                {
                    MQTT_evt_data_t stru_evt_data =
                    {
                        .x_inst         = x_inst,
                        .pv_arg         = x_inst->pv_cb_arg,
                        .enm_evt        = MQTT_EVT_CONNECTED,
                    };
                    x_inst->pfnc_cb (&stru_evt_data);
                }
            }
            break;
        }

        /* Disconnected with the MQTT broker */
        case MQTT_EVENT_DISCONNECTED:
        {
            LOGD ("Event MQTT_EVENT_DISCONNECTED on client %d", x_inst->enm_inst_id);

            /* Notify via callback */
            if (x_inst->b_connected)
            {
                LOGI ("Client %d is disconnected with MQTT broker", x_inst->enm_inst_id);
                x_inst->b_connected = false;
                if (x_inst->pfnc_cb)
                {
                    MQTT_evt_data_t stru_evt_data =
                    {
                        .x_inst         = x_inst,
                        .pv_arg         = x_inst->pv_cb_arg,
                        .enm_evt        = MQTT_EVT_DISCONNECTED,
                    };
                    x_inst->pfnc_cb (&stru_evt_data);
                }
            }
            break;
        }

        /* Received data from a subscribed topic */
        case MQTT_EVENT_DATA:
        {
            LOGD ("Event MQTT_EVENT_DATA on client %d", x_inst->enm_inst_id);

            /* Invoke callback */
            if (x_inst->pfnc_cb)
            {
                MQTT_evt_data_t stru_evt_data =
                {
                    .x_inst             = x_inst,
                    .pv_arg             = x_inst->pv_cb_arg,
                    .enm_evt            = MQTT_EVT_DATA_RECEIVED,

                    /*
                    ** Multiple MQTT_EVENT_DATA could be fired for one message, if it is longer than internal buffer.
                    ** In that case only first event contains topic pointer and length, others contain data only with
                    ** current data length and current data offset updating.
                    */
                    .stru_receive       =
                    {
                        .pstri_topic    = stru_mqtt_event->topic,
                        .u16_topic_len  = stru_mqtt_event->topic_len,
                        .pv_data        = stru_mqtt_event->data,
                        .u32_data_len   = stru_mqtt_event->data_len,
                        .u32_offset     = stru_mqtt_event->current_data_offset,
                        .u32_totlen     = stru_mqtt_event->total_data_len,
                    },
                };
                x_inst->pfnc_cb (&stru_evt_data);
            }
            break;
        }

        /* Error occurs */
        case MQTT_EVENT_ERROR:
        {
            LOGD ("Event MQTT_EVENT_ERROR on client %d", x_inst->enm_inst_id);
            break;
        }

        /* Other events */
        default:
        {
            LOGD ("MQTT event %d occurs on client %d", stru_mqtt_event->event_id, x_inst->enm_inst_id);
            break;
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
static bool b_MQTT_Is_Valid_Inst (MQTT_inst_t x_inst)
{
    uint32_t u32_idx = 0;

    /* Searching instance */
    for (u32_idx = 0; u32_idx < (uint32_t)MQTT_NUM_INST; u32_idx++)
    {
        if (x_inst == &g_astru_mqtt_objs[u32_idx])
        {
            /* Stop searching if there is one matching instance  */
            return true;
        }
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
