/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : gui.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Aug 3
**  @brief      : C-implementation of gui MP module
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Provides interface so that MicroPython script can interract with GUI (Graphical User Interface)
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "gui.h"                        /* Public header of this MP module */
#include "srvc_micropy.h"               /* Use common return code */
#include "app_gui_mngr.h"               /* Use exported function of GUI manager */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Macro convert a literal constant into a string */
#define _MP_STR(x)          #x
#define MP_STR(x)           _MP_STR(x)

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
**      Sets value of a GUI binding data
**
** @details
**      This function helps MP scripts set value of a GUI binding data
**      Example:
**          import gui
**          gui.set_data(gui.GUI_DATA_TEMPERATURE, 81.25)
**          gui.set_data(gui.GUI_DATA_CAKE, 'Roti')
**          gui.set_data(gui.GUI_DATA_MAC_ADDR, (0x00, 0x23, 0x92, 0x00, 0x01, 0xFF))
**
** @param [in]
**      x_data_alias: Alias of the GUI data to set. This alias is obtained from GUI_BINDING_DATA_TABLE in file
**                    app_gui_mngr_ext.h of GUI manager module and exported to MicroPython via gui MP module.
**
** @param [in]
**      x_value: New value of the GUI data. Value range and type of the value depend on the underlying data type of
**               the given GUI data. This information can be obtained from GUI_BINDING_DATA_TABLE in file
**               app_gui_mngr_ext.h of GUI manager module.
**
**                   Type of GUI data    | Type of x_value
**                  ---------------------+-----------------
**                   uint8_t, int8_t     | Number
**                   uint16_t, int16_t   | Number
**                   uint32_t, int32_t   | Number
**                   float               | Number
**                   string              | String
**                   blob                | Tuple or List
**
** @return
**      @arg    false: failed to set vaue of the data
**      @arg    true: value of the data has been set successfully
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Set_Gui_Data (mp_obj_t x_data_alias, mp_obj_t x_value)
{
    /* Get data index from the alias */
    int32_t s32_data_idx = mp_obj_get_int (x_data_alias);
    if ((s32_data_idx < 0) || (s32_data_idx >= GUI_NUM_DATA))
    {
        mp_raise_msg (&mp_type_ValueError, "Data alias is invalid");
        return mp_const_false;
    }
    GUI_data_id_t enm_data_id = (GUI_data_id_t)s32_data_idx;

    /* Set value of the binding data */
    GUI_data_type_t enm_data_type;
    s8_GUI_Get_Data_Type (enm_data_id, &enm_data_type);
    if (enm_data_type == GUI_DATA_TYPE_string)
    {
        /* Validate data type */
        if (!mp_obj_is_str (x_value))
        {
            mp_raise_msg (&mp_type_TypeError, "Data value must be a string");
            return mp_const_false;
        }

        /* Get string value */
        const char * pstri_value = mp_obj_str_get_str (x_value);

        /* Set value */
        if (s8_GUI_Set_Data (enm_data_id, pstri_value, 0) != GUI_OK)
        {
            LOGE ("Failed to set value of GUI binding data %d\n", enm_data_id);
            mp_raise_msg (&mp_type_OSError, "Failed to set GUI binding data");
            return mp_const_false;
        }
    }
    else if (enm_data_type == GUI_DATA_TYPE_blob)
    {
        /* Validate data type */
        if ((!mp_obj_is_type (x_value, &mp_type_tuple)) && (!mp_obj_is_type (x_value, &mp_type_list)))
        {
            mp_raise_msg (&mp_type_TypeError, "Data value must be a tuple or a list");
            return mp_const_false;
        }

        /* Get blob value */
        size_t x_len;
        mp_obj_t * px_elem;
        mp_obj_get_array (x_value, &x_len, &px_elem);
        uint8_t * pu8_value = malloc (x_len);
        if (pu8_value == NULL)
        {
            mp_raise_msg (&mp_type_MemoryError, "Failed to allocate memory for data value");
            return mp_const_false;
        }
        for (uint16_t u16_idx = 0; u16_idx < x_len; u16_idx++)
        {
            pu8_value[u16_idx] = (uint8_t)mp_obj_get_int (px_elem[u16_idx]);
        }

        /* Set value */
        if (s8_GUI_Set_Data (enm_data_id, pu8_value, x_len) != GUI_OK)
        {
            LOGE ("Failed to set value of GUI binding data %d\n", enm_data_id);
            mp_raise_msg (&mp_type_OSError, "Failed to set GUI binding data");
            free (pu8_value);
            return mp_const_false;
        }
        free (pu8_value);
    }
    else if (enm_data_type == GUI_DATA_TYPE_float)
    {
        /* Validate data type */
        if ((!mp_obj_is_type (x_value, &mp_type_float)) && (!mp_obj_is_int (x_value)))
        {
            mp_raise_msg (&mp_type_TypeError, "Data value must be a float number");
            return mp_const_false;
        }

        /* Get float value */
        float flt_value = mp_obj_get_float (x_value);

        /* Set value */
        if (s8_GUI_Set_Data (enm_data_id, &flt_value, 0) != GUI_OK)
        {
            LOGE ("Failed to set value of GUI binding data %d\n", enm_data_id);
            mp_raise_msg (&mp_type_OSError, "Failed to set GUI binding data");
            return mp_const_false;
        }
    }
    else
    {
        /* Validate data type */
        if (!mp_obj_is_int (x_value))
        {
            mp_raise_msg (&mp_type_TypeError, "Data value must be an integer number");
            return mp_const_false;
        }

        /* Get integer value */
        int32_t s32_value = mp_obj_get_int (x_value);

        /* Set value */
        if (s8_GUI_Set_Data (enm_data_id, &s32_value, 0) != GUI_OK)
        {
            LOGE ("Failed to set value of GUI binding data %d\n", enm_data_id);
            mp_raise_msg (&mp_type_OSError, "Failed to set GUI binding data");
            return mp_const_false;
        }
    }

    /* Successful */
    return mp_const_true;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of a GUI binding data
**
** @details
**      This function helps MP scripts get value of a GUI binding data
**      Example:
**          import gui
**          temperature = gui.get_data(gui.GUI_DATA_TEMPERATURE)
**          cake = gui.get_data(gui.GUI_DATA_CAKE)
**          mac_addr = gui.get_data(gui.GUI_DATA_MAC_ADDR)
**
** @param [in]
**      x_data_alias: Alias of the GUI data to set. This alias is obtained from GUI_BINDING_DATA_TABLE in file
**                    app_gui_mngr_ext.h of GUI manager module and exported to MicroPython via gui MP module.
**
** @return
**      @arg    None: failed to get value of the data
**      @arg    otherwise: value of the data
**
**                   Type of GUI data    | Type of return data
**                  ---------------------+---------------------
**                   uint8_t, int8_t     | Number
**                   uint16_t, int16_t   | Number
**                   uint32_t, int32_t   | Number
**                   float               | Number
**                   string              | String
**                   blob                | Bytes
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Get_Gui_Data (mp_obj_t x_data_alias)
{
    mp_obj_t x_value_obj = mp_const_none;

    /* Get data index from the alias */
    int32_t s32_data_idx = mp_obj_get_int (x_data_alias);
    if ((s32_data_idx < 0) || (s32_data_idx >= GUI_NUM_DATA))
    {
        mp_raise_msg (&mp_type_ValueError, "Data alias is invalid");
        return mp_const_none;
    }
    GUI_data_id_t enm_data_id = (GUI_data_id_t)s32_data_idx;

    /* Get length in bytes of the binding data's value */
    uint16_t u16_data_len = 0;
    if (s8_GUI_Get_Data (enm_data_id, NULL, &u16_data_len) != GUI_OK)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to get length of GUI binding data");
        return mp_const_none;
    }

    /* Allocate memory for the data */
    void * pv_value = malloc (u16_data_len);
    if (pv_value == NULL)
    {
        mp_raise_msg (&mp_type_MemoryError, "Failed to allocate memory for data value");
        return mp_const_none;
    }

    /* Get value of the binding data */
    if (s8_GUI_Get_Data (enm_data_id, pv_value, NULL) != GUI_OK)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to get value of GUI binding data");
        free (pv_value);
        return mp_const_none;
    }

    /* Encode the data value into corresponding Python object */
    GUI_data_type_t enm_data_type;
    s8_GUI_Get_Data_Type (enm_data_id, &enm_data_type);
    switch (enm_data_type)
    {
        case GUI_DATA_TYPE_string:
        {
            x_value_obj = mp_obj_new_str ((char *)pv_value, u16_data_len);
            break;
        }
        case GUI_DATA_TYPE_blob:
        {
            x_value_obj = mp_obj_new_bytes ((byte *)pv_value, u16_data_len);
            break;
        }
        case GUI_DATA_TYPE_float:
        {
            x_value_obj = mp_obj_new_float (*((float *)pv_value));
            break;
        }
        case GUI_DATA_TYPE_uint32_t:
        {
            x_value_obj = mp_obj_new_int_from_uint (*((uint32_t *)pv_value));
            break;
        }
        case GUI_DATA_TYPE_int32_t:
        {
            x_value_obj = mp_obj_new_int (*((int32_t *)pv_value));
            break;
        }
        case GUI_DATA_TYPE_uint16_t:
        {
            x_value_obj = mp_obj_new_int (*((uint16_t *)pv_value));
            break;
        }
        case GUI_DATA_TYPE_int16_t:
        {
            x_value_obj = mp_obj_new_int (*((int16_t *)pv_value));
            break;
        }
        case GUI_DATA_TYPE_uint8_t:
        {
            x_value_obj = mp_obj_new_int (*((uint8_t *)pv_value));
            break;
        }
        case GUI_DATA_TYPE_int8_t:
        {
            x_value_obj = mp_obj_new_int (*((int8_t *)pv_value));
            break;
        }
    }

    /* Successful */
    free (pv_value);
    return x_value_obj;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Displays a notify message on GUI
**
** @details
**      The function is asynchonous, i.e. it returns as soon as the notify message is displayed on GUI, it does not
**      wait for the notify message to be acknowledged by user.
**
** @param [in]
**      enm_type: Type of the notify
**
** @param [in]
**      x_brief: Brief description about the notify. This must be a MP string object
**
** @param [in]
**      x_detail: Detailed description about the notify. This must be a MP string object
**
** @param [in]
**      s32_wait_time: Timeout (ms) that the notify will wait for acknowledgement from user, 0 or < 0 if wait forever
**
** @return
**      @arg    false: failed to display notify on GUI
**      @arg    true: the notify is displayed on GUI successfully
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Display_Notify (MP_msg_t enm_type, mp_obj_t x_brief, mp_obj_t x_detail, int32_t s32_wait_time)
{
    GUI_notify_t stru_notify;

    /* Validate data type */
    if ((!mp_obj_is_str (x_brief)) || (!mp_obj_is_str (x_detail)))
    {
        mp_raise_msg (&mp_type_TypeError, "Type of the passed argument(s) is invalid");
        return mp_const_false;
    }

    /* Type of the notify */
    if (enm_type == MP_MSG_INFO)
    {
        stru_notify.enm_type = GUI_MSG_INFO;
    }
    else if (enm_type == MP_MSG_WARNING)
    {
        stru_notify.enm_type = GUI_MSG_WARNING;
    }
    else if (enm_type == MP_MSG_ERROR)
    {
        stru_notify.enm_type = GUI_MSG_ERROR;
    }
    else
    {
        mp_raise_msg (&mp_type_ValueError, "Invalid notify type");
        return mp_const_false;
    }

    /* Get brief and detail description */
    stru_notify.pstri_brief = mp_obj_str_get_str (x_brief);
    stru_notify.pstri_detail = mp_obj_str_get_str (x_detail);

    /* Wait time of the notify */
    stru_notify.u32_wait_time = (s32_wait_time < 0) ? 0 : s32_wait_time;

    /* Display the notify */
    if (s8_GUI_Notify (&stru_notify) != GUI_OK)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to display notify message on GUI");
        return mp_const_false;
    }

    /* Successful */
    return mp_const_true;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Displays a query message on GUI with a list of options and waits for user to select an option
**
** @details
**      The function is synchonous, i.e. it only returns when user selects one options of those provided or wait
**      timeout of the query expires
**
** @param [in]
**      enm_type: Type of the query
**
** @param [in]
**      x_brief: Brief description about the query. This must be a MP string object
**
** @param [in]
**      x_detail: Detailed description about the query. This must be a MP string object
**
** @param [in]
**      s32_wait_time: Timeout (ms) that the query will wait for user to select an option, 0 or < 0 if wait forever
**
** @param [in]
**      x_options: A python list or tuple of options so that user can choose from. Each option is a string object.
**
** @param [in]
**      s32_default_opt: Index of the option to be selected by default if s32_wait_time expires
**
** @return
**      @arg    -1: in case of some error occurred
**      @arg    Otherwise: index of the option selected by user
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Display_Query (MP_msg_t enm_type, mp_obj_t x_brief, mp_obj_t x_detail,
                             int32_t s32_wait_time, mp_obj_t x_options, int32_t s32_default_opt)
{
    GUI_query_t stru_query;
    uint8_t     u8_selection = -1;

    /* Validate data type */
    if ((!mp_obj_is_str (x_brief)) || (!mp_obj_is_str (x_detail)))
    {
        mp_raise_msg (&mp_type_TypeError, "Type of the passed argument(s) is invalid");
        return (mp_obj_new_int (-1));
    }
    if ((!mp_obj_is_type (x_options, &mp_type_tuple)) && (!mp_obj_is_type (x_options, &mp_type_list)))
    {
        mp_raise_msg (&mp_type_TypeError, "Option strings must be a tuple or a list");
        return (mp_obj_new_int (-1));
    }

    /* Type of the query */
    if (enm_type == MP_MSG_INFO)
    {
        stru_query.enm_type = GUI_MSG_INFO;
    }
    else if (enm_type == MP_MSG_WARNING)
    {
        stru_query.enm_type = GUI_MSG_WARNING;
    }
    else if (enm_type == MP_MSG_ERROR)
    {
        stru_query.enm_type = GUI_MSG_ERROR;
    }
    else
    {
        mp_raise_msg (&mp_type_ValueError, "Invalid query type");
        return (mp_obj_new_int (-1));
    }

    /* Get brief and detail description */
    stru_query.pstri_brief = mp_obj_str_get_str (x_brief);
    stru_query.pstri_detail = mp_obj_str_get_str (x_detail);

    /* Wait time of the query */
    stru_query.u32_wait_time = (s32_wait_time < 0) ? 0 : s32_wait_time;

    /* Get array of option strings and number of options */
    size_t x_num_opts;
    mp_obj_t * px_elems;
    mp_obj_get_array (x_options, &x_num_opts, &px_elems);
    if (x_num_opts > GUI_MAX_QUERY_OPTIONS)
    {
        mp_raise_msg (&mp_type_ValueError, "Number of option strings must be less than " MP_STR(GUI_MAX_QUERY_OPTIONS));
        return (mp_obj_new_int (-1));
    }

    stru_query.u8_num_options = (uint8_t)x_num_opts;
    for (uint16_t u16_idx = 0; u16_idx < x_num_opts; u16_idx++)
    {
        if (!mp_obj_is_str (px_elems[u16_idx]))
        {
            mp_raise_msg (&mp_type_TypeError, "Query options must be strings");
            return (mp_obj_new_int (-1));
        }
        stru_query.apstri_options[u16_idx] = mp_obj_str_get_str (px_elems[u16_idx]);
    }

    /* Default option */
    if ((s32_default_opt < 0) || (s32_default_opt >= x_num_opts))
    {
        mp_raise_msg (&mp_type_ValueError, "Index of default option must be less than number of options");
        return (mp_obj_new_int (-1));
    }
    stru_query.u8_default_option = (uint8_t)s32_default_opt;

    /* Display the query */
    if (s8_GUI_Query (&stru_query, &u8_selection) != GUI_OK)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to display query message on GUI");
        return (mp_obj_new_int (-1));
    }

    /* Successful */
    return (mp_obj_new_int (u8_selection));
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets elapsed time (in millisecond) since last user activity on GUI
**
** @return
**      @arg    None: failed to get GUI idle time
**      @arg    otherwise: idle time in millisecond
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Get_Idle_Time (void)
{
    uint32_t u32_inact_ms;

    /* Get GUI idle time */
    if (s8_GUI_Get_Idle_Time (&u32_inact_ms) != GUI_OK)
    {
        return mp_const_none;
    }

    return (mp_obj_new_int (u32_inact_ms));
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Triggers a GUI activity (do-nothing) to keep the GUI active
**
** @return
**      @arg    None
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Keep_Active (void)
{
    s8_GUI_Keep_Active ();
    return mp_const_none;
}


/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
