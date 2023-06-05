/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_param.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Feb 12
**  @brief      : Public header of Srvc_Param module. This file is the only header file to include in order to
**                use the module
**  @namespace  : PARAM
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Param
** @{
*/

#ifndef __SRVC_PARAM_H__
#define __SRVC_PARAM_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"             /* Use common definitions */
#include "srvc_param_ext.h"         /* Parameter configuration */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Status returned by APIs of Srvc_Param module */
enum
{
    PARAM_OK                    = 0,            //!< The function executed successfully
    PARAM_ERR                   = -1,           //!< There is unknown error while executing the function
};

/** @brief  Expand an entry in param table as parameter index enumeration */
#define PARAM_EXPAND_AS_PARAM_ID_ENUM(PARAM_ID, ...)        PARAM_ID,
typedef enum
{
    PARAM_TABLE (PARAM_EXPAND_AS_PARAM_ID_ENUM)
    PARAM_NUM_PARAMS

} PARAM_id_t;

/** @brief  Base type of parameter */
typedef enum
{
    BASE_TYPE_uint8_t,                      //!< unsigned 8-bit integer
    BASE_TYPE_int8_t,                       //!< 8-bit integer
    BASE_TYPE_uint16_t,                     //!< unsigned 16-bit integer
    BASE_TYPE_int16_t,                      //!< 16-bit integer
    BASE_TYPE_uint32_t,                     //!< unsigned 32-bit integer
    BASE_TYPE_int32_t,                      //!< 32-bit integer
    BASE_TYPE_uint64_t,                     //!< unsigned 64-bit integer
    BASE_TYPE_int64_t,                      //!< 64-bit integer
    BASE_TYPE_string,                       //!< NULL-terminated string
    BASE_TYPE_blob,                         //!< variable length binary data (array of uint8_t)

} PARAM_base_type_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Initializes Srvc_Param module */
extern int8_t s8_PARAM_Init (void);

/* Resets a parameter to its default value */
extern int8_t s8_PARAM_Reset_Default (PARAM_id_t enm_param_id);

/* Converts Param Unique Code of a parameter to parameter ID */
extern int8_t s8_PARAM_Convert_PUC_To_ID (uint16_t u16_param_puc, PARAM_id_t * penm_param_id);

/* Converts parameter ID of a parameter to Param Unique Code */
extern int8_t s8_PARAM_Convert_ID_To_PUC (PARAM_id_t enm_param_id, uint16_t * pu16_param_puc);

/* Gets data type of a parameter */
extern int8_t s8_PARAM_Get_Type (PARAM_id_t enm_param_id, PARAM_base_type_t * penm_type);

/*
** Generic function to get value of a parameter
** NOTE: The caller MUST free the memory pointed by ppu8_value after using it.
*/
extern int8_t s8_PARAM_Get_Value (PARAM_id_t enm_param_id, void ** ppv_value, uint16_t * pu16_len);

/* Generic function to set value of a parameter */
extern int8_t s8_PARAM_Set_Value (PARAM_id_t enm_param_id, const void * pv_value, uint16_t u16_len);

/*
** Gets value of an unmanaged string parameter, remember to free the memory pointed by ppstri_value after using it
** NOTE: The caller MUST free the memory pointed by ppstri_value after using it.
*/
extern int8_t s8_PARAM_Get_String_Unmanaged (const char * pstri_key, char ** ppstri_value);

/* Sets value of an unmanaged string parameter, if the parameter doesn't exist, it will be created */
extern int8_t s8_PARAM_Set_String_Unmanaged (const char * pstri_key, const char * pstri_value);

/*
** Gets value of a string parameter, remember to free the memory pointed by ppstri_value after using it
** NOTE: The caller MUST free the memory pointed by ppstri_value after using it.
*/
extern int8_t s8_PARAM_Get_String (PARAM_id_t enm_param_id, char ** ppstri_value);

/* Sets value of a string parameter */
extern int8_t s8_PARAM_Set_String (PARAM_id_t enm_param_id, const char * pstri_value);

/*
** Gets value of a blob parameter (variable length binary data)
** NOTE: The caller MUST free the memory pointed by ppu8_value after using it.
*/
extern int8_t s8_PARAM_Get_Blob (PARAM_id_t enm_param_id, uint8_t ** ppu8_value, uint16_t * pu16_len);

/* Sets value of a blob parameter (variable length binary data) */
extern int8_t s8_PARAM_Set_Blob (PARAM_id_t enm_param_id, const void * pv_value, uint16_t u16_len);

/* Gets value of an int8_t parameter */
extern int8_t s8_PARAM_Get_Int8 (PARAM_id_t enm_param_id, int8_t * ps8_value);

/* Sets value of an int8_t parameter */
extern int8_t s8_PARAM_Set_Int8 (PARAM_id_t enm_param_id, int8_t s8_value);

/* Gets value of an uint8_t parameter */
extern int8_t s8_PARAM_Get_Uint8 (PARAM_id_t enm_param_id, uint8_t * pu8_value);

/* Sets value of an uint8_t parameter */
extern int8_t s8_PARAM_Set_Uint8 (PARAM_id_t enm_param_id, uint8_t u8_value);

/* Gets value of an int16_t parameter */
extern int8_t s8_PARAM_Get_Int16 (PARAM_id_t enm_param_id, int16_t * ps16_value);

/* Sets value of an int16_t parameter */
extern int8_t s8_PARAM_Set_Int16 (PARAM_id_t enm_param_id, int16_t s16_value);

/* Gets value of an uint16_t parameter */
extern int8_t s8_PARAM_Get_Uint16 (PARAM_id_t enm_param_id, uint16_t * pu16_value);

/* Sets value of an uint16_t parameter */
extern int8_t s8_PARAM_Set_Uint16 (PARAM_id_t enm_param_id, uint16_t u16_value);

/* Gets value of an int32_t parameter */
extern int8_t s8_PARAM_Get_Int32 (PARAM_id_t enm_param_id, int32_t * ps32_value);

/* Sets value of an int32_t parameter */
extern int8_t s8_PARAM_Set_Int32 (PARAM_id_t enm_param_id, int32_t s32_value);

/* Gets value of an uint32_t parameter */
extern int8_t s8_PARAM_Get_Uint32 (PARAM_id_t enm_param_id, uint32_t * pu32_value);

/* Sets value of an uint32_t parameter */
extern int8_t s8_PARAM_Set_Uint32 (PARAM_id_t enm_param_id, uint32_t u32_value);

/* Gets value of an int64_t parameter */
extern int8_t s8_PARAM_Get_Int64 (PARAM_id_t enm_param_id, int64_t * ps64_value);

/* Sets value of an int64_t parameter */
extern int8_t s8_PARAM_Set_Int64 (PARAM_id_t enm_param_id, int64_t s64_value);

/* Gets value of an uint64_t parameter */
extern int8_t s8_PARAM_Get_Uint64 (PARAM_id_t enm_param_id, uint64_t * pu64_value);

/* Sets value of an uint64_t parameter */
extern int8_t s8_PARAM_Set_Uint64 (PARAM_id_t enm_param_id, uint64_t u64_value);

#endif /* __SRVC_PARAM_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
