/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : gui_binding.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Aug 3
**  @brief      : Registers functions and constants of gui MP module to MicroPython
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Declares functions and constants objects of gui MP module and registers them to MicroPython
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "gui.h"                /* Use exported C-binding functions */
#include "py/objstr.h"          /* Use definition of mp_obj_str_t */

/* Use GUI_BINDING_DATA_TABLE from GUI manager */
#include "../app_gui_mngr/app_gui_mngr_ext.h"

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Expand an entry in GUI_BINDING_DATA_TABLE table as enum of binding data alias */
#define MP_BINDING_DATA_TABLE_EXPAND_AS_DATA_ENUM(DATA_ID, ...)     DATA_ID,
enum
{
    GUI_BINDING_DATA_TABLE (MP_BINDING_DATA_TABLE_EXPAND_AS_DATA_ENUM)
};

/** @brief  Expand an entry in GUI_BINDING_DATA_TABLE as constant declaration for MP's gui module */
#define MP_GUI_BINDING_DATA_TABLE_AS_CONST_DECLARE(DATA_ID, ...)                \
    { MP_ROM_QSTR(MP_QSTR_##DATA_ID)        , MP_ROM_INT(DATA_ID)               },

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static mp_obj_t x_MP_Notify_Gui (size_t x_args, const mp_obj_t * px_pos_args, mp_map_t * px_kw_args);
static mp_obj_t x_MP_Query_Gui (size_t x_args, const mp_obj_t * px_pos_args, mp_map_t * px_kw_args);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Function object of x_MP_Set_Gui_Data() */
STATIC MP_DEFINE_CONST_FUN_OBJ_2(set_data_fnc_obj, x_MP_Set_Gui_Data);

/** @brief  Function object of x_MP_Get_Gui_Data() */
STATIC MP_DEFINE_CONST_FUN_OBJ_1(get_data_fnc_obj, x_MP_Get_Gui_Data);

/** @brief  Function object of x_MP_Notify_Gui() */
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(notify_fnc_obj, 1, x_MP_Notify_Gui);

/** @brief  Function object of x_MP_Query_Gui() */
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(query_fnc_obj, 1, x_MP_Query_Gui);

/** @brief  Function object of x_MP_Get_Idle_Time() */
STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_idle_time_fnc_obj, x_MP_Get_Idle_Time);

/** @brief  Function object of x_MP_Keep_Active() */
STATIC MP_DEFINE_CONST_FUN_OBJ_0(keep_active_fnc_obj, x_MP_Keep_Active);

/** @brief  Declare all properties of the module */
STATIC const mp_rom_map_elem_t x_gui_module_globals_table[] =
{
    { MP_ROM_QSTR(MP_QSTR___name__)         , MP_ROM_QSTR(MP_QSTR_gui)          },

    /* Module constants */
    GUI_BINDING_DATA_TABLE(MP_GUI_BINDING_DATA_TABLE_AS_CONST_DECLARE)
    { MP_ROM_QSTR(MP_QSTR_INFO)             , MP_ROM_INT(MP_MSG_INFO)           },
    { MP_ROM_QSTR(MP_QSTR_WARNING)          , MP_ROM_INT(MP_MSG_WARNING)        },
    { MP_ROM_QSTR(MP_QSTR_ERROR)            , MP_ROM_INT(MP_MSG_ERROR)          },

    /* Module functions */
    { MP_ROM_QSTR(MP_QSTR_set_data)         , MP_ROM_PTR(&set_data_fnc_obj)     },
    { MP_ROM_QSTR(MP_QSTR_get_data)         , MP_ROM_PTR(&get_data_fnc_obj)     },
    { MP_ROM_QSTR(MP_QSTR_notify)           , MP_ROM_PTR(&notify_fnc_obj)       },
    { MP_ROM_QSTR(MP_QSTR_query)            , MP_ROM_PTR(&query_fnc_obj)        },
    { MP_ROM_QSTR(MP_QSTR_get_idle_time)    , MP_ROM_PTR(&get_idle_time_fnc_obj)},
    { MP_ROM_QSTR(MP_QSTR_keep_active)      , MP_ROM_PTR(&keep_active_fnc_obj)  },
};
STATIC MP_DEFINE_CONST_DICT(x_gui_module_globals, x_gui_module_globals_table);

/** @brief  Define module object */
const mp_obj_module_t x_gui_module =
{
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&x_gui_module_globals,
};

/** @brief  Register the module to make it available in MicroPython */
MP_REGISTER_MODULE(MP_QSTR_gui, x_gui_module, true);

/** @brief  Empty string */
STATIC MP_DEFINE_STR_OBJ(g_x_empty_str, "");


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Binds gui.notify() MP function with x_MP_Display_Notify()
**
** @details
**      This function allow MP scripts to asynchonously displays a notify message on GUI
**      Example:
**          import gui
**          gui.notify('IP address obtained')
**          gui.notify('Connection lost', type=gui.WARNING, timer=4000)
**          gui.notify('Temperature exceeds threshold', type=gui.ERROR, brief='High temperature', timer=10000)
**
** @param [in]
**      x_args: Number of arguments passed into gui.notify() function
**
** @param [in]
**      px_pos_args: Pointer to the array of positional argruments passed into gui.notify()
**                   gui.notify() requires 1 positional arguments and other 3 optional keyword arguments can be passed:
**
**                   Argument  | Type    | Default value | Description
**                  -----------+---------+---------------+-----------------------------------------
**                   (1st arg) | String  |               | Detailed description about the notify
**                   type      | Number  | gui.INFO      | Type of the notify: gui.INFO, gui.WARNING, gui.ERROR
**                   brief     | String  | ""            | Brief description about the notify
**                   timer     | Number  | 0             | Timeout (ms) that the notify will auto close, 0 if no timeout
**
** @param [in]
**      px_kw_args: Mapping for the keyword argruments
**
** @return
**      @arg    false: failed to display notify on GUI
**      @arg    true: the notify is displayed on GUI successfully
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static mp_obj_t x_MP_Notify_Gui (size_t x_args, const mp_obj_t * px_pos_args, mp_map_t * px_kw_args)
{
    /* Argument description */
    const mp_arg_t astru_allowed_args[] =
    {
        { MP_QSTR_detail, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_type, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MP_MSG_INFO} },
        { MP_QSTR_brief, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = &g_x_empty_str} },
        { MP_QSTR_timer, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };

    /* Parse the keyword argruments passed */
    mp_arg_val_t ax_args [MP_ARRAY_SIZE (astru_allowed_args)];
    mp_arg_parse_all (x_args, px_pos_args, px_kw_args, MP_ARRAY_SIZE (astru_allowed_args), astru_allowed_args, ax_args);

    /* Display the notify message */
    return (x_MP_Display_Notify (ax_args[1].u_int, ax_args[2].u_obj, ax_args[0].u_obj, ax_args[3].u_int));
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Binds gui.query() MP function with x_MP_Display_Query()
**
** @details
**      This function allow MP scripts to synchonously displays a query message on GUI
**      Example:
**          import gui
**          gui.query('Which cake do you want to make?', ('Roti 1', 'Roti 2', 'Roti 3'))
**          gui.query('Which cake do you want to make?', ('Roti 1', 'Roti 2', 'Roti 3'), default=2, timer=10000)
**          gui.query('Failed to make cake. Do you want to retry?', ['Retry', 'Ignore'], brief='Failure', type=gui.WARNING)
**          gui.query('Oops, an unknown error occurred.', ['Retry', 'Reset', 'Cancel'],
**                    default=1, brief='Critical error', type=gui.ERROR, timer=3000)
**
** @param [in]
**      x_args: Number of arguments passed into gui.query() function
**
** @param [in]
**      px_pos_args: Pointer to the array of positional argruments passed into gui.query()
**                   gui.query() requires 2 positional arguments and other 4 optional keyword arguments can be passed:
**
**                   Argument  | Type    | Default value | Description
**                  -----------+---------+---------------+-----------------------------------------
**                   (1st arg) | String  |               | Detailed description about the query
**                   (2nd arg) | String  |               | List or tuple of user options
**                   type      | Number  | gui.INFO      | Type of the query: gui.INFO, gui.WARNING, gui.ERROR
**                   brief     | String  | ""            | Brief description about the query
**                   timer     | Number  | 0             | Timeout (ms) that the query will auto close, 0 if no timeout
**                   default   | Number  | 0             | Index of the option to be selected by default if timer expires
**
** @param [in]
**      px_kw_args: Mapping for the keyword argruments
**
** @return
**      @arg    -1: in case of some error occurred
**      @arg    Otherwise: index of the option selected by user
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static mp_obj_t x_MP_Query_Gui (size_t x_args, const mp_obj_t * px_pos_args, mp_map_t * px_kw_args)
{
    /* Argument description */
    const mp_arg_t astru_allowed_args[] =
    {
        { MP_QSTR_detail, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_options, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_type, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MP_MSG_INFO} },
        { MP_QSTR_brief, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = &g_x_empty_str} },
        { MP_QSTR_timer, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_default, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };

    /* Parse the keyword argruments passed */
    mp_arg_val_t ax_args [MP_ARRAY_SIZE (astru_allowed_args)];
    mp_arg_parse_all (x_args, px_pos_args, px_kw_args, MP_ARRAY_SIZE (astru_allowed_args), astru_allowed_args, ax_args);

    /* Display the query message and wait for response from user */
    return (x_MP_Display_Query (ax_args[2].u_int, ax_args[3].u_obj, ax_args[0].u_obj,
                                ax_args[4].u_int, ax_args[1].u_obj, ax_args[5].u_int));
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
