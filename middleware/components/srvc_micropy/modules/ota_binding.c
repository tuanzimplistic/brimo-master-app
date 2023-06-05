/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : ota_binding.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Sep 14
**  @brief      : Registers functions and constants of OTA MP module to MicroPython
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Declares functions and constants objects of OTA MP module and registers them to MicroPython
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "ota.h"                /* Use exported C-binding functions */

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

/** @brief  Function object of x_MP_Update_Master_Firmware() */
STATIC MP_DEFINE_CONST_FUN_OBJ_2(update_master_firmware_fnc_obj, x_MP_Update_Master_Firmware);

/** @brief  Function object of x_MP_Update_Slave_Firmware() */
STATIC MP_DEFINE_CONST_FUN_OBJ_2(update_slave_firmware_fnc_obj, x_MP_Update_Slave_Firmware);

/** @brief  Function object of x_MP_Update_Master_File() */
STATIC MP_DEFINE_CONST_FUN_OBJ_2(update_master_file_fnc_obj, x_MP_Update_Master_File);

/** @brief  Function object of x_MP_Cancel() */
STATIC MP_DEFINE_CONST_FUN_OBJ_0(cancel_fnc_obj, x_MP_Cancel);

/** @brief  Declare all properties of the module */
STATIC const mp_rom_map_elem_t x_ota_module_globals_table[] =
{
    { MP_ROM_QSTR(MP_QSTR___name__)                 , MP_ROM_QSTR(MP_QSTR_ota)                      },

    /* Module functions */
    { MP_ROM_QSTR(MP_QSTR_update_master_firmware)   , MP_ROM_PTR(&update_master_firmware_fnc_obj)   },
    { MP_ROM_QSTR(MP_QSTR_update_slave_firmware)    , MP_ROM_PTR(&update_slave_firmware_fnc_obj)    },
    { MP_ROM_QSTR(MP_QSTR_update_master_file)       , MP_ROM_PTR(&update_master_file_fnc_obj)       },
    { MP_ROM_QSTR(MP_QSTR_cancel)                   , MP_ROM_PTR(&cancel_fnc_obj)                   },
};
STATIC MP_DEFINE_CONST_DICT(x_ota_module_globals, x_ota_module_globals_table);

/** @brief  Define module object */
const mp_obj_module_t x_ota_module =
{
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&x_ota_module_globals,
};

/** @brief  Register the module to make it available in MicroPython */
MP_REGISTER_MODULE(MP_QSTR_ota, x_ota_module, true);

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
