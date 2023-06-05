/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : recovery_binding.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2023 Jan 7
**  @brief      : Registers functions and constants of recovery MP module to MicroPython
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Declares functions and constants objects of recovery module and registers them to MicroPython
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "recovery.h"       /* Use exported C-binding functions */

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

/** @brief  Function object of x_MP_Set_Recovery_Data() */
STATIC MP_DEFINE_CONST_FUN_OBJ_1(set_data_fnc_obj, x_MP_Set_Recovery_Data);

/** @brief  Function object of x_MP_Get_Recovery_Data() */
STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_data_fnc_obj, x_MP_Get_Recovery_Data);

/** @brief  Declare all properties of the module */
STATIC const mp_rom_map_elem_t x_recovery_module_globals_table[] =
{
    { MP_ROM_QSTR(MP_QSTR___name__)         , MP_ROM_QSTR(MP_QSTR_recovery)     },

    /* Module functions */
    { MP_ROM_QSTR(MP_QSTR_set_data)         , MP_ROM_PTR(&set_data_fnc_obj)     },
    { MP_ROM_QSTR(MP_QSTR_get_data)         , MP_ROM_PTR(&get_data_fnc_obj)     },
};
STATIC MP_DEFINE_CONST_DICT(x_recovery_module_globals, x_recovery_module_globals_table);

/** @brief  Define module object */
const mp_obj_module_t x_recovery_module =
{
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&x_recovery_module_globals,
};

/** @brief  Register the module to make it available in MicroPython */
MP_REGISTER_MODULE(MP_QSTR_recovery, x_recovery_module, true);

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
