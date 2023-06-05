/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : common_hdr.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Mar 11
**  @brief      : Common header of all modules
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Common
** @{
*/

#ifndef __COMMON_HDR_H__
#define __COMMON_HDR_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include <stdint.h>                 /* Standard types */
#include <stdbool.h>                /* Boolean type */
#include <stddef.h>                 /* NULL definition */
#include <stdlib.h>                 /* abort() function */

#include "esp_log.h"                /* Use logging component from ESP-IDF */
#include "lfs2.h"                   /* Use LittleFS v2 */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#define USE_MODULE_ASSERT
#ifdef USE_MODULE_ASSERT

/** @brief  The ASSERT_PARAM macro is used for function's parameters check */
/** @param  expr: If false, it traps the program. If true, it returns no value */
#define ASSERT_PARAM(expr)                                                              \
    if (!(expr))                                                                        \
    {                                                                                   \
        ESP_LOGE (TAG, "Assertion failed at line %d, file %s", __LINE__, __FILE__);     \
        abort();                                                                        \
    }
#else
    #define ASSERT_PARAM(expr) ((void)0)
#endif

/** @brief  Path to mount LittleFS partition */
#define LFS_MOUNT_POINT                     "/."

/** @brief  Maximum file name length and file path length in bytes */
#define MAX_FILE_NAME_LEN                   64
#define MAX_FILE_PATH_LEN                   (sizeof (LFS_MOUNT_POINT) + 1 + MAX_FILE_NAME_LEN)

/** @brief  Common status codes */
enum
{
    STATUS_OK               = 0,        //!< Success
    STATUS_ERR              = -1,       //!< A general error has occurred
    STATUS_ERR_NOT_INIT     = -2,       //!< Error reason: not initialized yet
    STATUS_ERR_BUSY         = -3,       //!< Error reason: busy
};

/** @brief  Macro to set bits of a left value */
#define SET_BITS(lvalue, bitmask)           (lvalue |= (bitmask))

/** @brief  Macro to clear bits of a left value */
#define CLR_BITS(lvalue, bitmask)           (lvalue &= ~(bitmask))

/** @brief  Macro to invert bits of a left value */
#define INV_BITS(lvalue, bitmask)           (lvalue ^= (bitmask))

/** @brief  Macro to check if all given bits of a variable are set */
#define ALL_BITS_SET(var, bitmask)          (((var) & (bitmask)) == (bitmask))

/** @brief  Macro to check if any of the given bits of a variable is/are set */
#define ANY_BITS_SET(var, bitmask)          (((var) & (bitmask)) != 0)

/** @brief  Macro to check if all given bits of a variable are reset */
#define ALL_BITS_CLR(var, bitmask)          (((var) & (bitmask)) == 0)

/** @brief  Macro to check if any of the given bits of a variable is/are reset */
#define ANY_BITS_CLR(var, bitmask)          (((var) & (bitmask)) != (bitmask))

/** @brief   Macro to convert a 2-byte buffer to an uint16_t value (little endian format) */
#define ENDIAN_GET16(any_addr)              (((uint16_t)((uint8_t *)(any_addr))[1] << 8) | \
                                             ((uint16_t)((uint8_t *)(any_addr))[0]     ))

/** @brief   Macro to convert a 2-byte buffer to an uint16_t value (big endian format) */
#define ENDIAN_GET16_BE(any_addr)           (((uint16_t)((uint8_t *)(any_addr))[0] << 8) | \
                                             ((uint16_t)((uint8_t *)(any_addr))[1]     ))

/** @brief   Macro to write an uint16_t value to 2-byte buffer (little endian format) */
#define ENDIAN_PUT16(any_addr, u16_data)    {((uint8_t *)(any_addr))[1] = (uint8_t)((u16_data) >> 8); \
                                             ((uint8_t *)(any_addr))[0] = (uint8_t)((u16_data));}

/** @brief   Macro to write an uint16_t value to 2-byte buffer (big endian format) */
#define ENDIAN_PUT16_BE(any_addr, u16_data) {((uint8_t *)(any_addr))[0] = (uint8_t)((u16_data) >> 8); \
                                             ((uint8_t *)(any_addr))[1] = (uint8_t)((u16_data));}

/** @brief   Macro to convert a 4-byte buffer to an uint32_t value (little endian format) */
#define ENDIAN_GET32(any_addr)              (((uint32_t)((uint8_t *)(any_addr))[3] << 24) | \
                                             ((uint32_t)((uint8_t *)(any_addr))[2] << 16) | \
                                             ((uint32_t)((uint8_t *)(any_addr))[1] <<  8) | \
                                             ((uint32_t)((uint8_t *)(any_addr))[0]      ))

/** @brief   Macro to convert a 4-byte buffer to an uint32_t value (big endian format) */
#define ENDIAN_GET32_BE(any_addr)           (((uint32_t)((uint8_t *)(any_addr))[0] << 24) | \
                                             ((uint32_t)((uint8_t *)(any_addr))[1] << 16) | \
                                             ((uint32_t)((uint8_t *)(any_addr))[2] <<  8) | \
                                             ((uint32_t)((uint8_t *)(any_addr))[3]      ))

/** @brief   Macro to write an uint32_t value to 4-byte buffer (little endian format) */
#define ENDIAN_PUT32(any_addr, u32_data)    {((uint8_t *)(any_addr))[3] = (uint8_t)((u32_data) >> 24); \
                                             ((uint8_t *)(any_addr))[2] = (uint8_t)((u32_data) >> 16); \
                                             ((uint8_t *)(any_addr))[1] = (uint8_t)((u32_data) >>  8); \
                                             ((uint8_t *)(any_addr))[0] = (uint8_t)((u32_data));}

/** @brief   Macro to write an uint32_t value to 4-byte buffer (big endian format) */
#define ENDIAN_PUT32_BE(any_addr, u32_data) {((uint8_t *)(any_addr))[0] = (uint8_t)((u32_data) >> 24); \
                                             ((uint8_t *)(any_addr))[1] = (uint8_t)((u32_data) >> 16); \
                                             ((uint8_t *)(any_addr))[2] = (uint8_t)((u32_data) >>  8); \
                                             ((uint8_t *)(any_addr))[3] = (uint8_t)((u32_data));}

/** @brief  Macro to start time monitoring */
#define TIMER_RESET(x_timer)                (x_timer) = xTaskGetTickCount ()

/** @brief  Macro to check time (in FreeRTOS ticks) elapsed since a timer was started */
#define TIMER_ELAPSED(x_timer)              (xTaskGetTickCount () - (x_timer))

/** @brief  Macro converting number of FreeRTOS ticks to milliseconds */
#define TIMER_TICKS_TO_MS(u32_ticks)        ((u32_ticks) * portTICK_RATE_MS)

/** @brief  Macro converting milliseconds to number of FreeRTOS ticks */
#define TIMER_MS_TO_TICKS(u32_ms)           (pdMS_TO_TICKS(u32_ms))

/** @brief  Macros printing debug log */
#define LOGV(...)                           ESP_LOGV (TAG, __VA_ARGS__)
#define LOGD(...)                           ESP_LOGD (TAG, __VA_ARGS__)
#define LOGI(...)                           ESP_LOGI (TAG, __VA_ARGS__)
#define LOGW(...)                           ESP_LOGW (TAG, __VA_ARGS__)
#define LOGE(...)                           ESP_LOGE (TAG, __VA_ARGS__)
#define LOG_DATA(pv_data, u16_len)          ESP_LOG_BUFFER_HEX (TAG, pv_data, u16_len)

/** @brief  Macro converting a literal definition to string */
#define TO_STR(DEFINE)                      _TO_STR(DEFINE)
#define _TO_STR(DEFINE)                     #DEFINE

/**
** @brief  Macro displaying the minimum amount of remaining stack space that was available to the task since
**         the task started executing
*/
#define PRINT_STACK_USAGE(u16_period_ms)                                                    \
{                                                                                           \
    static TickType_t x_print_timer;                                                        \
    if ((TIMER_ELAPSED (x_print_timer) >= pdMS_TO_TICKS (u16_period_ms)))                   \
    {                                                                                       \
        TIMER_RESET (x_print_timer);                                                        \
        UBaseType_t x_stack_remain = uxTaskGetStackHighWaterMark (NULL);                    \
        LOGI ("Remaining stack of task %s = %d", pcTaskGetName (NULL), x_stack_remain);     \
        LOGI ("Free DMA heap size = %d", heap_caps_get_free_size (MALLOC_CAP_DMA));         \
    }                                                                                       \
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @brief   Handle to LittleFS object of flash storage
** @note    This variable is exported by MP_VFS_LFSx(mount) function of vfs_lfsx.c (srvc_micropy component)
*/
extern lfs2_t * g_px_lfs2;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#endif /* __COMMON_HDR_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
