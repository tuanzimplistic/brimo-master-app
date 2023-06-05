/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : tx_messages.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 May 24
**  @brief      : This file contains helper functions to send response, notify, and data messages.
**                app_mqtt_mngr.c includes this file directly.
**  @namespace  : MQTTMN
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Mqtt_Mngr
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_fwu_esp32.h"         /* Get firmware version of master firmware */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Maximum data size in bytes that ESP32 client sends to download topic each time */
#define MAX_DOWNLOAD_CHUNK_LEN              16384

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a scanNotify command
**
** @details
**      This command is used to respond to a scanPost command. Besides, a client node shall also send this command when
**      it first connects to the MQTT network to notify its presence and some of its information.
**      Extra command data:
**          "state":"<devState>"
**          "masterFwVer":"<version>"
**          "slaveFwVer":"<version>"
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MQTTMN_Send_scanNotify (void)
{
    /* Construct the notify */
    cJSON * px_notify_root = cJSON_CreateObject ();
    cJSON_AddStringToObject (px_notify_root, JSON_KEY_CMD, "scanNotify");
    cJSON_AddNumberToObject (px_notify_root, JSON_KEY_EID, g_u32_notify_eid);
    g_u32_notify_eid = g_u32_notify_eid == 0x7FFFFFFF ? 1 : g_u32_notify_eid + 1;

    /** @todo   Determine device state */
    const char * pstri_dev_state = "idle";
    cJSON_AddStringToObject (px_notify_root, "state", pstri_dev_state);

    /* Version of master firmware */
    FWUESP_fw_desc_t stru_fw_desc;
    s8_FWUESP_Get_Fw_Descriptor (&stru_fw_desc);
    cJSON_AddStringToObject (px_notify_root, "masterFwVer", stru_fw_desc.pstri_ver);

    /** @todo   Determine version of slave firmware */
    const char * pstri_slave_version = "0.0.0";
    cJSON_AddStringToObject (px_notify_root, "slaveFwVer", pstri_slave_version);

    /* Publish the notify */
    char * pstri_notify = cJSON_Print (px_notify_root);
    cJSON_Delete (px_notify_root);
    if (pstri_notify != NULL)
    {
        enm_MQTT_Publish (g_x_mqtt, MQTT_S2M_NOTIFY, pstri_notify, 0);
        free (pstri_notify);
        return MQTTMN_OK;
    }

    LOGE ("Failed to construct command scanNotify");
    return MQTTMN_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a statusNotify command
**
** @details
**      This command is used by Rotimatic node to notify status or result of an operation such as firmware updating,
**      file uploading, etc.
**      Extra command data:
**          "statusType":"<statusType>"
**          "statusValue":"<statusValue>"
**          "description":"<statusDescription>"
**
** @param [in]
**      pstri_type: Type of the status
**      @arg    NOTIFY_FILE_UPLOAD_STATUS
**      @arg    NOTIFY_FILE_DOWNLOAD_STATUS
**      @arg    NOTIFY_OTA_DOWNLOAD_PROGRESS
**      @arg    NOTIFY_OTA_INSTALL_PROGRESS
**      @arg    NOTIFY_OTA_UPDATE_STATUS
**
** @param [in]
**      pstri_value: Value of the status
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**      @arg    STATUS_CANCELLED
**      @arg    0, 1, …, 100
**
** @param [in]
**      pstri_desc: Status description
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MQTTMN_Send_statusNotify (const char * pstri_type, const char * pstri_value, const char * pstri_desc)
{
    /* Construct the notify */
    cJSON * px_notify_root = cJSON_CreateObject ();
    cJSON_AddStringToObject (px_notify_root, JSON_KEY_CMD, "statusNotify");
    cJSON_AddNumberToObject (px_notify_root, JSON_KEY_EID, g_u32_notify_eid);
    g_u32_notify_eid = g_u32_notify_eid == 0x7FFFFFFF ? 1 : g_u32_notify_eid + 1;

    cJSON_AddStringToObject (px_notify_root, "statusType", pstri_type);
    cJSON_AddStringToObject (px_notify_root, "statusValue", pstri_value);
    cJSON_AddStringToObject (px_notify_root, "description", pstri_desc);

    /* Publish the notify */
    char * pstri_notify = cJSON_Print (px_notify_root);
    cJSON_Delete (px_notify_root);
    if (pstri_notify != NULL)
    {
        enm_MQTT_Publish (g_x_mqtt, MQTT_S2M_NOTIFY, pstri_notify, 0);
        free (pstri_notify);
        return MQTTMN_OK;
    }

    LOGE ("Failed to construct command statusNotify");
    return MQTTMN_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a paramReadResponse command
**
** @details
**      This command is used to respond to a paramReadRequest command. Values of the requested settings shall be
**      returned as strings.
**      Extra command data:
**          "status":"<commandStatus>"
**          "parameters":[ { "puc":<puc1>, "value":"<value1>"}, {"puc":<puc2>, "value":"<value2>"}, ... ]
**
** @param [in]
**      pstru_session: the session to send the command
**
** @param [in]
**      pstri_status: Command status
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**      @arg    STATUS_ERR_NOT_SUPPORTED
**      @arg    STATUS_ERR_INVALID_DATA
**      @arg    STATUS_ERR_BUSY
**      @arg    STATUS_ERR_STATE_NOT_ALLOWED
**      @arg    STATUS_ERR_INVALID_ACCESS
**
** @param [in]
**      pu16_puc_list: Array of PUCs to get values
**
** @param [in]
**      u8_num_pucs: Number of PUCs in pu16_puc_list
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MQTTMN_Send_paramReadResponse (MQTTMN_session_t * pstru_session, const char * pstri_status,
                                                uint16_t * pu16_puc_list, uint8_t u8_num_pucs)
{
    bool b_success = (strcmp (pstri_status, STATUS_OK) == 0);

    /* Construct the response */
    cJSON * px_response_root = cJSON_CreateObject ();
    cJSON_AddStringToObject (px_response_root, JSON_KEY_CMD, "paramReadResponse");
    cJSON_AddNumberToObject (px_response_root, JSON_KEY_EID, pstru_session->u32_request_eid);
    cJSON_AddStringToObject (px_response_root, "status", pstri_status);
    if (b_success)
    {
        /* Array of PUCs and parameter values */
        cJSON * px_param_array = cJSON_CreateArray ();
        cJSON_AddItemToObject (px_response_root, "parameters", px_param_array);
        for (uint8_t u8_idx = 0; u8_idx < u8_num_pucs; u8_idx++)
        {
            uint16_t u16_puc = pu16_puc_list[u8_idx];

            /* Get index of the parameter */
            PARAM_id_t enm_param_id;
            if (s8_PARAM_Convert_PUC_To_ID (u16_puc, &enm_param_id) != PARAM_OK)
            {
                LOGW ("Parameter with PUC 0x%02X is not available, ignore it", u16_puc);
                continue;
            }

            /* Get data type of the parameter, Json does not support numbers exceeding 32-bit value */
            PARAM_base_type_t enm_type;
            s8_PARAM_Get_Type (enm_param_id, &enm_type);
            if ((enm_type == BASE_TYPE_uint64_t) || (enm_type == BASE_TYPE_int64_t))
            {
                LOGW ("Data type of parameter with PUC 0x%02X is not supported", u16_puc);
                continue;
            }

            /* Json object of this parameter */
            cJSON * px_param_node = cJSON_CreateObject ();
            cJSON_AddItemToArray (px_param_array, px_param_node);
            cJSON_AddNumberToObject (px_param_node, "puc", u16_puc);

            /* Get parameter value */
            char stri_value[16];
            switch (enm_type)
            {
                case BASE_TYPE_uint8_t:
                {
                    uint8_t u8_value = 0;
                    s8_PARAM_Get_Uint8 (enm_param_id, &u8_value);
                    sprintf (stri_value, "%d", u8_value);
                    cJSON_AddStringToObject (px_param_node, "value", stri_value);
                    break;
                }

                case BASE_TYPE_int8_t:
                {
                    int8_t s8_value = 0;
                    s8_PARAM_Get_Int8 (enm_param_id, &s8_value);
                    sprintf (stri_value, "%d", s8_value);
                    cJSON_AddStringToObject (px_param_node, "value", stri_value);
                    break;
                }

                case BASE_TYPE_uint16_t:
                {
                    uint16_t u16_value = 0;
                    s8_PARAM_Get_Uint16 (enm_param_id, &u16_value);
                    sprintf (stri_value, "%d", u16_value);
                    cJSON_AddStringToObject (px_param_node, "value", stri_value);
                    break;
                }

                case BASE_TYPE_int16_t:
                {
                    int16_t s16_value = 0;
                    s8_PARAM_Get_Int16 (enm_param_id, &s16_value);
                    sprintf (stri_value, "%d", s16_value);
                    cJSON_AddStringToObject (px_param_node, "value", stri_value);
                    break;
                }

                case BASE_TYPE_uint32_t:
                {
                    uint32_t u32_value = 0;
                    s8_PARAM_Get_Uint32 (enm_param_id, &u32_value);
                    sprintf (stri_value, "%d", u32_value);
                    cJSON_AddStringToObject (px_param_node, "value", stri_value);
                    break;
                }

                case BASE_TYPE_int32_t:
                {
                    int32_t s32_value = 0;
                    s8_PARAM_Get_Int32 (enm_param_id, &s32_value);
                    sprintf (stri_value, "%d", s32_value);
                    cJSON_AddStringToObject (px_param_node, "value", stri_value);
                    break;
                }

                case BASE_TYPE_string:
                {
                    char * pstri_value = "";
                    s8_PARAM_Get_String (enm_param_id, &pstri_value);
                    cJSON_AddStringToObject (px_param_node, "value", pstri_value);
                    free (pstri_value);
                    break;
                }

                case BASE_TYPE_blob:
                {
                    uint8_t * pu8_value;
                    uint16_t u16_len = 0;
                    char * pstri_hex;
                    s8_PARAM_Get_Blob (enm_param_id, &pu8_value, &u16_len);
                    v_MQTTMN_Data2Hex (pu8_value, u16_len, &pstri_hex);
                    cJSON_AddStringToObject (px_param_node, "value", pstri_hex);
                    free (pu8_value);
                    free (pstri_hex);
                    break;
                }

                default:
                {
                    LOGE ("Unsupported type %d", enm_type);
                    break;
                }
            }
        }
    }

    /* Publish the response */
    char * pstri_response = cJSON_Print (px_response_root);
    cJSON_Delete (px_response_root);
    if (pstri_response != NULL)
    {
        v_MQTT_Set_Publish_Topic (g_x_mqtt, MQTT_S2M_RESPONSE, pstru_session->stri_response_topic);
        enm_MQTT_Publish (g_x_mqtt, MQTT_S2M_RESPONSE, pstri_response, 0);
        free (pstri_response);
        return MQTTMN_OK;
    }

    LOGE ("Failed to construct command paramReadResponse");
    return MQTTMN_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a paramWriteResponse command
**
** @details
**      This command is used to respond to a paramWriteRequest command.
**      Extra command data:
**          "status":"<commandStatus>"
**
** @param [in]
**      pstru_session: the session to send the command
**
** @param [in]
**      pstri_status: Command status
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**      @arg    STATUS_ERR_NOT_SUPPORTED
**      @arg    STATUS_ERR_INVALID_DATA
**      @arg    STATUS_ERR_BUSY
**      @arg    STATUS_ERR_STATE_NOT_ALLOWED
**      @arg    STATUS_ERR_INVALID_ACCESS
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MQTTMN_Send_paramWriteResponse (MQTTMN_session_t * pstru_session, const char * pstri_status)
{
    /* Construct the response */
    cJSON * px_response_root = cJSON_CreateObject ();
    cJSON_AddStringToObject (px_response_root, JSON_KEY_CMD, "paramWriteResponse");
    cJSON_AddNumberToObject (px_response_root, JSON_KEY_EID, pstru_session->u32_request_eid);
    cJSON_AddStringToObject (px_response_root, "status", pstri_status);

    /* Publish the response */
    char * pstri_response = cJSON_Print (px_response_root);
    cJSON_Delete (px_response_root);
    if (pstri_response != NULL)
    {
        v_MQTT_Set_Publish_Topic (g_x_mqtt, MQTT_S2M_RESPONSE, pstru_session->stri_response_topic);
        enm_MQTT_Publish (g_x_mqtt, MQTT_S2M_RESPONSE, pstri_response, 0);
        free (pstri_response);
        return MQTTMN_OK;
    }

    LOGE ("Failed to construct command paramWriteResponse");
    return MQTTMN_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a fileListReadResponse command
**
** @details
**      This command is used to respond to a fileListReadRequest command. The requested Rotimatic node(s) shall return
**      a list of names of all files stored in its root directory
**      Extra command data:
**          "status":"<commandStatus>"
**          "files":[ "<fileName1>", "<fileName2>", "<fileName3>", ... ]
**
** @param [in]
**      pstru_session: the session to send the command
**
** @param [in]
**      pstri_status: Command status
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**      @arg    STATUS_ERR_NOT_SUPPORTED
**      @arg    STATUS_ERR_INVALID_DATA
**      @arg    STATUS_ERR_BUSY
**      @arg    STATUS_ERR_STATE_NOT_ALLOWED
**      @arg    STATUS_ERR_INVALID_ACCESS
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MQTTMN_Send_fileListReadResponse (MQTTMN_session_t * pstru_session, const char * pstri_status)
{
    bool b_success = (strcmp (pstri_status, STATUS_OK) == 0);

    /* Open the directory on filesystem containing user files */
    lfs2_dir_t x_dir;
    if (b_success)
    {
        if (lfs2_dir_open (g_px_lfs2, &x_dir, LFS_MOUNT_POINT) < 0)
        {
            LOGE ("Failed to open directory containing user files");
            pstri_status = STATUS_ERR_INVALID_ACCESS;
            b_success = false;
        }
    }

    /* Construct the response */
    cJSON * px_response_root = cJSON_CreateObject ();
    cJSON_AddStringToObject (px_response_root, JSON_KEY_CMD, "fileListReadResponse");
    cJSON_AddNumberToObject (px_response_root, JSON_KEY_EID, pstru_session->u32_request_eid);
    cJSON_AddStringToObject (px_response_root, "status", pstri_status);
    if (b_success)
    {
        /* Array of file names */
        cJSON * px_filename_array = cJSON_CreateArray ();
        cJSON_AddItemToObject (px_response_root, "files", px_filename_array);
        struct lfs2_info stru_file_info;
        while (lfs2_dir_read (g_px_lfs2, &x_dir, &stru_file_info) > 0)
        {
            if (stru_file_info.type == LFS2_TYPE_REG)
            {
                cJSON_AddItemToArray (px_filename_array, cJSON_CreateString (stru_file_info.name));
            }
        }
        lfs2_dir_close (g_px_lfs2, &x_dir);
    }

    /* Publish the response */
    char * pstri_response = cJSON_Print (px_response_root);
    cJSON_Delete (px_response_root);
    if (pstri_response != NULL)
    {
        v_MQTT_Set_Publish_Topic (g_x_mqtt, MQTT_S2M_RESPONSE, pstru_session->stri_response_topic);
        enm_MQTT_Publish (g_x_mqtt, MQTT_S2M_RESPONSE, pstri_response, 0);
        free (pstri_response);
        return MQTTMN_OK;
    }

    LOGE ("Failed to construct command fileListReadResponse");
    return MQTTMN_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a fileUploadWriteResponse command
**
** @details
**      This command is used to respond to a fileUploadWriteRequest command. If the file can be uploaded, the requested
**      Rotimatic node(s) shall respond with status of ok. Upon receiving a response with ok status, the back-office
**      node shall send content of the file via #/data channel. Rotimatic node(s) shall report status of the file
**      uploading via statusNotify command.
**      Extra command data:
**          "status":"<commandStatus>"
**
** @param [in]
**      pstru_session: the session to send the command
**
** @param [in]
**      pstri_status: Command status
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**      @arg    STATUS_ERR_NOT_SUPPORTED
**      @arg    STATUS_ERR_INVALID_DATA
**      @arg    STATUS_ERR_BUSY
**      @arg    STATUS_ERR_STATE_NOT_ALLOWED
**      @arg    STATUS_ERR_INVALID_ACCESS
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MQTTMN_Send_fileUploadWriteResponse (MQTTMN_session_t * pstru_session, const char * pstri_status)
{
    /* Construct the response */
    cJSON * px_response_root = cJSON_CreateObject ();
    cJSON_AddStringToObject (px_response_root, JSON_KEY_CMD, "fileUploadWriteResponse");
    cJSON_AddNumberToObject (px_response_root, JSON_KEY_EID, pstru_session->u32_request_eid);
    cJSON_AddStringToObject (px_response_root, "status", pstri_status);

    /* Publish the response */
    char * pstri_response = cJSON_Print (px_response_root);
    cJSON_Delete (px_response_root);
    if (pstri_response != NULL)
    {
        v_MQTT_Set_Publish_Topic (g_x_mqtt, MQTT_S2M_RESPONSE, pstru_session->stri_response_topic);
        enm_MQTT_Publish (g_x_mqtt, MQTT_S2M_RESPONSE, pstri_response, 0);
        free (pstri_response);
        return MQTTMN_OK;
    }

    LOGE ("Failed to construct command fileUploadWriteResponse");
    return MQTTMN_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a fileDownloadReadResponse command
**
** @details
**      This command is used to respond to a fileDownloadReadRequest command. If the specified file exists and can be
**      downloaded, the requested Rotimatic node(s) shall respond with status of ok. Then the content of the file shall
**      be sent via #/data channel to the requesting back-office node. When downloading is done, Rotimatic node(s) shall
**      report downloading status via statusNotify command.
**      Extra command data:
**          "status":"<commandStatus>"
**          "size":<fileSize>
**          "checksum":<fileChecksum>
**
** @param [in]
**      pstru_session: the session to send the command
**
** @param [in]
**      pstri_status: Command status
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**      @arg    STATUS_ERR_NOT_SUPPORTED
**      @arg    STATUS_ERR_INVALID_DATA
**      @arg    STATUS_ERR_BUSY
**      @arg    STATUS_ERR_STATE_NOT_ALLOWED
**      @arg    STATUS_ERR_INVALID_ACCESS
**
** @param [in]
**      u32_file_size: size in bytes of the specified file. This is only used if pstri_status is STATUS_OK
**
** @param [in]
**      u32_checksum: checksum CRC-CCITT-32 of the specified file. This is only used if pstri_status is STATUS_OK
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MQTTMN_Send_fileDownloadReadResponse (MQTTMN_session_t * pstru_session, const char * pstri_status,
                                                       uint32_t u32_file_size, uint32_t u32_checksum)
{
    bool b_success = (strcmp (pstri_status, STATUS_OK) == 0);

    /* Construct the response */
    cJSON * px_response_root = cJSON_CreateObject ();
    cJSON_AddStringToObject (px_response_root, JSON_KEY_CMD, "fileDownloadReadResponse");
    cJSON_AddNumberToObject (px_response_root, JSON_KEY_EID, pstru_session->u32_request_eid);
    cJSON_AddStringToObject (px_response_root, "status", pstri_status);
    if (b_success)
    {
        cJSON_AddNumberToObject (px_response_root, "size", u32_file_size);
        cJSON_AddNumberToObject (px_response_root, "checksum", u32_checksum);
    }

    /* Publish the response */
    char * pstri_response = cJSON_Print (px_response_root);
    cJSON_Delete (px_response_root);
    if (pstri_response != NULL)
    {
        v_MQTT_Set_Publish_Topic (g_x_mqtt, MQTT_S2M_RESPONSE, pstru_session->stri_response_topic);
        enm_MQTT_Publish (g_x_mqtt, MQTT_S2M_RESPONSE, pstri_response, 0);
        free (pstri_response);
        return MQTTMN_OK;
    }

    LOGE ("Failed to construct command fileDownloadReadResponse");
    return MQTTMN_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a fileDeleteWriteResponse command
**
** @details
**      This command is used to respond to a fileDeleteWriteRequest command. If the specified file exists and can be
**      deleted, the requested Rotimatic node(s) shall delete it and respond with status of ok. Otherwise, an error
**      will be reported.
**      Extra command data:
**          "status":"<commandStatus>"
**
** @param [in]
**      pstru_session: the session to send the command
**
** @param [in]
**      pstri_status: Command status
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**      @arg    STATUS_ERR_NOT_SUPPORTED
**      @arg    STATUS_ERR_INVALID_DATA
**      @arg    STATUS_ERR_BUSY
**      @arg    STATUS_ERR_STATE_NOT_ALLOWED
**      @arg    STATUS_ERR_INVALID_ACCESS
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MQTTMN_Send_fileDeleteWriteResponse (MQTTMN_session_t * pstru_session, const char * pstri_status)
{
    /* Construct the response */
    cJSON * px_response_root = cJSON_CreateObject ();
    cJSON_AddStringToObject (px_response_root, JSON_KEY_CMD, "fileDeleteWriteResponse");
    cJSON_AddNumberToObject (px_response_root, JSON_KEY_EID, pstru_session->u32_request_eid);
    cJSON_AddStringToObject (px_response_root, "status", pstri_status);

    /* Publish the response */
    char * pstri_response = cJSON_Print (px_response_root);
    cJSON_Delete (px_response_root);
    if (pstri_response != NULL)
    {
        v_MQTT_Set_Publish_Topic (g_x_mqtt, MQTT_S2M_RESPONSE, pstru_session->stri_response_topic);
        enm_MQTT_Publish (g_x_mqtt, MQTT_S2M_RESPONSE, pstri_response, 0);
        free (pstri_response);
        return MQTTMN_OK;
    }

    LOGE ("Failed to construct command fileDeleteWriteResponse");
    return MQTTMN_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a fileRunWriteResponse command
**
** @details
**      This command is used to respond to a fileRunWriteRequest command. If the specified script exists and can be
**      executed, the requested Rotimatic node(s) shall respond with status of ok. Then the specified file shall be
**      executed.
**      Extra command data:
**          "status":"<commandStatus>"
**
** @param [in]
**      pstru_session: the session to send the command
**
** @param [in]
**      pstri_status: Command status
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**      @arg    STATUS_ERR_NOT_SUPPORTED
**      @arg    STATUS_ERR_INVALID_DATA
**      @arg    STATUS_ERR_BUSY
**      @arg    STATUS_ERR_STATE_NOT_ALLOWED
**      @arg    STATUS_ERR_INVALID_ACCESS
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MQTTMN_Send_fileRunWriteResponse (MQTTMN_session_t * pstru_session, const char * pstri_status)
{
    /* Construct the response */
    cJSON * px_response_root = cJSON_CreateObject ();
    cJSON_AddStringToObject (px_response_root, JSON_KEY_CMD, "fileRunWriteResponse");
    cJSON_AddNumberToObject (px_response_root, JSON_KEY_EID, pstru_session->u32_request_eid);
    cJSON_AddStringToObject (px_response_root, "status", pstri_status);

    /* Publish the response */
    char * pstri_response = cJSON_Print (px_response_root);
    cJSON_Delete (px_response_root);
    if (pstri_response != NULL)
    {
        v_MQTT_Set_Publish_Topic (g_x_mqtt, MQTT_S2M_RESPONSE, pstru_session->stri_response_topic);
        enm_MQTT_Publish (g_x_mqtt, MQTT_S2M_RESPONSE, pstri_response, 0);
        free (pstri_response);
        return MQTTMN_OK;
    }

    LOGE ("Failed to construct command fileRunWriteResponse");
    return MQTTMN_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a otaUpdateWriteResponse command
**
** @details
**      This command is used to respond to an otaUpdateWriteRequest command
**      Extra command data:
**          "status":"<commandStatus>"
**
** @param [in]
**      pstru_session: the session to send the command
**
** @param [in]
**      pstri_status: Command status
**      @arg    STATUS_OK
**      @arg    STATUS_ERR
**      @arg    STATUS_ERR_NOT_SUPPORTED
**      @arg    STATUS_ERR_INVALID_DATA
**      @arg    STATUS_ERR_BUSY
**      @arg    STATUS_ERR_STATE_NOT_ALLOWED
**      @arg    STATUS_ERR_INVALID_ACCESS
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MQTTMN_Send_otaUpdateWriteResponse (MQTTMN_session_t * pstru_session, const char * pstri_status)
{
    /* Construct the response */
    cJSON * px_response_root = cJSON_CreateObject ();
    cJSON_AddStringToObject (px_response_root, JSON_KEY_CMD, "otaUpdateWriteResponse");
    cJSON_AddNumberToObject (px_response_root, JSON_KEY_EID, pstru_session->u32_request_eid);
    cJSON_AddStringToObject (px_response_root, "status", pstri_status);

    /* Publish the response */
    char * pstri_response = cJSON_Print (px_response_root);
    cJSON_Delete (px_response_root);
    if (pstri_response != NULL)
    {
        v_MQTT_Set_Publish_Topic (g_x_mqtt, MQTT_S2M_RESPONSE, pstru_session->stri_response_topic);
        enm_MQTT_Publish (g_x_mqtt, MQTT_S2M_RESPONSE, pstri_response, 0);
        free (pstri_response);
        return MQTTMN_OK;
    }

    LOGE ("Failed to construct command otaUpdateWriteResponse");
    return MQTTMN_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Publishes content of a file via MQTT_PUB_DOWNLOAD topic
**
** @return
**      @arg    MQTTMN_OK
**      @arg    MQTTMN_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MQTTMN_Publish_Downloading_File (void)
{
    LOGI ("Sending content of file %s ...", g_stri_download_file);

    /* Check if a file is being downloaded (its file path is not an empty string) */
    if (g_stri_download_file[0] == 0)
    {
        ESP_LOGW (TAG, "No file is being requested for downloading");
        return MQTTMN_ERR;
    }

    /* Get file size */
    struct lfs2_info stru_file_info;
    if (lfs2_stat (g_px_lfs2, g_stri_download_file, &stru_file_info) < 0)
    {
        LOGE ("Failed to get information of file %s", g_stri_download_file);
        s8_MQTTMN_Send_statusNotify (NOTIFY_FILE_DOWNLOAD_STATUS, STATUS_ERR, "Failed to get file information");
        g_stri_download_file[0] = 0;
        return MQTTMN_ERR;
    }
    uint32_t u32_file_size = stru_file_info.size;

    /* Allocate buffer for reading file data */
    void * pv_data = malloc (MAX_DOWNLOAD_CHUNK_LEN);
    if (pv_data == NULL)
    {
        LOGE ("Failed to allocate buffer for reading file data");
        s8_MQTTMN_Send_statusNotify (NOTIFY_FILE_DOWNLOAD_STATUS, STATUS_ERR, "Failed to allocate memory for file data");
        g_stri_download_file[0] = 0;
        return MQTTMN_ERR;
    }

    /* Open the file for reading */
    lfs2_file_t x_file;
    if (lfs2_file_open (g_px_lfs2, &x_file, g_stri_download_file, LFS2_O_RDONLY) < 0)
    {
        LOGE ("Failed to open file %s for reading", g_stri_download_file);
        free (pv_data);
        s8_MQTTMN_Send_statusNotify (NOTIFY_FILE_DOWNLOAD_STATUS, STATUS_ERR, "Failed to open file for reading");
        g_stri_download_file[0] = 0;
        return MQTTMN_ERR;
    }

    /* Read and publish data of the file */
    size_t x_num_read = 0;
    uint32_t u32_tx_count = 0;
    do
    {
        x_num_read = lfs2_file_read (g_px_lfs2, &x_file, pv_data, MAX_DOWNLOAD_CHUNK_LEN);
        u32_tx_count += x_num_read;
        if (x_num_read > 0)
        {
            if (enm_MQTT_Publish (g_x_mqtt, MQTT_S2M_DATA, pv_data, x_num_read) != MQTT_OK)
            {
                LOGE ("Failed to publish file data to the master");
                lfs2_file_close (g_px_lfs2, &x_file);
                free (pv_data);
                s8_MQTTMN_Send_statusNotify (NOTIFY_FILE_DOWNLOAD_STATUS, STATUS_ERR,
                                             "Failed to publish file data");
                g_stri_download_file[0] = 0;
                return MQTTMN_ERR;
            }

            /* Display progress every 20% of the file has been sent */
            if (u32_tx_count % (u32_file_size / 5) < MAX_DOWNLOAD_CHUNK_LEN)
            {
                LOGI ("%d bytes sent", u32_tx_count);
            }
        }
    }
    while (x_num_read == MAX_DOWNLOAD_CHUNK_LEN);

    /* Downloading done */
    lfs2_file_close (g_px_lfs2, &x_file);
    free (pv_data);
    LOGI ("%d bytes of file %s has been sent successfully", u32_tx_count, g_stri_download_file);
    s8_MQTTMN_Send_statusNotify (NOTIFY_FILE_DOWNLOAD_STATUS, STATUS_OK, "");
    g_stri_download_file[0] = 0;
    return MQTTMN_OK;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
