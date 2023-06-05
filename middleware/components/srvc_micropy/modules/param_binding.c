/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : param_binding.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Aug 25
**  @brief      : Registers functions and constants of param MP module to MicroPython
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Declares functions and constants objects of param module and registers them to MicroPython
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "param.h"              /* Use exported C-binding functions */

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

/** @brief  Function object of x_MP_Param_Get_All_Keys() */
STATIC MP_DEFINE_CONST_FUN_OBJ_1(get_all_keys_fnc_obj, x_MP_Param_Get_All_Keys);

/** @brief  Function object of x_MP_Param_Erase_All() */
STATIC MP_DEFINE_CONST_FUN_OBJ_1(erase_all_fnc_obj, x_MP_Param_Erase_All);

/** @brief  Declare all properties of the module */
STATIC const mp_rom_map_elem_t x_param_module_globals_table[] =
{
    { MP_ROM_QSTR(MP_QSTR___name__)         , MP_ROM_QSTR(MP_QSTR_param)            },

    /* Module functions */
    { MP_ROM_QSTR(MP_QSTR_get_all_keys)     , MP_ROM_PTR(&get_all_keys_fnc_obj)     },
    { MP_ROM_QSTR(MP_QSTR_erase_all)        , MP_ROM_PTR(&erase_all_fnc_obj)        },
};
STATIC MP_DEFINE_CONST_DICT(x_param_module_globals, x_param_module_globals_table);

/** @brief  Define module object */
const mp_obj_module_t x_param_module =
{
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&x_param_module_globals,
};

/** @brief  Register the module to make it available in MicroPython */
MP_REGISTER_MODULE(MP_QSTR_param, x_param_module, true);

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
