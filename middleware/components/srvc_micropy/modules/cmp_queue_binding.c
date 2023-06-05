/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : cmp_queue_binding.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 May 29
**  @brief      : Registers functions and constants of cmp_queue MP module to MicroPython
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Declares functions and constants objects of cmp_queue module and registers them to MicroPython
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "cmp_queue.h"              /* Use exported C-binding functions */

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

/** @brief  Function object of x_MP_Send_Str() */
STATIC MP_DEFINE_CONST_FUN_OBJ_1(send_str_fnc_obj, x_MP_Send_Str);

/** @brief  Function object of x_MP_Send_Bytes() */
STATIC MP_DEFINE_CONST_FUN_OBJ_1(send_bytes_fnc_obj, x_MP_Send_Bytes);

/** @brief  Function object of x_MP_Receive_Str() */
STATIC MP_DEFINE_CONST_FUN_OBJ_0(receive_str_fnc_obj, x_MP_Receive_Str);

/** @brief  Function object of x_MP_Receive_Bytes() */
STATIC MP_DEFINE_CONST_FUN_OBJ_0(receive_bytes_fnc_obj, x_MP_Receive_Bytes);

/** @brief  Function object of x_MP_Exchange_Str() */
STATIC MP_DEFINE_CONST_FUN_OBJ_2(exchange_str_fnc_obj, x_MP_Exchange_Str);

/** @brief  Function object of x_MP_Exchange_Bytes() */
STATIC MP_DEFINE_CONST_FUN_OBJ_2(exchange_bytes_fnc_obj, x_MP_Exchange_Bytes);

/** @brief  Declare all properties of the module */
STATIC const mp_rom_map_elem_t x_cmp_queue_module_globals_table[] =
{
    { MP_ROM_QSTR(MP_QSTR___name__)         , MP_ROM_QSTR(MP_QSTR_cmp_queue)        },

    /* Module constants */
    { MP_ROM_QSTR(MP_QSTR_WAIT_FOREVER)     , MP_ROM_INT(-1)                        },

    /* Module functions */
    { MP_ROM_QSTR(MP_QSTR_send_str)         , MP_ROM_PTR(&send_str_fnc_obj)         },
    { MP_ROM_QSTR(MP_QSTR_send_bytes)       , MP_ROM_PTR(&send_bytes_fnc_obj)       },
    { MP_ROM_QSTR(MP_QSTR_receive_str)      , MP_ROM_PTR(&receive_str_fnc_obj)      },
    { MP_ROM_QSTR(MP_QSTR_receive_bytes)    , MP_ROM_PTR(&receive_bytes_fnc_obj)    },
    { MP_ROM_QSTR(MP_QSTR_exchange_str)     , MP_ROM_PTR(&exchange_str_fnc_obj)     },
    { MP_ROM_QSTR(MP_QSTR_exchange_bytes)   , MP_ROM_PTR(&exchange_bytes_fnc_obj)   },
};
STATIC MP_DEFINE_CONST_DICT(x_cmp_queue_module_globals, x_cmp_queue_module_globals_table);

/** @brief  Define module object */
const mp_obj_module_t x_cmp_queue_module =
{
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&x_cmp_queue_module_globals,
};

/** @brief  Register the module to make it available in MicroPython */
MP_REGISTER_MODULE(MP_QSTR_cmp_queue, x_cmp_queue_module, true);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
