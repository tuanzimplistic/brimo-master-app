/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : rx_messages.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 May 24
**  @brief      : This file contains handlers for received commands (request, post, and data messages).
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

#include "esp_system.h"                 /* Use esp_restart() */
#include "app_ota_mngr.h"               /* Use OTA update manager */
#include "srvc_micropy.h"               /* Use MicroPython service */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

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
**      Handler of scanPost command
**
** @details
**      A back-office node uses this command to check if a Rotimatic node is present or alive.
**      Extra command data: none
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_scanPost_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    /* Respond with scanNotify command */
    s8_MQTTMN_Send_scanNotify ();
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of devResetPost command
**
** @details
**      This command can be used to request one or many Rotimatic nodes to do a self-restart.
**      Extra command data: none
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_devResetPost_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    LOGI ("Restarting ESP32...");
    esp_restart ();
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of webReplRunPost command
**
** @details
**      This command is used to start WebREPL interface
**      Extra command data: none
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_webReplRunPost_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    /* Start WebREPL */
    s8_MP_Run_WebRepl ();
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of otaUpdateCancelPost command
**
** @details
**      This command is used to cancel an ongoing over-the-air update process
**      Extra command data: none
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_otaUpdateCancelPost_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    /* Request OTA manager to cancel ongoing OTA update process (if any) */
    s8_OTAMN_Cancel ();
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of paramReadRequest command
**
** @details
**      This command is used to get value of Rotimatic’s non-volatile settings using their parameter unique codes (PUC)
**      Extra command data:
**          "pucs":[<puc1>, <puc2>, <puc3>, ...]
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_paramReadRequest_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    cJSON *         px_puc_array = NULL;
    uint16_t *      pu16_puc_list = NULL;
    uint8_t         u8_num_pucs = 0;
    const char *    pstri_status = STATUS_OK;
    bool            b_success = true;

    /* List of PUCs to read */
    px_puc_array = cJSON_GetObjectItem (px_json_root, "pucs");
    if (px_puc_array == NULL)
    {
        LOGE ("Invalid request command received: No \"pucs\" key");
        pstri_status = STATUS_ERR_INVALID_DATA;
        b_success = false;
    }

    /* Allocate buffer to store requested PUC list */
    if (b_success)
    {
        u8_num_pucs = (uint8_t)cJSON_GetArraySize (px_puc_array);
        pu16_puc_list = (uint16_t *)calloc (u8_num_pucs, sizeof (uint16_t));
        if (pu16_puc_list == NULL)
        {
            LOGE ("Failed to allocate buffer to store requested PUC list");
            pstri_status = STATUS_ERR;
            b_success = false;
        }
    }

    /* Get list of requested PUCs */
    if (b_success)
    {
        for (uint8_t u8_idx = 0; u8_idx < u8_num_pucs; u8_idx++)
        {
            cJSON * px_puc_item = cJSON_GetArrayItem (px_puc_array, u8_idx);
            pu16_puc_list[u8_idx] = px_puc_item->valueint;
        }
    }

    /* Publish the response */
    s8_MQTTMN_Send_paramReadResponse (pstru_session, pstri_status, pu16_puc_list, u8_num_pucs);

    /* Clean up */
    if (pu16_puc_list != NULL)
    {
        free (pu16_puc_list);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of paramWriteRequest command
**
** @details
**      This command is used to change value of Rotimatic’s non-volatile settings using their parameter unique codes
**      Extra command data:
**          "parameters":[ { "puc":<puc1>, "value":"<value1>"}, {"puc":<puc2>, "value":"<value2>"}, ... ]
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_paramWriteRequest_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    cJSON *         px_json_node = NULL;
    const char *    pstri_status = STATUS_OK;
    cJSON *         px_param_array = NULL;
    uint8_t         u8_num_params = 0;

    /* Get array of parameters to write and its size */
    px_param_array = cJSON_GetObjectItem (px_json_root, "parameters");
    if (px_param_array == NULL)
    {
        LOGE ("Invalid request command received: No \"parameters\" key");
        pstri_status = STATUS_ERR_INVALID_DATA;
    }
    else
    {
        u8_num_params = (uint8_t)cJSON_GetArraySize (px_param_array);
    }

    /* Parse and set value of all requested parameters */
    for (uint8_t u8_idx = 0; u8_idx < u8_num_params; u8_idx++)
    {
        /* Get parameter item */
        cJSON * px_param_item = cJSON_GetArrayItem (px_param_array, u8_idx);
        if (px_param_item == NULL)
        {
            LOGE ("Failed to parse parameter Json node");
            pstri_status = STATUS_ERR_INVALID_DATA;
            continue;
        }

        /* PUC */
        px_json_node = cJSON_GetObjectItem (px_param_item, "puc");
        if (px_json_node == NULL)
        {
            LOGE ("Invalid request command received: No \"puc\" key");
            pstri_status = STATUS_ERR_INVALID_DATA;
            continue;
        }
        uint16_t u16_puc = px_json_node->valueint;

        /* Parameter value */
        px_json_node = cJSON_GetObjectItem (px_param_item, "value");
        if (px_json_node == NULL)
        {
            LOGE ("Invalid request command received: No \"value\" key");
            pstri_status = STATUS_ERR_INVALID_DATA;
            continue;
        }
        char * pstri_value = px_json_node->valuestring;

        /* Get index of the parameter */
        PARAM_id_t enm_param_id;
        if (s8_PARAM_Convert_PUC_To_ID (u16_puc, &enm_param_id) != PARAM_OK)
        {
            LOGW ("Parameter with PUC 0x%02X is not available", u16_puc);
            pstri_status = STATUS_ERR_INVALID_DATA;
            continue;
        }

        /* Get data type of the parameter, Json does not support numbers exceeding 32-bit value */
        PARAM_base_type_t enm_type;
        s8_PARAM_Get_Type (enm_param_id, &enm_type);
        if ((enm_type == BASE_TYPE_uint64_t) || (enm_type == BASE_TYPE_int64_t))
        {
            LOGW ("Data type of parameter with PUC 0x%02X is not supported", u16_puc);
            pstri_status = STATUS_ERR_INVALID_DATA;
            continue;
        }

        /* Change value of the corresponding parameter */
        int32_t s32_value;
        switch (enm_type)
        {
            case BASE_TYPE_uint8_t:
            {
                sscanf (pstri_value, "%d", &s32_value);
                s8_PARAM_Set_Uint8 (enm_param_id, s32_value);
                break;
            }

            case BASE_TYPE_int8_t:
            {
                sscanf (pstri_value, "%d", &s32_value);
                s8_PARAM_Set_Int8 (enm_param_id, s32_value);
                break;
            }

            case BASE_TYPE_uint16_t:
            {
                sscanf (pstri_value, "%d", &s32_value);
                s8_PARAM_Set_Uint16 (enm_param_id, s32_value);
                break;
            }

            case BASE_TYPE_int16_t:
            {
                sscanf (pstri_value, "%d", &s32_value);
                s8_PARAM_Set_Int16 (enm_param_id, s32_value);
                break;
            }

            case BASE_TYPE_uint32_t:
            {
                sscanf (pstri_value, "%d", &s32_value);
                s8_PARAM_Set_Uint32 (enm_param_id, s32_value);
                break;
            }

            case BASE_TYPE_int32_t:
            {
                sscanf (pstri_value, "%d", &s32_value);
                s8_PARAM_Set_Int32 (enm_param_id, s32_value);
                break;
            }

            case BASE_TYPE_string:
            {
                s8_PARAM_Set_String (enm_param_id, pstri_value);
                break;
            }

            case BASE_TYPE_blob:
            {
                uint8_t * pu8_data = NULL;
                uint8_t u8_len = 0;
                v_MQTTMN_Hex2Data (pstri_value, &pu8_data, &u8_len);
                if (pu8_data != NULL)
                {
                    s8_PARAM_Set_Blob (enm_param_id, pu8_data, u8_len);
                    free (pu8_data);
                }
                break;
            }

            default:
            {
                LOGE ("Unsupported type %d", enm_type);
                break;
            }
        }
    }

    /* Publish the response */
    s8_MQTTMN_Send_paramWriteResponse (pstru_session, pstri_status);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of fileListReadRequest command
**
** @details
**      This command is used to get list of all files available in the root directory of the requested Rotimatic node(s)
**      Extra command data: none
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_fileListReadRequest_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    /* Publish the response */
    s8_MQTTMN_Send_fileListReadResponse (pstru_session, STATUS_OK);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of fileUploadWriteRequest command
**
** @details
**      This command is used to start uploading a file to filesystem of the requested Rotimatic node(s). Content of the
**      file shall be sent through data messages
**      Extra command data:
**          "file":"<filePathName>"
**          "size":<fileSize>
**          "checksum":"<fileChecksum>"
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_fileUploadWriteRequest_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    cJSON *         px_json_node = NULL;
    char *          pstri_file_name = NULL;
    char            stri_file_path [MAX_FILE_PATH_LEN];
    uint32_t        u32_file_size = 0;
    char *          pstri_file_checksum = NULL;
    const char *    pstri_status = STATUS_OK;
    bool            b_success = true;

    /* File name */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, "file");
        if (px_json_node == NULL)
        {
            LOGE ("Invalid request command received: No \"file\" key");
            pstri_status = STATUS_ERR_INVALID_DATA;
            b_success = false;
        }
        else
        {
            pstri_file_name = px_json_node->valuestring;
            if (snprintf (stri_file_path, sizeof (stri_file_path), "%s/%s", LFS_MOUNT_POINT, pstri_file_name) < 0)
            {
                LOGE ("File name %s is too long", pstri_file_name);
                pstri_status = STATUS_ERR_INVALID_DATA;
                b_success = false;
            }
        }
    }

    /* File size */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, "size");
        if (px_json_node == NULL)
        {
            LOGE ("Invalid request command received: No \"size\" key");
            pstri_status = STATUS_ERR_INVALID_DATA;
            b_success = false;
        }
        else
        {
            u32_file_size = px_json_node->valueint;
            if (u32_file_size > MQTT_MAX_FILE_SIZE)
            {
                LOGE ("File size (%d bytes) is too big", u32_file_size);
                pstri_status = STATUS_ERR_INVALID_DATA;
                b_success = false;
            }
        }
    }

    /* File checksum */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, "checksum");
        if (px_json_node == NULL)
        {
            LOGE ("Invalid request command received: No \"checksum\" key");
            pstri_status = STATUS_ERR_INVALID_DATA;
            b_success = false;
        }
        else
        {
            pstri_file_checksum = px_json_node->valuestring;
            /* Currently, checksum is not used */
            (void) pstri_file_checksum;
        }
    }

    /* Check if the file already exists */
    if (b_success)
    {
        struct lfs2_info stru_file_info;
        if (lfs2_stat (g_px_lfs2, stri_file_path, &stru_file_info) >= 0)
        {
            /* File already exists */
            LOGE ("File %s already exists", pstri_file_name);
            pstri_status = STATUS_ERR_INVALID_ACCESS;
            b_success = false;
        }
    }

    /* Check if there in enough space for the file */
    if (b_success)
    {
        uint32_t u32_free_space;
        if (s8_MQTTMN_Get_Storage_Space (NULL, &u32_free_space) != MQTTMN_OK)
        {
            LOGE ("Failed to get information of LittleFS storage");
            pstri_status = STATUS_ERR;
            b_success = false;
        }
        else
        {
            if (u32_file_size > u32_free_space)
            {
                LOGE ("Not enough space in LittleFS storage (required = %d bytes, free = %d bytes)",
                          u32_file_size, u32_free_space);
                pstri_status = STATUS_ERR_INVALID_ACCESS;
                b_success = false;
            }
        }
    }

    /* Store path of the new file, its data shall be sent via unicast data channel */
    g_stri_upload_file [0] = 0;
    if (b_success)
    {
        strncpy (g_stri_upload_file, stri_file_path, sizeof (g_stri_upload_file));
        g_stri_upload_file [sizeof (g_stri_upload_file) - 1] = 0;
    }

    /* Publish the response */
    s8_MQTTMN_Send_fileUploadWriteResponse (pstru_session, pstri_status);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of fileDownloadReadRequest command
**
** @details
**      This command is used to start downloading a file in filesystem of the requested Rotimatic node(s). Content of
**      the file shall be sent through data messages
**      Extra command data:
**          "file":"<filePathName>"
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_fileDownloadReadRequest_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    cJSON *         px_json_node = NULL;
    char *          pstri_file_name = NULL;
    char            stri_file_path [MAX_FILE_PATH_LEN];
    uint32_t        u32_file_size = 0;
    uint32_t        u32_checksum = 0;
    const char *    pstri_status = STATUS_OK;
    bool            b_success = true;

    /* File name */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, "file");
        if (px_json_node == NULL)
        {
            LOGE ("Invalid request command received: No \"file\" key");
            pstri_status = STATUS_ERR_INVALID_DATA;
            b_success = false;
        }
        else
        {
            pstri_file_name = px_json_node->valuestring;
            if (snprintf (stri_file_path, sizeof (stri_file_path), "%s/%s", LFS_MOUNT_POINT, pstri_file_name) < 0)
            {
                LOGE ("File name %s is too long", pstri_file_name);
                pstri_status = STATUS_ERR_INVALID_DATA;
                b_success = false;
            }
        }
    }

    /* Get file size */
    if (b_success)
    {
        struct lfs2_info stru_file_info;
        if (lfs2_stat (g_px_lfs2, stri_file_path, &stru_file_info) < 0)
        {
            LOGE ("File %s doesn't exist or can't be read", pstri_file_name);
            pstri_status = STATUS_ERR_INVALID_ACCESS;
            b_success = false;
        }
        else
        {
            u32_file_size = stru_file_info.size;
        }
    }

    /** @todo   Calculate file checksum */
    if (b_success)
    {
        u32_checksum = 0;
    }    

    /* Store path of the file to download, its data shall be sent via unicast data channel */
    g_stri_download_file [0] = 0;
    if (b_success)
    {
        strncpy (g_stri_download_file, stri_file_path, sizeof (g_stri_download_file));
        g_stri_download_file [sizeof (g_stri_download_file) - 1] = 0;
    }
    
    /* Publish the response */
    s8_MQTTMN_Send_fileDownloadReadResponse (pstru_session, pstri_status, u32_file_size, u32_checksum);

    /* Publish content of the requested file */
    if (b_success)
    {
        xEventGroupSetBits (g_x_event_group, MQTTMN_FILE_DOWNLOAD_STARTED_EVT);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of fileDeleteWriteRequest command
**
** @details
**      This command is used to delete a file in filesystem of the requested Rotimatic node(s)
**      Extra command data:
**          "file":"<filePathName>"
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_fileDeleteWriteRequest_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    cJSON *         px_json_node = NULL;
    char *          pstri_file_name = NULL;
    char            stri_file_path [MAX_FILE_PATH_LEN];
    const char *    pstri_status = STATUS_OK;
    bool            b_success = true;

    /* File name */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, "file");
        if (px_json_node == NULL)
        {
            LOGE ("Invalid request command received: No \"file\" key");
            pstri_status = STATUS_ERR_INVALID_DATA;
            b_success = false;
        }
        else
        {
            pstri_file_name = px_json_node->valuestring;
            if (snprintf (stri_file_path, sizeof (stri_file_path), "%s/%s", LFS_MOUNT_POINT, pstri_file_name) < 0)
            {
                LOGE ("File name %s is too long", pstri_file_name);
                pstri_status = STATUS_ERR_INVALID_DATA;
                b_success = false;
            }
        }
    }

    /* Check if the file exists */
    if (b_success)
    {
        struct lfs2_info stru_file_info;
        if (lfs2_stat (g_px_lfs2, stri_file_path, &stru_file_info) < 0)
        {
            LOGE ("File %s doesn't exist", pstri_file_name);
            pstri_status = STATUS_ERR_INVALID_ACCESS;
            b_success = false;
        }
    }

    /* Delete the physical file in LittleFS */
    if (b_success)
    {
        if (lfs2_remove (g_px_lfs2, stri_file_path) < 0)
        {
            LOGE ("Failed to remove file %s", pstri_file_name);
            pstri_status = STATUS_ERR_INVALID_ACCESS;
            b_success = false;
        }
    }

    /* Publish the response */
    s8_MQTTMN_Send_fileDeleteWriteResponse (pstru_session, pstri_status);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of fileRunWriteRequest command
**
** @details
**      This command is used to execute a Python script inside file system of the requested Rotimatic nodes
**      Extra command data:
**          "file":"<filePathName>"
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_fileRunWriteRequest_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    cJSON *         px_json_node = NULL;
    char *          pstri_file_name = NULL;
    char            stri_file_path [MAX_FILE_PATH_LEN];
    const char *    pstri_status = STATUS_OK;
    bool            b_success = true;

    /* File name */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, "file");
        if (px_json_node == NULL)
        {
            LOGE ("Invalid request command received: No \"file\" key");
            pstri_status = STATUS_ERR_INVALID_DATA;
            b_success = false;
        }
        else
        {
            pstri_file_name = px_json_node->valuestring;
            if (snprintf (stri_file_path, sizeof (stri_file_path), "%s/%s", LFS_MOUNT_POINT, pstri_file_name) < 0)
            {
                LOGE ("File name %s is too long", pstri_file_name);
                pstri_status = STATUS_ERR_INVALID_DATA;
                b_success = false;
            }
        }
    }

    /* Check if the file exists */
    if (b_success)
    {
        struct lfs2_info stru_file_info;
        if (lfs2_stat (g_px_lfs2, stri_file_path, &stru_file_info) < 0)
        {
            LOGE ("File %s doesn't exist", pstri_file_name);
            pstri_status = STATUS_ERR_INVALID_ACCESS;
            b_success = false;
        }
    }

    /* Run the file */
    if (b_success)
    {
        uint16_t u16_len = strlen (pstri_file_name);

        /* Check file extension */
        if ((pstri_file_name [u16_len - 3] == '.') &&
            (pstri_file_name [u16_len - 2] == 'j') &&
            (pstri_file_name [u16_len - 1] == 's'))
        {
            /* The file is a Javascript file */
            LOGE ("Javascript file is not supported");
            pstri_status = STATUS_ERR_INVALID_ACCESS;
            b_success = false;
        }
        else if ((pstri_file_name [u16_len - 3] == '.') &&
                 (pstri_file_name [u16_len - 2] == 'p') &&
                 (pstri_file_name [u16_len - 1] == 'y'))
        {
            /* The file is a Python file */
            s8_MP_Execute_File (stri_file_path);
        }
        else
        {
            LOGE ("File %s is neither a Javascript file nor a Python script", pstri_file_name);
            pstri_status = STATUS_ERR_INVALID_ACCESS;
            b_success = false;
        }
    }

    /* Publish the response */
    s8_MQTTMN_Send_fileRunWriteResponse (pstru_session, pstri_status);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of otaUpdateWriteRequest command
**
** @details
**      This command is used to trigger over-the-air update process on the requested Rotimatic nodes
**      Extra command data:
**          "target":"<masterFw | slaveFw | file>"
**          "checkNewer":<true | false>
**          "file":"<filePathName>"
**          "url":"<downloadUrl>"
**
** @param [in]
**      pstru_session: the session through which the command was received
**
** @param [in]
**      px_json_root: cJSON object of received command
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MQTTMN_otaUpdateWriteRequest_Handler (MQTTMN_session_t * pstru_session, const cJSON * px_json_root)
{
    cJSON *         px_json_node = NULL;
    OTAMN_config_t  stru_ota_cfg;
    const char *    pstri_status = STATUS_OK;
    bool            b_success = true;

    /* Target of the update */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, "target");
        if (px_json_node == NULL)
        {
            LOGE ("Invalid request command received: No \"target\" key");
            pstri_status = STATUS_ERR_INVALID_DATA;
            b_success = false;
        }
        else
        {
            char * pstri_target = px_json_node->valuestring;
            if (strcmp ("masterFw", pstri_target) == 0)
            {
                stru_ota_cfg.enm_target = OTAMN_MASTER_FW;
            }
            else if (strcmp ("slaveFw", pstri_target) == 0)
            {
                stru_ota_cfg.enm_target = OTAMN_SLAVE_FW;
            }
            else if (strcmp ("file", pstri_target) == 0)
            {
                stru_ota_cfg.enm_target = OTAMN_MASTER_FILE;
            }
            else
            {
                LOGE ("Invalid OTA target component: %s", pstri_target);
                pstri_status = STATUS_ERR_INVALID_DATA;
                b_success = false;
            }
        }
    }

    /* Source download URL */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, "url");
        if (px_json_node == NULL)
        {
            LOGE ("Invalid request command received: No \"url\" key");
            pstri_status = STATUS_ERR_INVALID_DATA;
            b_success = false;
        }
        else
        {
            stru_ota_cfg.pstri_url = px_json_node->valuestring;
        }
    }

    /* Installation path (this field is optional for firmware update, but mandatory for file update) */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, "file");
        if (px_json_node == NULL)
        {
            if (stru_ota_cfg.enm_target == OTAMN_MASTER_FILE)
            {
                LOGE ("Invalid request command received: No \"file\" key");
                pstri_status = STATUS_ERR_INVALID_DATA;
                b_success = false;
            }
            else
            {
                stru_ota_cfg.pstri_inst_dir = "/";
            }
        }
        else
        {
            stru_ota_cfg.pstri_inst_dir = px_json_node->valuestring;
        }
    }

    /* Check if source is newer (this field is optional) */
    if (b_success)
    {
        px_json_node = cJSON_GetObjectItem (px_json_root, "checkNewer");
        stru_ota_cfg.b_check_newer = (px_json_node == NULL) ? false : cJSON_IsTrue (px_json_node);
    }

    /* Request OTA manager to start the update */
    if (b_success)
    {
        if (s8_OTAMN_Start (&stru_ota_cfg) != OTAMN_OK)
        {
            LOGE ("Failed to start OTA update");
            pstri_status = STATUS_ERR;
            b_success = false;
        }
    }

    /* Publish the response */
    s8_MQTTMN_Send_otaUpdateWriteResponse (pstru_session, pstri_status);
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
