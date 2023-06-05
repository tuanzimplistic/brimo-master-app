/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : cmp_queue.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Aug 3
**  @brief      : Exports functions of cmp_queue MP module for binding into MicroPython
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @{
*/

#ifndef __CMP_QUEUE_H__
#define __CMP_QUEUE_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "py/runtime.h"             /* Declaration of MicroPython interpreter */

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

/* Binds function cmp_queue.send_str() in MicroPython environment to s8_MP_Que_Send_To_C() in C environment */
extern mp_obj_t x_MP_Send_Str (mp_obj_t x_string_obj);

/* Binds function cmp_queue.send_bytes() in MicroPython environment to s8_MP_Que_Send_To_C() in C environment */
extern mp_obj_t x_MP_Send_Bytes (mp_obj_t x_array_obj);

/* Binds function cmp_queue.receive_str() in MicroPython environment to s8_MP_Que_Receive_From_C() in C environment */
extern mp_obj_t x_MP_Receive_Str (void);

/* Binds function cmp_queue.receive_bytes() in MicroPython environment to s8_MP_Que_Receive_From_C() in C environment */
extern mp_obj_t x_MP_Receive_Bytes (void);

/* Binds function cmp_queue.exchange_str() in MicroPython environment to s8_MP_Que_Exchange_With_C() in C environment */
extern mp_obj_t x_MP_Exchange_Str (mp_obj_t x_string_obj, mp_obj_t x_timeout);

/* Binds function cmp_queue.exchange_bytes() in MicroPython environment to s8_MP_Que_Exchange_With_C() in C environment */
extern mp_obj_t x_MP_Exchange_Bytes (mp_obj_t x_array_obj, mp_obj_t x_timeout);

#endif /* __CMP_QUEUE_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
