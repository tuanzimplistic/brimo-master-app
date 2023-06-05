/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_fwu_esp32.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Aug 27
**  @brief      : Public header of Srvc_Fwu_Esp32 module
**  @namespace  : FWUESP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Fwu_Esp32
** @{
*/

#ifndef __SRVC_FWU_ESP32_H__
#define __SRVC_FWU_ESP32_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"             /* Use common definitions */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Status returned by APIs of Srvc_Fwu_Esp32 module */
enum
{
    FWUESP_OK                       = 0,        //!< The function executed successfully
    FWUESP_ERR                      = -1,       //!< There is unknown error while executing the function
    FWUESP_ERR_BUSY                 = -2,       //!< The function failed because the module is busy
};

/** @brief  Result code of firmware update operations */
typedef enum
{
    FWUESP_RESULT_OK                = 0x00,     //!< The operation was successful
    FWUESP_RESULT_WARN_FW_OLDER     = 0x01,     //!< Version of the given firmware is older than that of the current running firmware
    FWUESP_RESULT_WARN_FW_SAME      = 0x02,     //!< Version of the given firmware is same as that of the current running firmware

    FWUESP_RESULT_ERR               = 0x80,     //!< Error, unknown
    FWUESP_RESULT_ERR_PRJ_MISMATCH  = 0x81,     //!< Error, project name mismatch
    FWUESP_RESULT_ERR_FW_TOO_BIG    = 0x82,     //!< Error, size of the given firmware is too big
    FWUESP_RESULT_ERR_NOT_PREPARED  = 0x83,     //!< Error, no firmware is given yet with s8_FWUESP_Prepare_Update()
    FWUESP_RESULT_ERR_NOT_STARTED   = 0x84,     //!< Error, firmware update process is not started yet
    FWUESP_RESULT_ERR_NOT_FINALIZED = 0x85,     //!< Error, current firmware update process is not finalized yet
    FWUESP_RESULT_ERR_DATA_INVALID  = 0x86,     //!< Error, invalid firmware data
    FWUESP_RESULT_ERR_FW_INVALID    = 0x87,     //!< Error, validation of the uploaded firmware failed

} FWUESP_result_t;

/** @brief  Structure wrapping firmware information */
typedef struct
{
    char        stri_name[32];      //!< Project name (NULL-terminated string)
    uint8_t     u8_major_rev;       //!< Firmware major revision
    uint8_t     u8_minor_rev;       //!< Firmware minor revision
    uint8_t     u8_patch_rev;       //!< Firmware patch revision
    uint32_t    u32_size;           //!< Size in bytes of the firmware

} FWUESP_fw_info_t;

/** @brief  Structure wrapping one chunk of firmware data */
typedef struct
{
    uint32_t    u32_offset;         //!< Offset in firmware image
    uint16_t    u16_data_len;       //!< Size in bytes of the data in buffer pointed by 'pu8_firmware'
    uint16_t    u16_unpacked_len;   //!< Size in bytes of the original data after decompressing 'pu8_firmware', 0 if not compressed
    uint8_t *   pu8_firmware;       //!< Pointer to the buffer containing firmware data of this chunk

} FWUESP_data_chunk_t;

/** @brief  Descriptor of current running firmware */
typedef struct
{
    const char *    pstri_name;     //!< Project name
    const char *    pstri_ver;      //!< Version
    const char *    pstri_time;     //!< Compiling date and time (e.g, Apr 19 2022 07:43:22)

} FWUESP_fw_desc_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Initializes Srvc_Fwu_Esp32 module */
extern int8_t s8_FWUESP_Init (bool * pb_first_run);

/* Gets descriptor of current running firmware */
extern int8_t s8_FWUESP_Get_Fw_Descriptor (FWUESP_fw_desc_t * pstru_desc);

/* Prepares a new firmware update */
extern int8_t s8_FWUESP_Prepare_Update (const FWUESP_fw_info_t * pstru_fw_info, FWUESP_result_t * penm_result);

/* Starts firmware update process */
extern int8_t s8_FWUESP_Start_Update (FWUESP_result_t * penm_result);

/* Programs each chunk of a firmware data onto flash */
extern int8_t s8_FWUESP_Program_Firmware (const FWUESP_data_chunk_t * pstru_fw_data, FWUESP_result_t * penm_result);

/* Finalizes current firmware update process */
extern int8_t s8_FWUESP_Finalize_Update (bool b_finalized, FWUESP_result_t * penm_result);

#endif /* __SRVC_FWU_ESP32_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
