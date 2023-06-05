/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : app_ota_mngr.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Aug 28
**  @brief      : Public header of App_Ota_Mngr module
**  @namespace  : OTAMN
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Ota_Mngr
** @{
*/

#ifndef __APP_OTA_MNGR_H__
#define __APP_OTA_MNGR_H__

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

/** @brief  Status returned by APIs of App_Ota_Mngr module */
enum
{
    OTAMN_IGNORED       = 2,            //!< The operation is ignored
    OTAMN_CANCELLED     = 1,            //!< The operation is cancelled by user
    OTAMN_OK            = 0,            //!< The operation executed successfully
    OTAMN_ERR           = -1,           //!< There is unknown error while executing the operation
};

/** @brief  Target components of OTA update */
typedef enum
{
    OTAMN_MASTER_FW,                    //!< Firmware of master board (ESP32)
    OTAMN_SLAVE_FW,                     //!< Firmware of slave board (GD32)
    OTAMN_MASTER_FILE,                  //!< A file in filesystem of master board
    OTAMN_NUM_TARGETS

} OTAMN_target_t;

/** @brief  Structure wrapping OTA update configuration */
typedef struct
{
    OTAMN_target_t  enm_target;         //!< The component to update
    char *          pstri_url;          //!< URL (NULL-terminated string) to download the source for the update
    char *          pstri_inst_dir;     //!< Path (NULL-terminated string) to install the file (for file update)
    bool            b_check_newer;      //!< If version check is performed before updating (for firmware update)

} OTAMN_config_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Initializes App_Ota_Mngr module */
extern int8_t s8_OTAMN_Init (void);

/* Starts OTA update of a component */
extern int8_t s8_OTAMN_Start (const OTAMN_config_t * pstru_config);

/* Cancels the ongoing OTA update (if any) */
extern int8_t s8_OTAMN_Cancel (void);

#endif /* __APP_OTA_MNGR_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
