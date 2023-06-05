/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : param.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Aug 25
**  @brief      : C-implementation of param MP module
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Provides interface so that MicroPython script can interract with non-volatile storage
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "param.h"                      /* Public header of this MP module */
#include "srvc_micropy.h"               /* Use common return code */
#include "nvs_flash.h"                  /* Use non-volatile storage component from ESP-IDF */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @brief   Name of the partition containing non-volatile storage
** @note    This name is obtained from partition table of the firmware
*/
#define MP_NVS_PARTITION_NAME           "nvs"

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Micropy";

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
**      Gets list of all keys available in a non-volatile storage namespace
**
** @details
**      This function returns a tuple of keys of all settings available in a non-volatile storage namespace
**      Example:
**          import param
**          keys = param.get_all_keys('my_namespace')
**          for key in keys:
**              print(key)
**
** @param [in]
**      x_namespace: non-volatile storage namespace (MicroPython string)
**
** @return
**      @arg    None: failed to access the given namespace
**      @arg    otherwise: MicroPython tuple of all keys available
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Param_Get_All_Keys (mp_obj_t x_namespace)
{
    /* Validate data type of namespace */
    if (!mp_obj_is_str (x_namespace))
    {
        mp_raise_msg (&mp_type_TypeError, "Namespace must be a string");
        return mp_const_none;
    }

    /* Get namespace */
    const char * pstri_namespace = mp_obj_str_get_str (x_namespace);

    /* Create an iterator to enumerate NVS entries */
    nvs_iterator_t x_iter = nvs_entry_find (MP_NVS_PARTITION_NAME, pstri_namespace, NVS_TYPE_ANY);
    if (x_iter == NULL)
    {
        return (mp_obj_new_tuple (0, NULL));
    }

    /* Count number of parameters in the given namespace */
    uint16_t u16_num_params = 0;
    while (x_iter != NULL)
    {
        u16_num_params++;
        x_iter = nvs_entry_next (x_iter);
    }
    if (u16_num_params == 0)
    {
        nvs_release_iterator (x_iter);
        return (mp_obj_new_tuple (0, NULL));
    }

    /* Allocate a buffer to store MicroPython objects of all available keys */
    mp_obj_t * pax_keys = calloc (u16_num_params, sizeof (mp_obj_t));
    if (pax_keys == NULL)
    {
        mp_raise_msg (&mp_type_MemoryError, "Failed to allocate memory for setting key objects");
        nvs_release_iterator (x_iter);
        return mp_const_none;
    }

    /* Get all keys inside the namespace */
    uint16_t u16_param_count = 0;
    x_iter = nvs_entry_find (MP_NVS_PARTITION_NAME, pstri_namespace, NVS_TYPE_ANY);
    while ((x_iter != NULL) && (u16_param_count < u16_num_params))
    {
        nvs_entry_info_t stru_info;
        nvs_entry_info (x_iter, &stru_info);
        pax_keys[u16_param_count++] = mp_obj_new_str (stru_info.key, strlen (stru_info.key));
        x_iter = nvs_entry_next (x_iter);
    };

    /* Tuple of all available keys */
    mp_obj_t x_key_tuple = mp_obj_new_tuple (u16_param_count, pax_keys);

    /* Cleanup */
    free (pax_keys);
    nvs_release_iterator (x_iter);

    return x_key_tuple;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Erases all parameters in a non-volatile storage namespace
**
** @details
**      This function erases all parameters available in a non-volatile storage given its namespace
**      Example:
**          import param
**          param.erase_all ('my_namespace')
**
** @param [in]
**      x_namespace: non-volatile storage namespace (MicroPython string)
**
** @return
**      @arg    false: failed to erase the given namespace
**      @arg    true: all parameters have been erased successfully
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Param_Erase_All (mp_obj_t x_namespace)
{
    /* Validate data type of namespace */
    if (!mp_obj_is_str (x_namespace))
    {
        mp_raise_msg (&mp_type_TypeError, "Namespace must be a string");
        return mp_const_false;
    }

    /* Get namespace */
    const char * pstri_namespace = mp_obj_str_get_str (x_namespace);

    /* Open the non-volatile storage for erasing */
    nvs_handle_t x_param_handle;
    esp_err_t x_err = nvs_open_from_partition (MP_NVS_PARTITION_NAME, pstri_namespace, NVS_READWRITE, &x_param_handle);
    if (x_err != ESP_OK)
    {
        LOGE ("Failed to access namespace %s", pstri_namespace);
        mp_raise_msg (&mp_type_OSError, "Failed to access the given namespace");
        return mp_const_false;
    }

    /* Erase all parameters of the namespace */
    x_err = nvs_erase_all (x_param_handle);
    if (x_err != ESP_OK)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to erase all parameters of the given namespace");
        return mp_const_false;
    }

    /* Commit the changes */
    nvs_commit (x_param_handle);

    /* Close the non-volatile storage */
    nvs_close (x_param_handle);

    return mp_const_true;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
