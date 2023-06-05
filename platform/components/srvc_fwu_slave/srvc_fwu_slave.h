/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_fwu_slave.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Dec 22
**  @brief      : Public header of Srvc_Fwu_Slave module
**  @namespace  : FWUSLV
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Fwu_Slave
** @{
*/

#ifndef __SRVC_FWU_SLAVE_H__
#define __SRVC_FWU_SLAVE_H__

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

/** @brief  Status returned by APIs of Srvc_Fwu_Slave module */
enum
{
    FWUSLV_OK                               = 0,        //!< The function executed successfully
    FWUSLV_ERR                              = -1,       //!< There is unknown error while executing the function
    FWUSLV_ERR_BUSY                         = -2,       //!< The function failed because the module is busy
};

/** @brief  Result code of firmware update operations */
typedef enum
{
    FWUSLV_RESULT_OK                        = 0x00,     //!< The operation was successful

    FWUSLV_RESULT_WARN_FW_OLDER_VER         = 0x01,     //!< Version of the given fw is older than that of running fw
    FWUSLV_RESULT_WARN_FW_SAME_VER          = 0x02,     //!< Version of the given fw is same as that of running fw
    FWUSLV_RESULT_WARN_FW_VAR_MISMATCH      = 0x03,     //!< Variant ID of the firmware to be downloaded doesn't match
    FWUSLV_RESULT_WARN_FW_ALREADY_EXIST     = 0x04,     //!< The given fw already exists on the Slave board

    FWUSLV_RESULT_ERR_UNKNOWN               = 0x80,     //!< Unknown error
    FWUSLV_RESULT_ERR_FW_NOT_COMPATIBLE     = 0x81,     //!< The given fw is not compatible with Slave board
    FWUSLV_RESULT_ERR_FW_SIZE_TOO_BIG       = 0x82,     //!< Size of the given fw is too big
    FWUSLV_RESULT_ERR_FW_REJECTED           = 0x83,     //!< The given firmware is not accepted
    FWUSLV_RESULT_ERR_FW_UPDATE_NOT_STARTED = 0x84,     //!< Fw update process is not started yet
    FWUSLV_RESULT_ERR_FW_UPDATE_NOT_DONE    = 0x85,     //!< Previous firmware update process is not done yet
    FWUSLV_RESULT_ERR_INVALID_DATA          = 0x86,     //!< The given firmware data is invalid
    FWUSLV_RESULT_ERR_VALIDATION_FAILED     = 0x87,     //!< Validation of the firmware just downloaded failed
    FWUSLV_RESULT_ERR_FW_DOWNLOAD_TIMEOUT   = 0x88,     //!< Time between 2 consecutive download data chunks is too long
    FWUSLV_RESULT_ERR_INSTALL_BL_FAILED     = 0x89,     //!< Installation of bootloader firmware failed
    FWUSLV_RESULT_ERR_APP_CORRUPT           = 0x8A,     //!< Application firmware is corrupt and cannot be used

    FWUSLV_RESULT_ERR_ERASING_FAILED        = 0x90,     //!< Failed to erase flash (before downloading firmware)
    FWUSLV_RESULT_ERR_WRITING_FAILED        = 0x91,     //!< Failed to write flash (while downloading firmware)

} FWUSLV_result_t;

/** @brief  Offset of slave firmware descriptor */
#define FWUSLV_DESC_OFFSET                  0x200

/** @brief  Slave firmware descriptor */
typedef struct
{
    uint32_t    u32_recognizer;             //!< Magic word to recognize Proteus firmware, this is always 0xAA55CC33
    uint8_t     u8_descriptor_rev;          //!< Revision of this descriptor structure
    uint8_t     u8_fw_type;                 //!< Firmware type: 0 = bootloader, 1 = application
    uint8_t     u8_major_rev;               //!< Firmware major revision
    uint8_t     u8_minor_rev;               //!< Firmware minor revision
    uint8_t     u8_patch_rev;               //!< Firmware patch revision
    uint8_t     au8_build_number[3];        //!< Firmware build number (byte 0 is least significant)
    uint8_t     au8_reserved[4];            //!< Reserved for future use
    uint16_t    u16_project_id;             //!< Project ID of this firmware
    uint16_t    u16_variant_id;             //!< Variant ID of this firmware
    uint32_t    u32_start_addr;             //!< Firmware start address in memory map
    uint32_t    u32_size;                   //!< Firmware size in bytes
    uint32_t    u32_crc;                    //!< Checksum CRC32 (whole firmware but excluding CRC word)
    uint32_t    u32_run_addr;               //!< Firmware run address in memory map
    char        stri_build_time[32];        //!< Firmware build time (NULL-terminated string)
    char        stri_desc[64];              //!< NULL-terminated description about the firmware

} FWUSLV_desc_t;

/** @brief  Structure wrapping one chunk of firmware data */
typedef struct
{
    uint32_t    u32_offset;                 //!< Offset in firmware image
    uint16_t    u16_data_len;               //!< Size in bytes of the data in buffer pointed by 'pu8_firmware'
    uint8_t *   pu8_firmware;               //!< Pointer to the buffer containing firmware data of this chunk

} FWUSLV_data_chunk_t;

/** @brief  Firmware types */
enum
{
    FWUSLV_TYPE_BL = 0,                     //!< Bootloader firmware
    FWUSLV_TYPE_APP,                        //!< Application firmware
};

/** @brief  Modes of slave board */
typedef enum
{
    FWUSLV_MODE_BL = 0,                     //!< Slave is in Bootloader mode
    FWUSLV_MODE_APP,                        //!< Slave is in Application mode
    FWUSLV_MODE_UNKNOWN                     //!< Failed to communicate with Slave board to determine mode

} FWUSLV_slave_mode_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Initializes Srvc_Fwu_Slave module */
extern int8_t s8_FWUSLV_Init (void);

/* Gets current mode of slave board */
extern int8_t s8_FWUSLV_Get_Mode (FWUSLV_slave_mode_t * penm_mode);

/* Gets version of current running application slave firmware */
extern int8_t s8_FWUSLV_Get_App_Version (uint8_t * pu8_major, uint8_t * pu8_minor, uint8_t * pu8_patch);

/* Validates if slave firmware is valid */
extern int8_t s8_FWUSLV_Validate_Firmware_Info (const FWUSLV_desc_t * pstru_fw_desc);

/* Requests slave board to enter Bootloader mode */
extern int8_t s8_FWUSLV_Enter_Bootloader (void);

/* Requests slave board to exit Bootloader mode and enter Application mode */
extern int8_t s8_FWUSLV_Exit_Bootloader (void);

/* Prepares a new firmware update (slave board must be in Bootloader mode) */
extern int8_t s8_FWUSLV_Prepare_Update (const FWUSLV_desc_t * pstru_fw_desc, FWUSLV_result_t * penm_result);

/* Starts firmware update process (slave board must be in Bootloader mode) */
extern int8_t s8_FWUSLV_Start_Update (FWUSLV_result_t * penm_result);

/* Programs each chunk of a firmware data onto flash of slave board (slave board must be in Bootloader mode) */
extern int8_t s8_FWUSLV_Program_Firmware (const FWUSLV_data_chunk_t * pstru_fw_data, FWUSLV_result_t * penm_result);

/* Cancels or finalizes current firmware update process (slave board must be in Bootloader mode) */
extern int8_t s8_FWUSLV_Finalize_Update (bool b_finalized, FWUSLV_result_t * penm_result);

#endif /* __SRVC_FWU_SLAVE_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
