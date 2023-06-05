/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_master_commander.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Dec 18
**  @brief      : Public header of Srvc_Master_Commander module
**  @namespace  : MCMD
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Master_Commander
** @{
*/

#ifndef __SRVC_MASTER_COMMANDER_H__
#define __SRVC_MASTER_COMMANDER_H__

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

/** @brief  Handle to manage a Master commander */
typedef struct MCMD_obj *           MCMD_inst_t;

/** @brief  Status returned by APIs of Srvc_Master_Commander module */
enum
{
    MCMD_OK                         = 0,        //!< The function executed successfully
    MCMD_ERR                        = -1,       //!< There is unknown error while executing the function
    MCMD_ERR_BUSY                   = -2,       //!< The function failed because the given instance is busy
};

/** @brief  Current state of Slave board during firmware update */
typedef enum
{
    MCMD_STATE_RESERVED             = 0x00,     //!< Reserved for future use
    MCMD_STATE_BL_IDLE              = 0x01,     //!< Slave board is in bootloader mode and waiting for new firmware
    MCMD_STATE_BL_DOWNLOAD          = 0x02,     //!< Bootloader is downloading firmware onto flash
    MCMD_STATE_BL_INSTALLING        = 0x03,     //!< Bootloader firmware is being installed
    MCMD_STATE_BL_INSTALLED         = 0x04,     //!< Bootloader firmware has been installed
    MCMD_STATE_BL_DONE_OK           = 0x05,     //!< Firmware update has been done successfully
    MCMD_STATE_BL_DONE_ERR          = 0x80,     //!< Firmware update has failed

} MCMD_fwu_state_t;

/** @brief  Result code of firmware update process returned by Slave board */
typedef enum
{
    MCMD_RESULT_OK                          = 0x00,     //!< The operation was successful

    MCMD_RESULT_WARN_FW_OLDER_VER           = 0x01,     //!< Version of the given fw is older than that of running fw
    MCMD_RESULT_WARN_FW_SAME_VER            = 0x02,     //!< Version of the given fw is same as that of running fw
    MCMD_RESULT_WARN_FW_VAR_MISMATCH        = 0x03,     //!< Variant ID of the firmware to be downloaded doesn't match
    MCMD_RESULT_WARN_FW_ALREADY_EXIST       = 0x04,     //!< The given fw already exists on the Slave board

    MCMD_RESULT_ERR_UNKNOWN                 = 0x80,     //!< Unknown error
    MCMD_RESULT_ERR_FW_NOT_COMPATIBLE       = 0x81,     //!< The given fw is not compatible with Slave board
    MCMD_RESULT_ERR_FW_SIZE_TOO_BIG         = 0x82,     //!< Size of the given fw is too big
    MCMD_RESULT_ERR_FW_REJECTED             = 0x83,     //!< The given firmware is not accepted
    MCMD_RESULT_ERR_FW_UPDATE_NOT_STARTED   = 0x84,     //!< Fw update process is not started yet
    MCMD_RESULT_ERR_FW_UPDATE_NOT_DONE      = 0x85,     //!< Previous firmware update process is not done yet
    MCMD_RESULT_ERR_INVALID_DATA            = 0x86,     //!< The given firmware data is invalid
    MCMD_RESULT_ERR_VALIDATION_FAILED       = 0x87,     //!< Validation of the firmware just downloaded failed
    MCMD_RESULT_ERR_FW_DOWNLOAD_TIMEOUT     = 0x88,     //!< Time between 2 consecutive download data chunks is too long
    MCMD_RESULT_ERR_INSTALL_BL_FAILED       = 0x89,     //!< Installation of bootloader firmware failed
    MCMD_RESULT_ERR_APP_CORRUPT             = 0x8A,     //!< Application firmware is corrupt and cannot be used

    MCMD_RESULT_ERR_ERASING_FAILED          = 0x90,     //!< Failed to erase flash (before downloading firmware)
    MCMD_RESULT_ERR_WRITING_FAILED          = 0x91,     //!< Failed to write flash (while downloading firmware)

} MCMD_result_code_t;

/** @brief  Events fired by Srvc_Master_Commander module */
typedef enum
{
    MCMD_EVT_SLAVE_IN_BOOTLOADER,           //!< Indicates Slave board is working in Bootloader mode

} MCMD_evt_t;

/** @brief  Callback invoked when an event occurs */
typedef void (*MCMD_cb_t) (MCMD_inst_t x_inst, MCMD_evt_t enm_evt, const void * pv_data, uint16_t u16_len);

/** @brief  Context data of events fired by Srvc_Master_Commander module */
typedef union
{
    /* Context data of MCMD_EVT_SLAVE_IN_BOOTLOADER */
    MCMD_fwu_state_t enm_bl_state;          //!< Current state of Slave board during firmware update

} MCMD_evt_data_t;

/** @brief   Structure wrapping major information of Slave firmware */
typedef struct
{
    uint8_t     u8_fw_type;         //!< Firmware type (0 = Bootloader firmware, 1 = Application firmware)
    uint8_t     u8_major_rev;       //!< Firmware major revision
    uint8_t     u8_minor_rev;       //!< Firmware minor revision
    uint8_t     u8_patch_rev;       //!< Firmware patch revision
    uint16_t    u16_project_id;     //!< Project ID of the firmware
    uint16_t    u16_variant_id;     //!< Variant ID of the firmware
    uint32_t    u32_size;           //!< Size in byte of the firmware
    uint32_t    u32_crc32;          //!< CRC32 of the whole firmware excluding the CRC32 word in firmware descriptor

} MCMD_fw_info_t;

/** @brief   Structure wrapping one chunk of firmware data */
typedef struct
{
    uint32_t    u32_offset;         //!< Offset from firmware's start address
    uint16_t    u16_data_len;       //!< Size in bytes of the data in 'pu8_firmware' buffer
    uint8_t *   pu8_firmware;       //!< Pointer to the buffer containing firmware data of this chunk

} MCMD_fw_data_chunk_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Gets instance of Master commander */
extern int8_t s8_MCMD_Get_Inst (MCMD_inst_t * px_inst);

/* Runs Master commander */
extern int8_t s8_MCMD_Run_Inst (MCMD_inst_t x_inst);

/* Registers callack function to a Master commander */
extern int8_t s8_MCMD_Register_Cb (MCMD_inst_t x_inst, MCMD_cb_t pfnc_cb);

/*
** Gets state of Slave board if it is in Bootloader mode.
** The state will be returned via context data of MCMD_EVT_SLAVE_IN_BOOTLOADER event.
*/
extern int8_t s8_MCMD_Check_Bootloader_State (MCMD_inst_t x_inst);

/* Resets Slave board */
extern int8_t s8_MCMD_Reset (MCMD_inst_t x_inst, bool b_bootloader_mode);

/* Prepares Slave board for firmware update */
extern int8_t s8_MCMD_Prepare_Update (MCMD_inst_t x_inst, const MCMD_fw_info_t * pstru_fw_info,
                                      MCMD_result_code_t * penm_result);

/* Starts firmware update on Slave board */
extern int8_t s8_MCMD_Start_Update (MCMD_inst_t x_inst, MCMD_result_code_t * penm_result);

/* Downloads each chunk of a firmware to Slave board */
extern int8_t s8_MCMD_Download_Firmware (MCMD_inst_t x_inst, const MCMD_fw_data_chunk_t * pstru_fw_data,
                                         MCMD_result_code_t * penm_result);

/* Finalizes firmware update on Slave board */
extern int8_t s8_MCMD_Finalize_Update (MCMD_inst_t x_inst, bool b_canceled, MCMD_result_code_t * penm_result);

#endif /* __SRVC_MASTER_COMMANDER_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
