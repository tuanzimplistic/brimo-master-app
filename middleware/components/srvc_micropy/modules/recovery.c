/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : recovery.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2023 Jan 7
**  @brief      : C-implementation of recovery MP module
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Provides API so that MicroPython script can backup and restore operating data in case of power
**              interruption
** @details
**              During normal operation, the MicroPython script can call recovery.set_data() function to store its
**              internal state onto a reseved cache in RAM. As soon as power interruption is detected, Platform part
**              of Master firmware will flush that data onto non-volatile memory. On the next bootup, that data will
**              be restored onto the cache. The MicroPython script can then call recovery.get_data() to get the data.
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "recovery.h"                   /* Public header of this MP module */
#include "srvc_micropy.h"               /* Use common return code */
#include "srvc_recovery.h"              /* Use Recovery service */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
#define TAG                             "Srvc_Micropy";

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Intermediate buffer to glue the data from MicroPython environment with C environment */
static uint8_t g_au8_buf [RCVR_MAX_DATA_LEN];

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
**      Stores a block of binary data onto recovery cache which will later be saved onto non-volatile memory when power
**      interruption occurs
**
** @details
**      Calling this function will store the given data into a reserved cached in RAM. The given data will override
**      what is being stored in the cache. The cache is only flushed onto non-volatile memory when power interruption
**      is detected.
**      Example:
**          import recovery
**          # Store a tuple data into recovery cache
**          recovery.set_data((0x11, 0x11, 0x11, 0x11, 0x11, 0x11))
**          # Override the current data in recovery cache with new list data
**          recovery.set_data([0x22, 0x22, 0x22, 0x22, 0x22, 0x22])
**
** @note
**      Length in bytes of the data to store in the cache must be greater than RCVR_MIN_DATA_LEN and not exceed
**      RCVR_MAX_DATA_LEN (srvc_recovery.h)
**
** @param [in]
**      x_blob_data: Data to store. The data can be a tuple, or a list
**
** @return
**      @arg    false: failed to store the data onto recovery cache
**      @arg    true: the data has been stored successfully onto recovery cache
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Set_Recovery_Data (mp_obj_t x_blob_data)
{
    /* Validate data type of the passed data */
    if ((!mp_obj_is_type (x_blob_data, &mp_type_tuple)) &&
        (!mp_obj_is_type (x_blob_data, &mp_type_list)))
    {
        mp_raise_msg (&mp_type_TypeError, "Data must be a tuple, or a list");
        return mp_const_false;
    }

    /* Extract data and length of the passed data */
    size_t x_len = 0;
    mp_obj_t * px_elem = NULL;
    mp_obj_get_array (x_blob_data, &x_len, &px_elem);

    /* Validate */
    if ((x_len == 0) || (px_elem == NULL))
    {
        mp_raise_msg (&mp_type_TypeError, "Data must be a valid tuple or list");
        return mp_const_false;
    }

    /* Ensure that data length is valid */
    if ((x_len < RCVR_MIN_DATA_LEN) || (x_len > RCVR_MAX_DATA_LEN))
    {
        mp_raise_msg_varg (&mp_type_ValueError, "Data length must be from %d to %d bytes",
                                                RCVR_MIN_DATA_LEN, RCVR_MAX_DATA_LEN);
        return mp_const_false;
    }

    /* Extract the data */
    for (uint16_t u16_idx = 0; u16_idx < x_len; u16_idx++)
    {
        g_au8_buf[u16_idx] = (uint8_t)mp_obj_get_int (px_elem[u16_idx]);
    }

    /* Store the data onto recovery cache */
    if (enm_RCVR_Set_Data (g_au8_buf, x_len) != RCVR_OK)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to store data onto recovery cache");
        return mp_const_false;
    }

    /* Successful */
    return mp_const_true;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets back the data that has been stored before
**
** @details
**      This function reads the data that is currently in recovery cache. If a power interruption has occurred,
**      recovery cache contains the data which had been stored before the interruption. The data is returned as a Bytes.
**      If there is no data in cache, the function returns None.
**      Example:
**          import recovery
**          from struct import *
**          # Serialize and backup data
**          recovery.set_data(tuple(pack('hhl', 1, 2, 3)))
**          # Restore and deserialize
**          data = unpack('hhl', recovery.get_data())
**
** @return
**      @arg    None: no data is in recovery cache
**      @arg    Bytes: the data currently in recovery cache
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Get_Recovery_Data (void)
{
    mp_obj_t x_value_obj = mp_const_none;

    /*
    ** Get pointer to the data (if present) in recovery cache
    ** This operation is safe from race condition because MicroPython is the only provider/consumer of the cache
    ** and MicroPython runs in a single thread
    */
    uint8_t * pu8_data;
    uint16_t u16_data_len = u16_RCVR_Get_Data_Pointer (&pu8_data);
    if (u16_data_len == 0)
    {
        return mp_const_none;
    }

    /* Encode the data value into Bytes object */
    x_value_obj = mp_obj_new_bytes ((byte *)pu8_data, u16_data_len);

    return x_value_obj;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
