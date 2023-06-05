/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : ws_notify.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Apr 11
**  @brief      : C-implementation of ws_notify MP module
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Provides communication channels over Websocket protocol (server side) so that MicroPython environment
**              can broadcast notification to those clients who need.
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "ws_notify.h"                  /* Public header of this MP module */
#include "srvc_micropy.h"               /* Use common return code */
#include "srvc_ws_server.h"             /* Use Websocket server service */

#include <string.h>                     /* Use strlen() */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
#define TAG                 "Srvc_Micropy";

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

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
**      Broadcasts status of slave board to all Websocket clients that connect to ws://<master_ip>/slave/status
**
** @param [in]
**      x_status: Status to send to all Websocket clients. The status can be a string, or a tuple, or a list
**                Examples: ws_notify.notify_slave_status("Bottom temperature = 102 Celsius degrees")
**                          ws_notify.notify_slave_status((0x11, 0x22, 0x33, 0x44))
**                          ws_notify.notify_slave_status([0x11, 0x22, 0x33, 0x44])
**
** @return
**      @arg    false: failed to notify status over Websocket connection
**      @arg    true: status has been broadcasted successfully
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Notify_Slave_Status (mp_obj_t x_status)
{
    /* Validate data type of the passed status */
    if ((!mp_obj_is_str (x_status)) &&
        (!mp_obj_is_type (x_status, &mp_type_tuple)) &&
        (!mp_obj_is_type (x_status, &mp_type_list)))
    {
        mp_raise_msg (&mp_type_TypeError, "Status must be a string, or a tuple, or a list");
        return mp_const_false;
    }

    /* Get instance of corresponding Websocket server channel */
    WSS_inst_t x_ws_inst = x_WSS_Get_Inst (WSS_SLAVE_STATUS);
    if (x_ws_inst == NULL)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to access Websocket server channel");
        return mp_const_false;
    }

    /* Extract data and data length of the passed status correspondingly with its type */
    if (mp_obj_is_str (x_status))
    {
        /* Extract the NULL-terminated string from the micropython input object */
        const char * pstri_data = mp_obj_str_get_str (x_status);

        /* Length in bytes of the status, including NULL-terminated character */
        uint16_t u16_len = strlen (pstri_data);

        /* Broadcast the status over the Websocket channel */
        if (enm_WSS_Send (x_ws_inst, WSS_ALL_CLIENTS, pstri_data, u16_len) != WSS_OK)
        {
            mp_raise_msg (&mp_type_OSError, "Failed to broadcast the status over the Websocket channel");
            return mp_const_false;
        }
    }
    else
    {
        size_t x_len = 0;
        mp_obj_t * px_elem = NULL;
        mp_obj_get_array (x_status, &x_len, &px_elem);

        /* Validate */
        if ((x_len == 0) || (px_elem == NULL))
        {
            mp_raise_msg (&mp_type_TypeError, "Status must be a valid tuple or list");
            return mp_const_false;
        }

        /* Allocate memory for the status data */
        uint8_t * pu8_data = malloc (x_len);
        if (pu8_data == NULL)
        {
            mp_raise_msg (&mp_type_OSError, "Failed to allocate memory for status data");
            return mp_const_false;
        }

        /* Extract the data */
        for (uint16_t u16_idx = 0; u16_idx < x_len; u16_idx++)
        {
            pu8_data[u16_idx] = (uint8_t)mp_obj_get_int (px_elem[u16_idx]);
        }

        /* Broadcast the status over the Websocket channel */
        if (enm_WSS_Send (x_ws_inst, WSS_ALL_CLIENTS, pu8_data, x_len) != WSS_OK)
        {
            mp_raise_msg (&mp_type_OSError, "Failed to broadcast the status over the Websocket channel");
            return mp_const_false;
        }

        /* Cleanup */
        free (pu8_data);
    }

    /* Successful */
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
