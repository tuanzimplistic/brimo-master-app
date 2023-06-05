/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_param.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Feb 12
**  @brief      : Implementation of Srvc_Param module
**  @namespace  : PARAM
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Param
** @brief       Provides helper APIs to manipulate parameters in non-volatile storage
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_param.h"             /* Public header of this module */
#include "nvs_flash.h"              /* Use non-volatile storage component from ESP-IDF */

#include <string.h>                 /* Use strlen() */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Namespace of parameter flash */
#define PARAM_NAME_SPACE            "Params"

/** @brief  Base type of a string parameter */
#define string                      char

/** @brief  Base type of a blob parameter */
#define blob                        uint8_t

/** @brief  Specify if a data type is an array type or not */
#define ARRAY_SYMBOL_uint8_t
#define ARRAY_SYMBOL_int8_t
#define ARRAY_SYMBOL_uint16_t
#define ARRAY_SYMBOL_int16_t
#define ARRAY_SYMBOL_uint32_t
#define ARRAY_SYMBOL_int32_t
#define ARRAY_SYMBOL_uint64_t
#define ARRAY_SYMBOL_int64_t
#define ARRAY_SYMBOL_string                 []
#define ARRAY_SYMBOL_blob                   []

/** @brief  Macro to expand an entry in param table as constant variable definition of parameter's default value */
#define PARAM_EXPAND_AS_DEFAULT_VALUE_DEFINITION(PARAM_ID, PUC, TYPE, MIN, MAX, ...)    \
    const TYPE PARAM_ID##_DEFAULT ARRAY_SYMBOL_##TYPE __attribute__((aligned (8))) = __VA_ARGS__;

/** @brief  Structure type to manage a parameter information */
typedef struct
{
    /** @brief  Parameter key */
    const char *            pstri_key;

    /** @brief  Parameter unique code */
    const uint16_t          u16_puc;

    /** @brief  Parameter base type */
    PARAM_base_type_t       enm_base_type;

    /** @brief  Min and max value of the parameter */
    const union
    {
        uint8_t             x_uint8_t;
        int8_t              x_int8_t;
        uint16_t            x_uint16_t;
        int16_t             x_int16_t;
        uint32_t            x_uint32_t;
        int32_t             x_int32_t;
        uint64_t            x_uint64_t;
        int64_t             x_int64_t;
        uint16_t            x_string;       //!< Length in bytes of string data
        uint16_t            x_blob;         //!< Length in bytes of blob data

    } un_min, un_max;

    /** @brief  Pointer to the buffer containing default value of the parameter */
    const void *            pv_def_data;

    /** @brief  Length in bytes of the default value */
    const uint16_t          u16_def_data_len;

} PARAM_info_t;

/** @brief  Macros to expand an entry in param table as initialization value for parameter's information structure */
#define PARAM_EXPAND_AS_INFO_STRUCT_INIT(PARAM_ID, PUC, TYPE, MIN, MAX, ...)    \
{                                                                               \
    .pstri_key              = #PUC,                                             \
    .u16_puc                = PUC,                                              \
    .enm_base_type          = BASE_TYPE_##TYPE,                                 \
    .un_min.x_##TYPE        = MIN,                                              \
    .un_max.x_##TYPE        = MAX,                                              \
    .pv_def_data            = (void *)&PARAM_ID##_DEFAULT,                      \
    .u16_def_data_len       = sizeof (PARAM_ID##_DEFAULT)                       \
},

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Param";

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Parameter handle */
static nvs_handle_t g_x_handle;

/** @brief  Default values of all parameters */
static PARAM_TABLE (PARAM_EXPAND_AS_DEFAULT_VALUE_DEFINITION);

/** @brief  Information of all parameters */
static PARAM_info_t g_astru_params [PARAM_NUM_PARAMS] = { PARAM_TABLE (PARAM_EXPAND_AS_INFO_STRUCT_INIT) };

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
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
**      Initializes Srvc_Param module
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Init (void)
{
    /* Do nothing if this module has been initialized */
    if (g_b_initialized)
    {
        return PARAM_OK;
    }

    LOGD ("Initializing Srvc_Param module");

    /* Initialize non-volatile storage */
    esp_err_t x_ret = nvs_flash_init ();
    if ((x_ret == ESP_ERR_NVS_NO_FREE_PAGES) || (x_ret == ESP_ERR_NVS_NEW_VERSION_FOUND))
    {
        /* Erase the default NVS partition */
        ESP_ERROR_CHECK (nvs_flash_erase ());
        x_ret = nvs_flash_init ();
    }
    ESP_ERROR_CHECK (x_ret);

    /* Get handle for parameter storage */
    esp_err_t x_err = nvs_open (PARAM_NAME_SPACE, NVS_READWRITE, &g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Error (%s) opening NVS handle of namespace %s", esp_err_to_name (x_err), PARAM_NAME_SPACE);
        return PARAM_ERR;
    }

    /* If any parameters are not available or not within min and max range, create and initialize them */
    for (uint16_t u16_id = 0; u16_id < PARAM_NUM_PARAMS; u16_id++)
    {
        PARAM_info_t * pstru_param = &g_astru_params[u16_id];
        switch (pstru_param->enm_base_type)
        {
            case BASE_TYPE_uint8_t:
            {
                uint8_t u8_val;
                if ((nvs_get_u8 (g_x_handle, pstru_param->pstri_key, &u8_val) != ESP_OK) ||
                    (((pstru_param->un_min.x_uint8_t != 0) || (pstru_param->un_max.x_uint8_t != 0)) &&
                     ((u8_val < pstru_param->un_min.x_uint8_t) || (u8_val > pstru_param->un_max.x_uint8_t))))
                {
                    nvs_set_u8 (g_x_handle, pstru_param->pstri_key, *((uint8_t *)pstru_param->pv_def_data));
                    LOGW ("Parameter PUC = 0x%04X has been reset to default value = %u",
                          pstru_param->u16_puc, *((uint8_t *)pstru_param->pv_def_data));
                }
                break;
            }

            case BASE_TYPE_int8_t:
            {
                int8_t s8_val;
                if ((nvs_get_i8 (g_x_handle, pstru_param->pstri_key, &s8_val) != ESP_OK) ||
                    (((pstru_param->un_min.x_int8_t != 0) || (pstru_param->un_max.x_int8_t != 0)) &&
                     ((s8_val < pstru_param->un_min.x_int8_t) || (s8_val > pstru_param->un_max.x_int8_t))))
                {
                    nvs_set_i8 (g_x_handle, pstru_param->pstri_key, *((int8_t *)pstru_param->pv_def_data));
                    LOGW ("Parameter PUC = 0x%04X has been reset to default value = %d",
                          pstru_param->u16_puc, *((int8_t *)pstru_param->pv_def_data));
                }
                break;
            }

            case BASE_TYPE_uint16_t:
            {
                uint16_t u16_val;
                if ((nvs_get_u16 (g_x_handle, pstru_param->pstri_key, &u16_val) != ESP_OK) ||
                    (((pstru_param->un_min.x_uint16_t != 0) || (pstru_param->un_max.x_uint16_t != 0)) &&
                     ((u16_val < pstru_param->un_min.x_uint16_t) || (u16_val > pstru_param->un_max.x_uint16_t))))
                {
                    nvs_set_u16 (g_x_handle, pstru_param->pstri_key, *((uint16_t *)pstru_param->pv_def_data));
                    LOGW ("Parameter PUC = 0x%04X has been reset to default value = %u",
                          pstru_param->u16_puc, *((uint16_t *)pstru_param->pv_def_data));
                }
                break;
            }

            case BASE_TYPE_int16_t:
            {
                int16_t s16_val;
                if ((nvs_get_i16 (g_x_handle, pstru_param->pstri_key, &s16_val) != ESP_OK) ||
                    (((pstru_param->un_min.x_int16_t != 0) || (pstru_param->un_max.x_int16_t != 0)) &&
                     ((s16_val < pstru_param->un_min.x_int16_t) || (s16_val > pstru_param->un_max.x_int16_t))))
                {
                    nvs_set_i16 (g_x_handle, pstru_param->pstri_key, *((int16_t *)pstru_param->pv_def_data));
                    LOGW ("Parameter PUC = 0x%04X has been reset to default value = %d",
                          pstru_param->u16_puc, *((int16_t *)pstru_param->pv_def_data));
                }
                break;
            }

            case BASE_TYPE_uint32_t:
            {
                uint32_t u32_val;
                if ((nvs_get_u32 (g_x_handle, pstru_param->pstri_key, &u32_val) != ESP_OK) ||
                    (((pstru_param->un_min.x_uint32_t != 0) || (pstru_param->un_max.x_uint32_t != 0)) &&
                     ((u32_val < pstru_param->un_min.x_uint32_t) || (u32_val > pstru_param->un_max.x_uint32_t))))
                {
                    nvs_set_u32 (g_x_handle, pstru_param->pstri_key, *((uint32_t *)pstru_param->pv_def_data));
                    LOGW ("Parameter PUC = 0x%04X has been reset to default value = %u",
                          pstru_param->u16_puc, *((uint32_t *)pstru_param->pv_def_data));
                }
                break;
            }

            case BASE_TYPE_int32_t:
            {
                int32_t s32_val;
                if ((nvs_get_i32 (g_x_handle, pstru_param->pstri_key, &s32_val) != ESP_OK) ||
                    (((pstru_param->un_min.x_int32_t != 0) || (pstru_param->un_max.x_int32_t != 0)) &&
                     ((s32_val < pstru_param->un_min.x_int32_t) || (s32_val > pstru_param->un_max.x_int32_t))))
                {
                    nvs_set_i32 (g_x_handle, pstru_param->pstri_key, *((int32_t *)pstru_param->pv_def_data));
                    LOGW ("Parameter PUC = 0x%04X has been reset to default value = %d",
                          pstru_param->u16_puc, *((int32_t *)pstru_param->pv_def_data));
                }
                break;
            }

            case BASE_TYPE_uint64_t:
            {
                uint64_t u64_val;
                if ((nvs_get_u64 (g_x_handle, pstru_param->pstri_key, &u64_val) != ESP_OK) ||
                    (((pstru_param->un_min.x_uint64_t != 0) || (pstru_param->un_max.x_uint64_t != 0)) &&
                     ((u64_val < pstru_param->un_min.x_uint64_t) || (u64_val > pstru_param->un_max.x_uint64_t))))
                {
                    nvs_set_u64 (g_x_handle, pstru_param->pstri_key, *((uint64_t *)pstru_param->pv_def_data));
                    LOGW ("Parameter PUC = 0x%04X has been reset to default value = %llu",
                          pstru_param->u16_puc, *((uint64_t *)pstru_param->pv_def_data));
                }
                break;
            }

            case BASE_TYPE_int64_t:
            {
                int64_t s64_val;
                if ((nvs_get_i64 (g_x_handle, pstru_param->pstri_key, &s64_val) != ESP_OK) ||
                    (((pstru_param->un_min.x_int64_t != 0) || (pstru_param->un_max.x_int64_t != 0)) &&
                     ((s64_val < pstru_param->un_min.x_int64_t) || (s64_val > pstru_param->un_max.x_int64_t))))
                {
                    nvs_set_i64 (g_x_handle, pstru_param->pstri_key, *((int64_t *)pstru_param->pv_def_data));
                    LOGW ("Parameter PUC = 0x%04X has been reset to default value = %lld",
                          pstru_param->u16_puc, *((int64_t *)pstru_param->pv_def_data));
                }
                break;
            }

            case BASE_TYPE_string:
            {
                size_t x_param_size = 0;
                if ((nvs_get_str (g_x_handle, pstru_param->pstri_key, NULL, &x_param_size) != ESP_OK) ||
                    (((pstru_param->un_min.x_string != 0) || (pstru_param->un_max.x_string != 0)) &&
                     ((x_param_size < pstru_param->un_min.x_string) || (x_param_size > pstru_param->un_max.x_string))))
                {
                    nvs_set_str (g_x_handle, pstru_param->pstri_key, (const char *)pstru_param->pv_def_data);
                    LOGW ("Parameter PUC = 0x%04X has been reset to default value = %s",
                          pstru_param->u16_puc, (const char *)pstru_param->pv_def_data);
                }
                break;
            }

            case BASE_TYPE_blob:
            {
                size_t x_param_size = 0;
                if ((nvs_get_blob (g_x_handle, pstru_param->pstri_key, NULL, &x_param_size) != ESP_OK) ||
                    (((pstru_param->un_min.x_blob != 0) || (pstru_param->un_max.x_blob != 0)) &&
                     ((x_param_size < pstru_param->un_min.x_blob) || (x_param_size > pstru_param->un_max.x_blob))))
                {
                    nvs_set_blob (g_x_handle, pstru_param->pstri_key,
                                  pstru_param->pv_def_data, pstru_param->u16_def_data_len);
                    LOGW ("Parameter PUC = 0x%04X has been reset to default value", pstru_param->u16_puc);
                }
                break;
            }
        }
    }

    /* Commit any changes to non-volatile storage */
    nvs_commit (g_x_handle);

    /* Done */
    LOGD ("Initialization of Srvc_Param module is done");
    g_b_initialized = true;
    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Resets a parameter to its default value
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Reset_Default (PARAM_id_t enm_param_id)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];
    esp_err_t x_err = ESP_OK;

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS));

    /* Reset to default value */
    switch (pstru_param->enm_base_type)
    {
        case BASE_TYPE_uint8_t:
        {
            x_err = nvs_set_u8 (g_x_handle, pstru_param->pstri_key, *((uint8_t *)pstru_param->pv_def_data));
            break;
        }
        case BASE_TYPE_int8_t:
        {
            x_err = nvs_set_i8 (g_x_handle, pstru_param->pstri_key, *((int8_t *)pstru_param->pv_def_data));
            break;
        }
        case BASE_TYPE_uint16_t:
        {
            x_err = nvs_set_u16 (g_x_handle, pstru_param->pstri_key, *((uint16_t *)pstru_param->pv_def_data));
            break;
        }
        case BASE_TYPE_int16_t:
        {
            x_err = nvs_set_i16 (g_x_handle, pstru_param->pstri_key, *((int16_t *)pstru_param->pv_def_data));
            break;
        }
        case BASE_TYPE_uint32_t:
        {
            x_err = nvs_set_u32 (g_x_handle, pstru_param->pstri_key, *((uint32_t *)pstru_param->pv_def_data));
            break;
        }
        case BASE_TYPE_int32_t:
        {
            x_err = nvs_set_i32 (g_x_handle, pstru_param->pstri_key, *((int32_t *)pstru_param->pv_def_data));
            break;
        }
        case BASE_TYPE_uint64_t:
        {
            x_err = nvs_set_u64 (g_x_handle, pstru_param->pstri_key, *((uint64_t *)pstru_param->pv_def_data));
            break;
        }
        case BASE_TYPE_int64_t:
        {
            x_err = nvs_set_i64 (g_x_handle, pstru_param->pstri_key, *((int64_t *)pstru_param->pv_def_data));
            break;
        }
        case BASE_TYPE_string:
        {
            x_err = nvs_set_str (g_x_handle, pstru_param->pstri_key, (const char *)pstru_param->pv_def_data);
            break;
        }
        case BASE_TYPE_blob:
        {
            x_err = nvs_set_blob (g_x_handle, pstru_param->pstri_key, pstru_param->pv_def_data, pstru_param->u16_def_data_len);
            break;
        }
    }

    /* Check result */
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to reset param %s to default value (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Converts Param Unique Code of a parameter to parameter ID
**
** @param [in]
**      u16_param_puc: Param Unique Code of a parameter
**
** @param [out]
**      penm_param_id: Parameter ID
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Convert_PUC_To_ID (uint16_t u16_param_puc, PARAM_id_t * penm_param_id)
{
    ASSERT_PARAM (g_b_initialized);

    /* Loop through all parameters */
    for (PARAM_id_t enm_param_id = 0; enm_param_id < PARAM_NUM_PARAMS; enm_param_id++)
    {
        if (g_astru_params[enm_param_id].u16_puc == u16_param_puc)
        {
            *penm_param_id = enm_param_id;
            return PARAM_OK;
        }
    }

    return PARAM_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Converts parameter ID of a parameter to Param Unique Code
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      pu16_param_puc: Param Unique Code
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Convert_ID_To_PUC (PARAM_id_t enm_param_id, uint16_t * pu16_param_puc)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS));

    /* Get PUC of the parameter */
    *pu16_param_puc = pstru_param->u16_puc;
    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets data type of a parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      penm_type: Type of parameter's data
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_Type (PARAM_id_t enm_param_id, PARAM_base_type_t * penm_type)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (penm_type != NULL));

    /* Get data type of the parameter */
    *penm_type = pstru_param->enm_base_type;
    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Generic function to get value of a parameter
**
** @note
**      The caller MUST free the memory pointed by ppv_value after using it.
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      ppv_value: Pointer to the allocated memory containing parameter's value. The value is NULL in case of failure
**
** @param [out]
**      pu16_len: Length in bytes of the parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_Value (PARAM_id_t enm_param_id, void ** ppv_value, uint16_t * pu16_len)
{
    esp_err_t       x_err = ESP_OK;
    bool            b_success = true;
    void *          pv_param_value = NULL;
    size_t          x_param_size = 0;
    PARAM_info_t *  pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (ppv_value != NULL) && (pu16_len != NULL));

    /* Determine length in bytes of parameter value */
    switch (pstru_param->enm_base_type)
    {
        case BASE_TYPE_uint8_t:
        case BASE_TYPE_int8_t:
            x_param_size = 1;
            break;

        case BASE_TYPE_uint16_t:
        case BASE_TYPE_int16_t:
            x_param_size = 2;
            break;

        case BASE_TYPE_uint32_t:
        case BASE_TYPE_int32_t:
            x_param_size = 4;
            break;

        case BASE_TYPE_uint64_t:
        case BASE_TYPE_int64_t:
            x_param_size = 8;
            break;

        case BASE_TYPE_string:
            x_err = nvs_get_str (g_x_handle, pstru_param->pstri_key, NULL, &x_param_size);
            break;

        case BASE_TYPE_blob:
            x_err = nvs_get_blob (g_x_handle, pstru_param->pstri_key, NULL, &x_param_size);
            break;
    }

    /* Check result */
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        b_success = false;
    }

    /* Allocate dynamic memory to store parameter value */
    if (b_success)
    {
        *pu16_len = x_param_size;
        pv_param_value = malloc (x_param_size);
        if (pv_param_value == NULL)
        {
            LOGE ("Failed to allocate memory for param %s", pstru_param->pstri_key);
            b_success = false;
        }
    }

    /* Get value of the parameter */
    if (b_success)
    {
        switch (pstru_param->enm_base_type)
        {
            case BASE_TYPE_uint8_t:
                x_err = nvs_get_u8 (g_x_handle, pstru_param->pstri_key, pv_param_value);
                break;

            case BASE_TYPE_int8_t:
                x_err = nvs_get_i8 (g_x_handle, pstru_param->pstri_key, pv_param_value);
                break;

            case BASE_TYPE_uint16_t:
                x_err = nvs_get_u16 (g_x_handle, pstru_param->pstri_key, pv_param_value);
                break;

            case BASE_TYPE_int16_t:
                x_err = nvs_get_i16 (g_x_handle, pstru_param->pstri_key, pv_param_value);
                break;

            case BASE_TYPE_uint32_t:
                x_err = nvs_get_u32 (g_x_handle, pstru_param->pstri_key, pv_param_value);
                break;

            case BASE_TYPE_int32_t:
                x_err = nvs_get_i32 (g_x_handle, pstru_param->pstri_key, pv_param_value);
                break;

            case BASE_TYPE_uint64_t:
                x_err = nvs_get_u64 (g_x_handle, pstru_param->pstri_key, pv_param_value);
                break;

            case BASE_TYPE_int64_t:
                x_err = nvs_get_i64 (g_x_handle, pstru_param->pstri_key, pv_param_value);
                break;

            case BASE_TYPE_string:
                x_err = nvs_get_str (g_x_handle, pstru_param->pstri_key, pv_param_value, &x_param_size);
                break;

            case BASE_TYPE_blob:
                x_err = nvs_get_blob (g_x_handle, pstru_param->pstri_key, pv_param_value, &x_param_size);
                break;
        }

        /* Check result */
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
            b_success = false;
        }
    }

    /* Done */
    if (b_success)
    {
        *ppv_value = pv_param_value;
        return PARAM_OK;
    }
    else
    {
        /* Clean up */
        if (pv_param_value != NULL)
        {
            free (pv_param_value);
        }
        *ppv_value = NULL;
        return PARAM_ERR;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Generic function to set value of a parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [in]
**      pv_value: Buffer storing parameter's value
**
** @param [in]
**      u16_len: Length in bytes of the parameter
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_Value (PARAM_id_t enm_param_id, const void * pv_value, uint16_t u16_len)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (pv_value != NULL) && (u16_len > 0));

    /* Change parameter value */
    switch (pstru_param->enm_base_type)
    {
        case BASE_TYPE_uint8_t:
            return s8_PARAM_Set_Uint8 (enm_param_id, *((uint8_t *)pv_value));

        case BASE_TYPE_int8_t:
            return s8_PARAM_Set_Int8 (enm_param_id, *((int8_t *)pv_value));

        case BASE_TYPE_uint16_t:
            return s8_PARAM_Set_Uint16 (enm_param_id, *((uint16_t *)pv_value));

        case BASE_TYPE_int16_t:
            return s8_PARAM_Set_Int16 (enm_param_id, *((int16_t *)pv_value));

        case BASE_TYPE_uint32_t:
            return s8_PARAM_Set_Uint32 (enm_param_id, *((uint32_t *)pv_value));

        case BASE_TYPE_int32_t:
            return s8_PARAM_Set_Int32 (enm_param_id, *((int32_t *)pv_value));

        case BASE_TYPE_uint64_t:
            return s8_PARAM_Set_Uint64 (enm_param_id, *((uint64_t *)pv_value));

        case BASE_TYPE_int64_t:
            return s8_PARAM_Set_Int64 (enm_param_id, *((int64_t *)pv_value));

        case BASE_TYPE_string:
            return s8_PARAM_Set_String (enm_param_id, (char *)pv_value);

        case BASE_TYPE_blob:
            return s8_PARAM_Set_Blob (enm_param_id, pv_value, u16_len);
    }

    LOGE ("Unsupported type");
    return PARAM_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of an unmanaged string parameter
**
** @note
**      The caller MUST free the memory pointed by ppstri_value after using it.
**
** @param [in]
**      pstri_key: Key identifying the parameter
**
** @param [out]
**      ppstri_value: Pointer to the allocated memory containing parameter's value (NULL-terminated string)
**                    Value is NULL in case of failure
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_String_Unmanaged (const char * pstri_key, char ** ppstri_value)
{
    esp_err_t   x_err = ESP_OK;
    bool        b_success = true;
    char *      pstri_param_value = NULL;
    size_t      x_param_size = 0;

    ASSERT_PARAM (g_b_initialized && (pstri_key != NULL) && (ppstri_value != NULL));

    /* Get length in bytes of parameter value */
    if (b_success)
    {
        x_err = nvs_get_str (g_x_handle, pstri_key, NULL, &x_param_size);
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to access param \"%s\" (%s)", pstri_key, esp_err_to_name (x_err));
            b_success = false;
        }
    }

    /* Allocate dynamic memory to store parameter value */
    if (b_success)
    {
        pstri_param_value = malloc (x_param_size);
        if (pstri_param_value == NULL)
        {
            LOGE ("Failed to allocate memory for param %s", pstri_key);
            b_success = false;
        }
    }

    /* Get NULL-terminated string value of the parameter */
    if (b_success)
    {
        x_err = nvs_get_str (g_x_handle, pstri_key, pstri_param_value, &x_param_size);
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to access param %s (%s)", pstri_key, esp_err_to_name (x_err));
            b_success = false;
        }
    }

    /* Done */
    if (b_success)
    {
        *ppstri_value = pstri_param_value;
        return PARAM_OK;
    }
    else
    {
        /* Clean up */
        if (pstri_param_value != NULL)
        {
            free (pstri_param_value);
        }
        *ppstri_value = NULL;
        return PARAM_ERR;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of an unmanaged string parameter, if the parameter doesn't exist, it will be created
**
** @param [in]
**      pstri_key: Key identifying the parameter
**
** @param [in]
**      pstri_value: Parameter's value (NULL-terminated string)
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_String_Unmanaged (const char * pstri_key, const char * pstri_value)
{
    ASSERT_PARAM (g_b_initialized && (pstri_key != NULL) && (pstri_value != NULL));

    /* Write value to the parameter value */
    esp_err_t x_err = nvs_set_str (g_x_handle, pstri_key, pstri_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to change value of param %s (%s)", pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    /* Commit the change to non-volatile storage */
    x_err = nvs_commit (g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to commit parameter change to non-volatile storage (%s)", esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of a string parameter
**
** @note
**      The caller MUST free the memory pointed by ppstri_value after using it.
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      ppstri_value: Pointer to the allocated memory containing parameter's value (NULL-terminated string)
**                    Value is NULL in case of failure
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_String (PARAM_id_t enm_param_id, char ** ppstri_value)
{
    esp_err_t       x_err = ESP_OK;
    bool            b_success = true;
    char *          pstri_param_value = NULL;
    size_t          x_param_size = 0;
    PARAM_info_t *  pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (ppstri_value != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_string);

    /* Get length in bytes of parameter value */
    if (b_success)
    {
        x_err = nvs_get_str (g_x_handle, pstru_param->pstri_key, NULL, &x_param_size);
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
            b_success = false;
        }
    }

    /* Allocate dynamic memory to store parameter value */
    if (b_success)
    {
        pstri_param_value = malloc (x_param_size);
        if (pstri_param_value == NULL)
        {
            LOGE ("Failed to allocate memory for param %s", pstru_param->pstri_key);
            b_success = false;
        }
    }

    /* Get NULL-terminated string value of the parameter */
    if (b_success)
    {
        x_err = nvs_get_str (g_x_handle, pstru_param->pstri_key, pstri_param_value, &x_param_size);
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
            b_success = false;
        }
    }

    /* Done */
    if (b_success)
    {
        *ppstri_value = pstri_param_value;
        return PARAM_OK;
    }
    else
    {
        /* Clean up */
        if (pstri_param_value != NULL)
        {
            free (pstri_param_value);
        }
        *ppstri_value = NULL;
        return PARAM_ERR;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of a string parameter, if the parameter doesn't exist, it will be created
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [in]
**      pstri_value: Parameter's value (NULL-terminated string)
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_String (PARAM_id_t enm_param_id, const char * pstri_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (pstri_value != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_string);

    /* Check data length if required */
    if ((pstru_param->un_min.x_string != 0) || (pstru_param->un_max.x_string != 0))
    {
        uint16_t u16_len = strlen (pstri_value);
        if ((u16_len < pstru_param->un_min.x_string) || (u16_len > pstru_param->un_max.x_string))
        {
            LOGE ("Data length of param %s (%d bytes) is NOT within the allowed range",
                  pstru_param->pstri_key, u16_len);
            return PARAM_ERR;
        }
    }

    /* Write value to the parameter value */
    esp_err_t x_err = nvs_set_str (g_x_handle, pstru_param->pstri_key, pstri_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to change value of param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    /* Commit the change to non-volatile storage */
    x_err = nvs_commit (g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to commit parameter change to non-volatile storage (%s)", esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of a blob parameter (variable length binary data)
**
** @note
**      The caller MUST free the memory pointed by ppu8_value after using it.
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      ppu8_value: Pointer to the allocated memory containing parameter's value. The value is NULL in case of failure
**
** @param [out]
**      pu16_len: Length in bytes of the parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_Blob (PARAM_id_t enm_param_id, uint8_t ** ppu8_value, uint16_t * pu16_len)
{
    esp_err_t       x_err = ESP_OK;
    bool            b_success = true;
    uint8_t *       pu8_param_value = NULL;
    size_t          x_param_size = 0;
    PARAM_info_t *  pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (ppu8_value != NULL) && (pu16_len != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_blob);

    /* Get length in bytes of parameter value */
    if (b_success)
    {
        x_err = nvs_get_blob (g_x_handle, pstru_param->pstri_key, NULL, &x_param_size);
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
            b_success = false;
        }
    }

    /* Allocate dynamic memory to store parameter value */
    if (b_success)
    {
        *pu16_len = x_param_size;
        pu8_param_value = malloc (x_param_size);
        if (pu8_param_value == NULL)
        {
            LOGE ("Failed to allocate memory for param %s", pstru_param->pstri_key);
            b_success = false;
        }
    }

    /* Get blob value of the parameter */
    if (b_success)
    {
        x_err = nvs_get_blob (g_x_handle, pstru_param->pstri_key, pu8_param_value, &x_param_size);
        if (x_err != ESP_OK)
        {
            LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
            b_success = false;
        }
    }

    /* Done */
    if (b_success)
    {
        *ppu8_value = pu8_param_value;
        return PARAM_OK;
    }
    else
    {
        /* Clean up */
        if (pu8_param_value != NULL)
        {
            free (pu8_param_value);
        }
        *ppu8_value = NULL;
        return PARAM_ERR;
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of a blob parameter (variable length binary data)
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [in]
**      pv_value: Buffer storing parameter's value
**
** @param [in]
**      u16_len: Length in bytes of the parameter
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_Blob (PARAM_id_t enm_param_id, const void * pv_value, uint16_t u16_len)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (pv_value != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_blob);

    /* Check data length if required */
    if ((pstru_param->un_min.x_blob != 0) || (pstru_param->un_max.x_blob != 0))
    {
        if ((u16_len < pstru_param->un_min.x_blob) || (u16_len > pstru_param->un_max.x_blob))
        {
            LOGE ("Data length of param %s (%d bytes) is NOT within the allowed range",
                  pstru_param->pstri_key, u16_len);
            return PARAM_ERR;
        }
    }

    /* Write value to the parameter value */
    esp_err_t x_err = nvs_set_blob (g_x_handle, pstru_param->pstri_key, pv_value, u16_len);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to change value of param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    /* Commit the change to non-volatile storage */
    x_err = nvs_commit (g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to commit parameter change to non-volatile storage (%s)", esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of an int8_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      ps8_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_Int8 (PARAM_id_t enm_param_id, int8_t * ps8_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (ps8_value != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_int8_t);

    /* Get int8_t value of the parameter */
    esp_err_t x_err = nvs_get_i8 (g_x_handle, pstru_param->pstri_key, ps8_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of an int8_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [in]
**      s8_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_Int8 (PARAM_id_t enm_param_id, int8_t s8_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_int8_t);

    /* Check if new value is different than the current value */
    int8_t s8_current;
    esp_err_t x_err = nvs_get_i8 (g_x_handle, pstru_param->pstri_key, &s8_current);
    if ((x_err == ESP_OK) && (s8_current == s8_value))
    {
        return PARAM_OK;
    }

    /* Validate data value if required */
    if ((pstru_param->un_min.x_int8_t != 0) || (pstru_param->un_max.x_int8_t != 0))
    {
        if ((s8_value < pstru_param->un_min.x_int8_t) || (s8_value > pstru_param->un_max.x_int8_t))
        {
            LOGE ("Value of param %s (%d) is NOT within the allowed range", pstru_param->pstri_key, s8_value);
            return PARAM_ERR;
        }
    }

    /* Write value to the parameter value */
    x_err = nvs_set_i8 (g_x_handle, pstru_param->pstri_key, s8_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to change value of param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    /* Commit the change to non-volatile storage */
    x_err = nvs_commit (g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to commit parameter change to non-volatile storage (%s)", esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of an uint8_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      pu8_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_Uint8 (PARAM_id_t enm_param_id, uint8_t * pu8_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (pu8_value != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_uint8_t);

    /* Get uint8_t value of the parameter */
    esp_err_t x_err = nvs_get_u8 (g_x_handle, pstru_param->pstri_key, pu8_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of an uint8_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [in]
**      u8_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_Uint8 (PARAM_id_t enm_param_id, uint8_t u8_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_uint8_t);

    /* Check if new value is different than the current value */
    uint8_t u8_current;
    esp_err_t x_err = nvs_get_u8 (g_x_handle, pstru_param->pstri_key, &u8_current);
    if ((x_err == ESP_OK) && (u8_current == u8_value))
    {
        return PARAM_OK;
    }

    /* Validate data value if required */
    if ((pstru_param->un_min.x_uint8_t != 0) || (pstru_param->un_max.x_uint8_t != 0))
    {
        if ((u8_value < pstru_param->un_min.x_uint8_t) || (u8_value > pstru_param->un_max.x_uint8_t))
        {
            LOGE ("Value of param %s (%u) is NOT within the allowed range", pstru_param->pstri_key, u8_value);
            return PARAM_ERR;
        }
    }

    /* Write value to the parameter value */
    x_err = nvs_set_u8 (g_x_handle, pstru_param->pstri_key, u8_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to change value of param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    /* Commit the change to non-volatile storage */
    x_err = nvs_commit (g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to commit parameter change to non-volatile storage (%s)", esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of an int16_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      ps16_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_Int16 (PARAM_id_t enm_param_id, int16_t * ps16_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (ps16_value != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_int16_t);

    /* Get int16_t value of the parameter */
    esp_err_t x_err = nvs_get_i16 (g_x_handle, pstru_param->pstri_key, ps16_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of an int16_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [in]
**      s16_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_Int16 (PARAM_id_t enm_param_id, int16_t s16_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_int16_t);

    /* Check if new value is different than the current value */
    int16_t s16_current;
    esp_err_t x_err = nvs_get_i16 (g_x_handle, pstru_param->pstri_key, &s16_current);
    if ((x_err == ESP_OK) && (s16_current == s16_value))
    {
        return PARAM_OK;
    }

    /* Validate data value if required */
    if ((pstru_param->un_min.x_int16_t != 0) || (pstru_param->un_max.x_int16_t != 0))
    {
        if ((s16_value < pstru_param->un_min.x_int16_t) || (s16_value > pstru_param->un_max.x_int16_t))
        {
            LOGE ("Value of param %s (%d) is NOT within the allowed range", pstru_param->pstri_key, s16_value);
            return PARAM_ERR;
        }
    }

    /* Write value to the parameter value */
    x_err = nvs_set_i16 (g_x_handle, pstru_param->pstri_key, s16_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to change value of param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    /* Commit the change to non-volatile storage */
    x_err = nvs_commit (g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to commit parameter change to non-volatile storage (%s)", esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of an uint16_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      pu16_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_Uint16 (PARAM_id_t enm_param_id, uint16_t * pu16_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (pu16_value != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_uint16_t);

    /* Get uint16_t value of the parameter */
    esp_err_t x_err = nvs_get_u16 (g_x_handle, pstru_param->pstri_key, pu16_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of an uint16_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [in]
**      u16_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_Uint16 (PARAM_id_t enm_param_id, uint16_t u16_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_uint16_t);

    /* Check if new value is different than the current value */
    uint16_t u16_current;
    esp_err_t x_err = nvs_get_u16 (g_x_handle, pstru_param->pstri_key, &u16_current);
    if ((x_err == ESP_OK) && (u16_current == u16_value))
    {
        return PARAM_OK;
    }

    /* Validate data value if required */
    if ((pstru_param->un_min.x_uint16_t != 0) || (pstru_param->un_max.x_uint16_t != 0))
    {
        if ((u16_value < pstru_param->un_min.x_uint16_t) || (u16_value > pstru_param->un_max.x_uint16_t))
        {
            LOGE ("Value of param %s (%u) is NOT within the allowed range", pstru_param->pstri_key, u16_value);
            return PARAM_ERR;
        }
    }

    /* Write value to the parameter value */
    x_err = nvs_set_u16 (g_x_handle, pstru_param->pstri_key, u16_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to change value of param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    /* Commit the change to non-volatile storage */
    x_err = nvs_commit (g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to commit parameter change to non-volatile storage (%s)", esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of an int32_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      ps32_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_Int32 (PARAM_id_t enm_param_id, int32_t * ps32_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (ps32_value != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_int32_t);

    /* Get int32_t value of the parameter */
    esp_err_t x_err = nvs_get_i32 (g_x_handle, pstru_param->pstri_key, ps32_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of an int32_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [in]
**      s32_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_Int32 (PARAM_id_t enm_param_id, int32_t s32_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_int32_t);

    /* Check if new value is different than the current value */
    int32_t s32_current;
    esp_err_t x_err = nvs_get_i32 (g_x_handle, pstru_param->pstri_key, &s32_current);
    if ((x_err == ESP_OK) && (s32_current == s32_value))
    {
        return PARAM_OK;
    }

    /* Validate data value if required */
    if ((pstru_param->un_min.x_int32_t != 0) || (pstru_param->un_max.x_int32_t != 0))
    {
        if ((s32_value < pstru_param->un_min.x_int32_t) || (s32_value > pstru_param->un_max.x_int32_t))
        {
            LOGE ("Value of param %s (%d) is NOT within the allowed range", pstru_param->pstri_key, s32_value);
            return PARAM_ERR;
        }
    }

    /* Write value to the parameter value */
    x_err = nvs_set_i32 (g_x_handle, pstru_param->pstri_key, s32_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to change value of param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    /* Commit the change to non-volatile storage */
    x_err = nvs_commit (g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to commit parameter change to non-volatile storage (%s)", esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of an uint32_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      pu32_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_Uint32 (PARAM_id_t enm_param_id, uint32_t * pu32_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (pu32_value != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_uint32_t);

    /* Get uint32_t value of the parameter */
    esp_err_t x_err = nvs_get_u32 (g_x_handle, pstru_param->pstri_key, pu32_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of an uint32_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [in]
**      u32_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_Uint32 (PARAM_id_t enm_param_id, uint32_t u32_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_uint32_t);

    /* Check if new value is different than the current value */
    uint32_t u32_current;
    esp_err_t x_err = nvs_get_u32 (g_x_handle, pstru_param->pstri_key, &u32_current);
    if ((x_err == ESP_OK) && (u32_current == u32_value))
    {
        return PARAM_OK;
    }

    /* Validate data value if required */
    if ((pstru_param->un_min.x_uint32_t != 0) || (pstru_param->un_max.x_uint32_t != 0))
    {
        if ((u32_value < pstru_param->un_min.x_uint32_t) || (u32_value > pstru_param->un_max.x_uint32_t))
        {
            LOGE ("Value of param %s (%u) is NOT within the allowed range", pstru_param->pstri_key, u32_value);
            return PARAM_ERR;
        }
    }

    /* Write value to the parameter value */
    x_err = nvs_set_u32 (g_x_handle, pstru_param->pstri_key, u32_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to change value of param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    /* Commit the change to non-volatile storage */
    x_err = nvs_commit (g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to commit parameter change to non-volatile storage (%s)", esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of an int64_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      ps64_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_Int64 (PARAM_id_t enm_param_id, int64_t * ps64_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (ps64_value != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_int64_t);

    /* Get int64_t value of the parameter */
    esp_err_t x_err = nvs_get_i64 (g_x_handle, pstru_param->pstri_key, ps64_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of an int64_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [in]
**      s64_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_Int64 (PARAM_id_t enm_param_id, int64_t s64_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_int64_t);

    /* Check if new value is different than the current value */
    int64_t s64_current;
    esp_err_t x_err = nvs_get_i64 (g_x_handle, pstru_param->pstri_key, &s64_current);
    if ((x_err == ESP_OK) && (s64_current == s64_value))
    {
        return PARAM_OK;
    }

    /* Validate data value if required */
    if ((pstru_param->un_min.x_int64_t != 0) || (pstru_param->un_max.x_int64_t != 0))
    {
        if ((s64_value < pstru_param->un_min.x_int64_t) || (s64_value > pstru_param->un_max.x_int64_t))
        {
            LOGE ("Value of param %s (%lld) is NOT within the allowed range", pstru_param->pstri_key, s64_value);
            return PARAM_ERR;
        }
    }

    /* Write value to the parameter value */
    x_err = nvs_set_i64 (g_x_handle, pstru_param->pstri_key, s64_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to change value of param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    /* Commit the change to non-volatile storage */
    x_err = nvs_commit (g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to commit parameter change to non-volatile storage (%s)", esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of an uint64_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [out]
**      pu64_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Get_Uint64 (PARAM_id_t enm_param_id, uint64_t * pu64_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS) && (pu64_value != NULL));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_uint64_t);

    /* Get uint64_t value of the parameter */
    esp_err_t x_err = nvs_get_u64 (g_x_handle, pstru_param->pstri_key, pu64_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to access param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of an uint64_t parameter
**
** @param [in]
**      enm_param_id: Parameter ID
**
** @param [in]
**      u64_value: Parameter's value
**
** @return
**      @arg    PARAM_OK
**      @arg    PARAM_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_PARAM_Set_Uint64 (PARAM_id_t enm_param_id, uint64_t u64_value)
{
    PARAM_info_t * pstru_param = &g_astru_params[enm_param_id];

    ASSERT_PARAM (g_b_initialized && (enm_param_id < PARAM_NUM_PARAMS));
    ASSERT_PARAM (pstru_param->enm_base_type == BASE_TYPE_uint64_t);

    /* Check if new value is different than the current value */
    uint64_t u64_current;
    esp_err_t x_err = nvs_get_u64 (g_x_handle, pstru_param->pstri_key, &u64_current);
    if ((x_err == ESP_OK) && (u64_current == u64_value))
    {
        return PARAM_OK;
    }

    /* Validate data value if required */
    if ((pstru_param->un_min.x_uint64_t != 0) || (pstru_param->un_max.x_uint64_t != 0))
    {
        if ((u64_value < pstru_param->un_min.x_uint64_t) || (u64_value > pstru_param->un_max.x_uint64_t))
        {
            LOGE ("Value of param %s (%llu) is NOT within the allowed range", pstru_param->pstri_key, u64_value);
            return PARAM_ERR;
        }
    }

    /* Write value to the parameter value */
    x_err = nvs_set_u64 (g_x_handle, pstru_param->pstri_key, u64_value);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to change value of param %s (%s)", pstru_param->pstri_key, esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    /* Commit the change to non-volatile storage */
    x_err = nvs_commit (g_x_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to commit parameter change to non-volatile storage (%s)", esp_err_to_name (x_err));
        return PARAM_ERR;
    }

    return PARAM_OK;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
