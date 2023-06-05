/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : cam_binding.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Dec 18
**  @brief      : Registers functions and constants of cam MP module to MicroPython
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Declares functions and constants objects of cam MP module and registers them to MicroPython
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "cam.h"                /* Use exported C-binding functions */
#include "py/objstr.h"          /* Use definition of mp_obj_str_t */

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

/** @brief  Function object of x_MP_Take_Picture() */
STATIC MP_DEFINE_CONST_FUN_OBJ_1(cv_take_picture_fnc_obj,           x_MP_CV_Take_Picture);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(cv_take_picture_exposure_fnc_obj,  x_MP_CV_Take_Picture_Exposure);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(cv_scan_qr_fnc_obj,                x_MP_CV_Scan_QR);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(cv_init_fnc_obj,                   x_MP_CV_Init);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(cv_release_fnc_obj,                x_MP_CV_Release);

/** @brief  Declare all properties of the module */
STATIC const mp_rom_map_elem_t x_cam_module_globals_table[] =
{
    { MP_ROM_QSTR(MP_QSTR___name__)     ,       MP_ROM_QSTR(MP_QSTR_cam)          },

    /* Module functions */
    { MP_ROM_QSTR(MP_QSTR_cv_take_picture) ,            MP_ROM_PTR(&cv_take_picture_fnc_obj) },
    { MP_ROM_QSTR(MP_QSTR_cv_take_picture_exposure) ,   MP_ROM_PTR(&cv_take_picture_exposure_fnc_obj) },
    { MP_ROM_QSTR(MP_QSTR_cv_scan_qr) ,                 MP_ROM_PTR(&cv_scan_qr_fnc_obj) },
    { MP_ROM_QSTR(MP_QSTR_cv_init) ,                    MP_ROM_PTR(&cv_init_fnc_obj) },
    { MP_ROM_QSTR(MP_QSTR_cv_release) ,                 MP_ROM_PTR(&cv_release_fnc_obj) },
};
STATIC MP_DEFINE_CONST_DICT(x_cam_module_globals, x_cam_module_globals_table);

/** @brief  Define module object */
const mp_obj_module_t x_cam_module =
{
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&x_cam_module_globals,
};

/** @brief  Register the module to make it available in MicroPython */
MP_REGISTER_MODULE(MP_QSTR_cam, x_cam_module, true);

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
