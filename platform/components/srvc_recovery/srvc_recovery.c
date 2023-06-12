/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_recovery.c
**  @author     : Nguyen Ngoc Tung
**  @date       : 2022 Dec 29
**  @brief      : Implementation of Srvc_Recovery module
**  @namespace  : RCVR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Recovery
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_recovery.h"              /* Public header of this module */
#include "srvc_param.h"                 /* Save and load data to/from flash */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/event_groups.h"      /* Use FreeRTOS event group */
#include "freertos/semphr.h"            /* Use FreeRTOS semaphore */

#include "string.h"                     /* Use memcpy() */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Maximum number of callback functions can be registered */
#define RCVR_MAX_NUM_CALLBACKS          10

/** @brief  Structure encapsulating a callback function and its argument */
typedef struct
{
    RCVR_callback_t     pfnc_cb;        //!< Pointer to callback function
    void *              pv_arg;         //!< Argument passed to the callback function when invoking it
} RCVR_cb_info_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Recovery";

/** @brief  Indicates if Srvc_Recovery module is initialized or not */
static bool g_b_initialized = false;

/** @brief  List of callback functions invoked when backup process is about to start */
static RCVR_cb_info_t g_astru_callbacks[RCVR_MAX_NUM_CALLBACKS];

/** @brief  Generic mutex serializing concurrent accesses to the service */
static SemaphoreHandle_t g_x_mutex = NULL;

/** @brief  Cache storing data in RAM */
static uint8_t g_au8_cache[RCVR_MAX_DATA_LEN] = {0};

/** @brief  Size in bytes of the data current stored in cache */
static uint16_t g_u16_data_len = 0;

/** @brief  Indicates if new data in cache is available */
static bool g_b_new_data_present = false;

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
**      Initializes Srvc_Recovery module
**
** @details
**      This function reads the data (if available) from non-volatile memory back onto volatile memory
**
** @return
**      @arg    RCVR_OK
**      @arg    RCVR_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
RCVR_status_t enm_RCVR_Init (void)
{
    /* If Srvc_Recovery module has been initialized, don't need to do that again */
    if (g_b_initialized)
    {
        return RCVR_OK;
    }

    /* Read data stored in flash and put it into the cache in RAM */
    uint8_t * pu8_data = NULL;
    uint16_t u16_len;
    ASSERT_PARAM (s8_PARAM_Get_Blob (PARAM_COOKING_SCRIPT_DATA, &pu8_data, &u16_len) == PARAM_OK);
    if ((u16_len < RCVR_MIN_DATA_LEN) || (u16_len > RCVR_MAX_DATA_LEN))
    {
        /* Invalid data */
        g_u16_data_len = 0;
    }
    else
    {
        LOGW ("Found recovery data. Recover it.");

        /* Read the data */
        memcpy (g_au8_cache, pu8_data, u16_len);
        g_u16_data_len = u16_len;

        /* The data should be used once, erase it */
        uint8_t u8_dummy = 0;
        ASSERT_PARAM (s8_PARAM_Set_Blob (PARAM_COOKING_SCRIPT_DATA, &u8_dummy, sizeof (u8_dummy)) == PARAM_OK);
    }
    free (pu8_data);

    /* Initialize callback function list */
    for (uint8_t u8_idx = 0; u8_idx < RCVR_MAX_NUM_CALLBACKS; u8_idx++)
    {
        g_astru_callbacks[u8_idx].pfnc_cb = NULL;
        g_astru_callbacks[u8_idx].pv_arg = NULL;
    }

    /* Create mutex serializing concurrent accesses to the service */
    g_x_mutex = xSemaphoreCreateMutex ();
    ASSERT_PARAM (g_x_mutex != NULL);

    /* Initialization is done */
    g_b_initialized = true;
    return RCVR_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Stores a block of data onto cache in volatile memory
**
** @note
**      Length in bytes of the data must be from (including) RCVR_MIN_DATA_LEN to RCVR_MAX_DATA_LEN
**
** @param [in]
**      pv_data: Pointer to the data to store
**
** @param [in]
**      u16_len: Length in byte of the data to store
**
** @return
**      @arg    RCVR_OK
**      @arg    RCVR_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
RCVR_status_t enm_RCVR_Set_Data (const void * pv_data, uint16_t u16_len)
{
    ASSERT_PARAM (g_b_initialized && (pv_data != NULL));

    /* Ensure that length in bytes of the data can be fit in the cache */
    if ((u16_len < RCVR_MIN_DATA_LEN) || (u16_len > RCVR_MAX_DATA_LEN))
    {
        return RCVR_ERR;
    }

    /* Serialize concurrent accesses to the cache */
    xSemaphoreTake (g_x_mutex, portMAX_DELAY);

    /* Store the data into cache */
    memcpy (g_au8_cache, pv_data, u16_len);
    g_u16_data_len = u16_len;
    g_b_new_data_present = true;

    /* Done */
    xSemaphoreGive (g_x_mutex);
    return RCVR_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets pointer to the data in cache if it's available
**
** @param [out]
**      ppu8_value: Pointer to the data stored in cached
**
** @return
**      @arg    0: No valid data is currently stored in cache
**      @arg    >0: Length in bytes of the data stored in cache
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
uint16_t u16_RCVR_Get_Data_Pointer (uint8_t ** ppu8_value)
{
    uint16_t u16_len = 0;

    ASSERT_PARAM (g_b_initialized && (ppu8_value != NULL));

    /* Serialize concurrent accesses to the cache */
    xSemaphoreTake (g_x_mutex, portMAX_DELAY);

    /* Return pointer to the data in cache if it is available */
    if ((g_u16_data_len >= RCVR_MIN_DATA_LEN) && (g_u16_data_len <= RCVR_MAX_DATA_LEN))
    {
        *ppu8_value = g_au8_cache;
        u16_len = g_u16_data_len;
    }

    /* Done */
    xSemaphoreGive (g_x_mutex);
    return u16_len;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Registers a callback function which will be invoked when backup process is triggered
**
** @note
**      A maximum of RCVR_MAX_NUM_CALLBACKS callbacks can be registered
**
** @note
**      This function cannot be called in interrupt context
**
** @param [in]
**      pfnc_cb: The callback to be called
**
** @param [in]
**      pv_arg: Optional context data which will be passed into the callback when it is invoked
**
** @return
**      @arg    RCVR_OK
**      @arg    RCVR_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
RCVR_status_t enm_RCVR_Register_Cb (RCVR_callback_t pfnc_cb, void * pv_arg)
{
    uint8_t u8_idx;

    /* Validate arguments */
    ASSERT_PARAM (g_b_initialized && (pfnc_cb != NULL));

    /* Serialize concurrent accesses to the callback function list */
    xSemaphoreTake (g_x_mutex, portMAX_DELAY);

    /* Store the callback function and its optional argument */
    for (u8_idx = 0; u8_idx < RCVR_MAX_NUM_CALLBACKS; u8_idx++)
    {
        if (g_astru_callbacks[u8_idx].pfnc_cb == NULL)
        {
            g_astru_callbacks[u8_idx].pfnc_cb = pfnc_cb;
            g_astru_callbacks[u8_idx].pv_arg = pv_arg;
            break;
        }
    }
    ASSERT_PARAM (u8_idx < RCVR_MAX_NUM_CALLBACKS);

    /* Cleanup */
    xSemaphoreGive (g_x_mutex);
    return RCVR_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Dumps the data currently stored in cache onto non-volatile memory
**
** @note
**      This function will invoke callbacks registered with enm_RCVR_Register_Cb() before dumping the data. Those
**      callback therefore must be done as quickly as possible so that there is enough time for the cached data to
**      be saved onto flash.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_RCVR_Backup_Data (void)
{
    /* Validate arguments */
    ASSERT_PARAM (g_b_initialized);

    /* Invoke registered callbacks */
    for (uint8_t u8_idx = 0; u8_idx < RCVR_MAX_NUM_CALLBACKS; u8_idx++)
    {
        if (g_astru_callbacks[u8_idx].pfnc_cb != NULL)
        {
            g_astru_callbacks[u8_idx].pfnc_cb (g_astru_callbacks[u8_idx].pv_arg);
        }
        else
        {
            /* All callbacks have been invoked */
            break;
        }
    }

    /* Serialize concurrent accesses to the cache */
    xSemaphoreTake (g_x_mutex, portMAX_DELAY);

    /* If the data is available in cache, stored it onto flash */
    if (g_b_new_data_present && (g_u16_data_len >= RCVR_MIN_DATA_LEN) && (g_u16_data_len <= RCVR_MAX_DATA_LEN))
    {
        ASSERT_PARAM (s8_PARAM_Set_Blob (PARAM_COOKING_SCRIPT_DATA, g_au8_cache, g_u16_data_len) == PARAM_OK);
    }

    /* Done */
    xSemaphoreGive (g_x_mutex);
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
