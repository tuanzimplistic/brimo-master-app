/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : cmp_queue.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 May 29
**  @brief      : C-implementation of cmp_queue MP module
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Implements queues transferring messages between C and MicroPython environments
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "cmp_queue.h"                  /* Public header of this MP module */
#include "srvc_micropy.h"               /* Use common return code */

#include <string.h>                     /* Use strlen(), etc. */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/message_buffer.h"    /* Use FreeRTOS message buffer */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Maximum size in bytes of the message received from C environment */
#define MP_MAX_C_MSG_LEN                128

/** @brief  Size in bytes of the buffer sending messages from C to MicroPython */
#define MP_QUE_C2MP_BUF_SIZE            256

/** @brief  Size in bytes of the buffer sending messages from MicroPython to C */
#define MP_QUE_MP2C_BUF_SIZE            256

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Micropy";

/** @brief  Handle of the buffer sending messages from C to MicroPython */
static MessageBufferHandle_t g_x_c2mp_buf;

/** @brief  Handle of the buffer sending messages from MicroPython to C */
static MessageBufferHandle_t g_x_mp2c_buf;

/** @brief  Buffer for send message */
static uint8_t g_au8_tx_buf[MP_MAX_C_MSG_LEN];

/** @brief  Buffer for receive message */
static uint8_t g_au8_rx_buf[MP_MAX_C_MSG_LEN];

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static int8_t s8_MP_Que_Send_To_C (const void * pv_tx_msg, uint16_t u16_tx_len);
static int8_t s8_MP_Que_Receive_From_C (void * pv_rx_msg, uint16_t * pu16_rx_len);
static int8_t s8_MP_Que_Exchange_With_C (const void * pv_tx_msg, uint16_t u16_tx_len,
                                         void * pv_rx_msg, uint16_t * pu16_rx_len, int32_t s32_timeout);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes cmp_queue module
**
** @return
**      @arg    MP_OK
**      @arg    MP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MP_Que_Init (void)
{
    /* Create buffer to send messages from C to MicroPython */
    g_x_c2mp_buf = xMessageBufferCreate (MP_QUE_C2MP_BUF_SIZE);
    if (g_x_c2mp_buf == NULL)
    {
        LOGE ("Failed to create buffer sending message from C to MicroPython");
        return MP_ERR;
    }

    /* Create buffer to send messages from MicroPython to C */
    g_x_mp2c_buf = xMessageBufferCreate (MP_QUE_MP2C_BUF_SIZE);
    if (g_x_c2mp_buf == NULL)
    {
        LOGE ("Failed to create buffer sending message from MicroPython to C");
        return MP_ERR;
    }

    return MP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Binds function cmp_queue.send_str() in MicroPython environment to s8_MP_Que_Send_To_C() in C environment.
**
** @details
**      This function helps send a string object from MicroPython environment to C environment
**      Example:
**          import cmp_queue
**          cmp_queue.send_str("Some Json message in MicroPython")
**
** @param [in]
**      x_string_obj: MicroPython string object
**
** @return
**      @arg    false: sending string message failed
**      @arg    true: sending string message successfully
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Send_Str (mp_obj_t x_string_obj)
{
    /* Validate data type */
    if (!mp_obj_is_str (x_string_obj))
    {
        mp_raise_msg (&mp_type_TypeError, "Sending data must be a string");
        return mp_const_false;
    }

    /* Extract the NULL-terminated string from the micropython input object */
    const char * pstri_tx_msg = mp_obj_str_get_str (x_string_obj);

    /* Send the message to C environment */
    if (s8_MP_Que_Send_To_C (pstri_tx_msg, 0))
    {
        return mp_const_false;
    }

    /* Successful */
    return mp_const_true;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Binds function cmp_queue.send_bytes() in MicroPython environment to s8_MP_Que_Send_To_C() in C environment.
**
** @details
**      This function helps send a tuple object or list object from MicroPython environment to C environment
**      Example 1:
**          import cmp_queue
**          cmp_queue.send_bytes([0x11, 0x22, 0x33, 0x44])
**
**      Example 2:
**          import cmp_queue
**          cmp_queue.send_bytes((0x11, 0x22, 0x33, 0x44))
**
** @note
**      All elements of the sending tuple or list object must be numbers in range of 0 -> 255
**
** @param [in]
**      x_array_obj: MicroPython tuple or list object
**
** @return
**      @arg    false: sending array message failed
**      @arg    true: sending array message successfully
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Send_Bytes (mp_obj_t x_array_obj)
{
    /* Validate data type */
    if ((!mp_obj_is_type (x_array_obj, &mp_type_tuple)) && (!mp_obj_is_type (x_array_obj, &mp_type_list)))
    {
        mp_raise_msg (&mp_type_TypeError, "Sending data must be a tuple or a list");
        return mp_const_false;
    }

    /* Extract objects from the micropython input object */
    size_t x_len;
    mp_obj_t * px_elem;
    mp_obj_get_array (x_array_obj, &x_len, &px_elem);

    /* Validate */
    if ((x_len > MP_MAX_C_MSG_LEN) || (x_len == 0) || (px_elem == NULL))
    {
        return mp_const_false;
    }

    /* Construct the message to send to C environment */
    for (uint16_t u16_idx = 0; u16_idx < x_len; u16_idx++)
    {
        g_au8_tx_buf[u16_idx] = (uint8_t)mp_obj_get_int (px_elem[u16_idx]);
    }

    /* Send the message to C environment */
    if (s8_MP_Que_Send_To_C (g_au8_tx_buf, x_len))
    {
        return mp_const_false;
    }

    /* Successful */
    return mp_const_true;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Binds function cmp_queue.receive_str() in MicroPython environment to s8_MP_Que_Receive_From_C() in C environment
**
** @details
**      This function helps receive a string message that was sent from C environment to MicroPython environment
**      Example:
**          import cmp_queue
**          cmessage = cmp_queue.receive_str()
**          if not cmessage is None:
**              print(cmessage)
**
** @return
**      @arg    None: no receive message is available
**      @arg    string object: receive message
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Receive_Str (void)
{
    uint16_t u16_rx_len = sizeof (g_au8_rx_buf);

    /* Receive NULL-terminated string from C environment if any */
    if (s8_MP_Que_Receive_From_C (g_au8_rx_buf, &u16_rx_len))
    {
        return mp_const_none;
    }

    /* Successful */
    return (mp_obj_new_str ((char *)g_au8_rx_buf, u16_rx_len));
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Binds function cmp_queue.receive_bytes() in MicroPython environment to s8_MP_Que_Receive_From_C() in C
**      environment
**
** @details
**      This function helps receive a message of binary data that was sent from C environment to MicroPython
**      environment
**      Example:
**          import cmp_queue
**          cmessage = cmp_queue.receive_bytes()
**          if not cmessage is None:
**              print(cmessage)
**
** @return
**      @arg    None: no receive message is available
**      @arg    bytes object: receive message from C
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Receive_Bytes (void)
{
    uint16_t u16_rx_len = sizeof (g_au8_rx_buf);

    /* Receive uint8_t array message from C environment if any */
    if (s8_MP_Que_Receive_From_C (g_au8_rx_buf, &u16_rx_len))
    {
        return mp_const_none;
    }

    /* Successful */
    return (mp_obj_new_bytes (g_au8_rx_buf, u16_rx_len));
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Binds function cmp_queue.exchange_str() in MicroPython environment to s8_MP_Que_Exchange_With_C() in C
**      environment
**
** @details
**      This function helps send a string message from MicroPython environment to C environment and then wait for
**      a response of string message from C environment.
**      Example 1:
**          import cmp_queue
**          cmessage = cmp_queue.exchange_str("Some Json message in MicroPython", cmp_queue.WAIT_FOREVER)
**          print(cmessage)
**
**      Example 2:
**          import cmp_queue
**          cmessage = cmp_queue.exchange_str("Some Json message in MicroPython", 100)
**          if not cmessage is None:
**              print(cmessage)
**
** @param [in]
**      x_string_obj: MicroPython string object to send to C
**
** @param [in]
**      x_timeout: Timeout (in milliseconds) waiting for the response.
**      @arg    0: no timeout, check receive queue and return immediately
**      @arg    cmp_queue.WAIT_FOREVER or < 0: wait until a receive message is available
**      @arg    > 0: if a receive message is available within x_timeout milliseconds, get the message and return.
**                      Otheriwse return as soon as x_timeout milliseconds expires
**
** @return
**      @arg    None: The given timeout expired but no receive message is available
**      @arg    string object: receive message from C
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Exchange_Str (mp_obj_t x_string_obj, mp_obj_t x_timeout)
{
    /* Validate data type */
    if (!mp_obj_is_str (x_string_obj))
    {
        mp_raise_msg (&mp_type_TypeError, "Sending data must be a string");
        return mp_const_false;
    }
    if (!mp_obj_is_int (x_timeout))
    {
        mp_raise_msg (&mp_type_TypeError, "Wait time must be an integer number");
        return mp_const_false;
    }

    /* Extract the NULL-terminated string from the micropython input object */
    const char * pstri_tx_msg = mp_obj_str_get_str (x_string_obj);

    /* Get timeout */
    int32_t s32_timeout = mp_obj_get_int (x_timeout);

    /* Send the message to C environment and wait for response with timeout */
    uint16_t u16_rx_len = sizeof (g_au8_rx_buf);
    if (s8_MP_Que_Exchange_With_C (pstri_tx_msg, 0, g_au8_rx_buf, &u16_rx_len, s32_timeout))
    {
        return mp_const_none;
    }

    /* Successful */
    return (mp_obj_new_str ((char *)g_au8_rx_buf, u16_rx_len));
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Binds function cmp_queue.exchange_bytes() in MicroPython environment to s8_MP_Que_Exchange_With_C() in
**      C environment
**
** @details
**      This function helps send a tuple or list object from MicroPython environment to C environment and then wait for
**      a response of binary data from C environment.
**      Example 1:
**          import cmp_queue
**          cmessage = cmp_queue.exchange_bytes((0x11, 0x22, 0x33, 0x44), cmp_queue.WAIT_FOREVER)
**          print(cmessage)
**
**      Example 2:
**          import cmp_queue
**          cmessage = cmp_queue.exchange_bytes([0x11, 0x22, 0x33, 0x44], 100)
**          if not cmessage is None:
**              print(cmessage)
**
** @note
**      All elements of the sending tuple or list object must be numbers in range of 0 -> 255
**
** @param [in]
**      x_array_obj: MicroPython tuple or list object
**
** @param [in]
**      x_timeout: Timeout (in milliseconds) waiting for the response.
**      @arg    0: no timeout, check receive queue and return immediately
**      @arg    cmp_queue.WAIT_FOREVER or < 0: wait until a receive message is available
**      @arg    > 0: if a receive message is available within x_timeout milliseconds, get the message and return.
**                      Otheriwse return as soon as x_timeout milliseconds expires
**
** @return
**      @arg    None: The given timeout expired but no receive message is available
**      @arg    bytes object: receive message from C
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Exchange_Bytes (mp_obj_t x_array_obj, mp_obj_t x_timeout)
{
    /* Validate data type */
    if ((!mp_obj_is_type (x_array_obj, &mp_type_tuple)) && (!mp_obj_is_type (x_array_obj, &mp_type_list)))
    {
        mp_raise_msg (&mp_type_TypeError, "Sending data must be a tuple or a list");
        return mp_const_false;
    }
    if (!mp_obj_is_int (x_timeout))
    {
        mp_raise_msg (&mp_type_TypeError, "Wait time must be an integer number");
        return mp_const_false;
    }

    /* Extract objects from the micropython input object */
    size_t x_tx_len;
    mp_obj_t * px_elem;
    mp_obj_get_array (x_array_obj, &x_tx_len, &px_elem);

    /* Validate */
    if ((x_tx_len > MP_MAX_C_MSG_LEN) || (x_tx_len == 0) || (px_elem == NULL))
    {
        return mp_const_none;
    }

    /* Construct the message to send to C environment */
    for (uint16_t u16_idx = 0; u16_idx < x_tx_len; u16_idx++)
    {
        g_au8_tx_buf[u16_idx] = (uint8_t)mp_obj_get_int (px_elem[u16_idx]);
    }

    /* Get timeout */
    int32_t s32_timeout = mp_obj_get_int (x_timeout);

    /* Send the message to C environment and wait for response with timeout */
    uint16_t u16_rx_len = sizeof (g_au8_rx_buf);
    if (s8_MP_Que_Exchange_With_C (g_au8_tx_buf, x_tx_len, g_au8_rx_buf, &u16_rx_len, s32_timeout))
    {
        return mp_const_none;
    }

    /* Successful */
    return (mp_obj_new_bytes (g_au8_rx_buf, u16_rx_len));
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a message from C environment to MicroPython environment
**
** @note
**      This function is exported for C-environment
**
** @param [in]
**      pv_msg: The message to send
**
** @param [in]
**      u16_len: Length in bytes of the message, if this is 0, strlen() shall be used to determine length
**
** @return
**      @arg    MP_OK
**      @arg    MP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MP_Que_Send_To_MP (const void * pv_msg, uint16_t u16_len)
{
    ASSERT_PARAM ((pv_msg != NULL) && (u16_len < MP_QUE_C2MP_BUF_SIZE));

    /* Determine length of the message */
    if (u16_len == 0)
    {
        u16_len = strlen (pv_msg);
    }

    /* Put the message into the relevant buffer */
    if (xMessageBufferSend (g_x_c2mp_buf, pv_msg, u16_len, 0) != u16_len)
    {
        /* Maybe not enough space in the buffer to store the message */
        return MP_ERR;
    }

    return MP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Waits and receives a message sent from MicroPython environment
**
** @note
**      This function is exported for C-environment
**
** @param [in]
**      pv_msg: Pointer to the buffer to store the received message
**
** @param [in,out]
**      pu16_max_len: For input, this is the length in bytes of the receive buffer. For output, this indicates length
**                    in bytes of the received message
**
** @return
**      @arg    MP_OK
**      @arg    MP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MP_Que_Receive_From_MP (void * pv_msg, uint16_t * pu16_len)
{
    ASSERT_PARAM ((pv_msg != NULL) && (pu16_len != NULL) && (*pu16_len > 0));

    /* Check and receive message (if any) from the relevant queue */
    size_t x_len = xMessageBufferReceive (g_x_mp2c_buf, pv_msg, *pu16_len, portMAX_DELAY);
    if (x_len == 0)
    {
        /* Receive buffer is not enough */
        return MP_ERR;
    }

    *pu16_len = x_len;
    return MP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a message from MicroPython environment to C environment
**
** @param [in]
**      pv_msg: The message to send
**
** @param [in]
**      u16_len: Length in bytes of the message, if this is 0, strlen() shall be used to determine length
**
** @return
**      @arg    MP_OK
**      @arg    MP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MP_Que_Send_To_C (const void * pv_msg, uint16_t u16_len)
{
    ASSERT_PARAM ((pv_msg != NULL) && (u16_len < MP_QUE_MP2C_BUF_SIZE));

    /* Determine length of the message */
    if (u16_len == 0)
    {
        u16_len = strlen (pv_msg);
    }

    /* Put the message into the relevant buffer */
    if (xMessageBufferSend (g_x_mp2c_buf, pv_msg, u16_len, portMAX_DELAY) != u16_len)
    {
        /* Maybe not enough space in the buffer to store the message */
        return MP_ERR;
    }

    return MP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Receives a message sent from C environment
**
** @param [in]
**      pv_msg: Pointer to the buffer to store the received message
**
** @param [in,out]
**      pu16_len: For input, this is the length in bytes of the receive buffer. For output, this indicates length
**                in bytes of the received message
**
** @return
**      @arg    MP_OK: A message has been received
**      @arg    MP_ERR: No receive message available or receive buffer is not enough to store the receive message
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MP_Que_Receive_From_C (void * pv_msg, uint16_t * pu16_len)
{
    ASSERT_PARAM ((pv_msg != NULL) && (pu16_len != NULL) && (*pu16_len > 0));

    /* Check and receive message (if any) from the relevant queue */
    size_t x_len = xMessageBufferReceive (g_x_c2mp_buf, pv_msg, *pu16_len, 0);
    if (x_len == 0)
    {
        /* No message available or receive buffer is not enough to store the receive message */
        *pu16_len = 0;
        return MP_ERR;
    }

    *pu16_len = x_len;
    return MP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a message from MicroPython environment to C environment and wait to receives a response message sent
**      from C environment
**
** @param [in]
**      pv_tx_msg: The message to send
**
** @param [in]
**      u16_tx_len: Length in bytes of the message, if this is 0, strlen() shall be used to determine length
**
** @param [in]
**      pv_rx_msg: Pointer to the buffer to store the received message.
**
** @param [in,out]
**      pu16_rx_len: For input, this is the length in bytes of the receive buffer. For output, this indicates length
**                   in bytes of the received message.
**
** @param [in]
**      s32_timeout: Timeout waiting for the response message
**      @arg    0: the function returns immediately if there is no receive message
**      @arg    >0: the function returns as soon as a receive message is available or the given timeout expires
**      @arg    <0: the function only returns when there is a receive message
**
** @return
**      @arg    MP_OK: A message has been received
**      @arg    MP_ERR: No receive message available or receive buffer is not enough to store the receive message
*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MP_Que_Exchange_With_C (const void * pv_tx_msg, uint16_t u16_tx_len,
                                         void * pv_rx_msg, uint16_t * pu16_rx_len, int32_t s32_timeout)
{
    ASSERT_PARAM ((pv_tx_msg != NULL) && (u16_tx_len < MP_QUE_MP2C_BUF_SIZE));
    ASSERT_PARAM ((pv_rx_msg != NULL) && (pu16_rx_len != NULL) && (*pu16_rx_len > 0));

    /* Determine length of the transmit message */
    if (u16_tx_len == 0)
    {
        u16_tx_len = strlen (pv_tx_msg);
    }

    /* Put the message into the relevant buffer */
    if (xMessageBufferSend (g_x_mp2c_buf, pv_tx_msg, u16_tx_len, 0) != u16_tx_len)
    {
        /* Maybe not enough space in the buffer to store the message */
        return MP_ERR;
    }

    /* Check and wait for receive message (if any) from the relevant queue */
    size_t x_rx_len = xMessageBufferReceive (g_x_c2mp_buf, pv_rx_msg, *pu16_rx_len,
                                             (s32_timeout < 0) ? portMAX_DELAY : pdMS_TO_TICKS (s32_timeout));
    if (x_rx_len == 0)
    {
        /* No message available or receive buffer is not enough to store the receive message */
        *pu16_rx_len = 0;
        return MP_ERR;
    }

    *pu16_rx_len = x_rx_len;
    return MP_OK;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
