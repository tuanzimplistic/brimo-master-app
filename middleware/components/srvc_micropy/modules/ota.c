/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : ota.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Sep 14
**  @brief      : C-implementation of OTA MP module
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Provides API so that MicroPython scripts can triggers over-the-air update for components such as
**              master firmware, a file in master's file system, slave firmware.
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "ota.h"                        /* Public header of this MP module */
#include "app_ota_mngr.h"               /* Use OTA manager to trigger OTA update */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Micropy";

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
**      Triggers an OTA update of master board's firmware
**
** @param [in]
**      x_download_url: URL to download the new firmware for updating. This argument must be a string.
**
** @param [in]
**      x_check_newer: This parameter is a boolean object
**      @arg    true: do the OTA update only if the remote firmware is newer than the current running firmware
**      @arg    false: always do the OTA update
**
** @return
**      @arg    false: failed to trigger the update
**      @arg    true: the OTA update has been started and is running in the background
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Update_Master_Firmware (mp_obj_t x_download_url, mp_obj_t x_check_newer)
{
    /* Validate data type */
    if ((!mp_obj_is_str (x_download_url)) || (!mp_obj_is_bool (x_check_newer)))
    {
        mp_raise_msg (&mp_type_TypeError, "Type of the passed argument(s) is invalid");
        return mp_const_false;
    }

    /* Request OTA manager to start the update */
    OTAMN_config_t stru_ota_cfg =
    {
        .enm_target     = OTAMN_MASTER_FW,
        .pstri_url      = (char *)mp_obj_str_get_str (x_download_url),
        .pstri_inst_dir = "/",
        .b_check_newer  = mp_obj_is_true (x_check_newer),
    };
    if (s8_OTAMN_Start (&stru_ota_cfg) != OTAMN_OK)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to trigger OTA update");
        return mp_const_false;
    }

    /* Successful */
    LOGI ("OTA update of master board's firmware triggered");
    return mp_const_true;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Triggers an OTA update of slave board's firmware
**
** @param [in]
**      x_download_url: URL to download the new firmware for updating. This argument must be a string.
**
** @param [in]
**      x_check_newer: This parameter is a boolean object
**      @arg    true: do the OTA update only if the remote firmware is newer than the current running firmware
**      @arg    false: always do the OTA update
**
** @return
**      @arg    false: failed to trigger the update
**      @arg    true: the OTA update has been started and is running in the background
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Update_Slave_Firmware (mp_obj_t x_download_url, mp_obj_t x_check_newer)
{
    /* Validate data type */
    if ((!mp_obj_is_str (x_download_url)) || (!mp_obj_is_bool (x_check_newer)))
    {
        mp_raise_msg (&mp_type_TypeError, "Type of the passed argument(s) is invalid");
        return mp_const_false;
    }

    /* Request OTA manager to start the update */
    OTAMN_config_t stru_ota_cfg =
    {
        .enm_target     = OTAMN_SLAVE_FW,
        .pstri_url      = (char *)mp_obj_str_get_str (x_download_url),
        .pstri_inst_dir = "/",
        .b_check_newer  = mp_obj_is_true (x_check_newer),
    };
    if (s8_OTAMN_Start (&stru_ota_cfg) != OTAMN_OK)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to trigger OTA update");
        return mp_const_false;
    }

    /* Successful */
    LOGI ("OTA update of slave board's firmware triggered");
    return mp_const_true;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Triggers an OTA update of a file in master board's filesystem
**
** @details
**      This function helps MP scripts start an OTA update for an arbitrary file in filesystem of master board. This
**      function returns immediately, the update continues to run in the background. If the updated file or the
**      folder(s) given in the installation path does not exist, they will be created. If a file with the same name
**      is available at the given path, it will be overwritten by the new file.
**      Example:
**          import ota
**          ota.update_master_file('https://itor3otabucket.s3.ap-southeast-1.amazonaws.com/README.md', '/docs/help.md')
**
** @param [in]
**      x_download_url: URL to download the new firmware for updating. This argument must be a string.
**
** @param [in]
**      x_inst_dir: Path and name in the master board's filesystem where the file is stored and named.
**                  Examples: '/module.mpy', '/dir1/dir2/myfile.txt'
**
** @return
**      @arg    false: failed to trigger the update
**      @arg    true: the OTA update has been started and is running in the background
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Update_Master_File (mp_obj_t x_download_url, mp_obj_t x_inst_dir)
{
    /* Validate data type */
    if ((!mp_obj_is_str (x_download_url)) || (!mp_obj_is_str (x_inst_dir)))
    {
        mp_raise_msg (&mp_type_TypeError, "Type of the passed argument(s) is invalid");
        return mp_const_false;
    }

    /* Request OTA manager to start the update */
    OTAMN_config_t stru_ota_cfg =
    {
        .enm_target     = OTAMN_MASTER_FILE,
        .pstri_url      = (char *)mp_obj_str_get_str (x_download_url),
        .pstri_inst_dir = (char *)mp_obj_str_get_str (x_inst_dir),
        .b_check_newer  = false,
    };
    if (s8_OTAMN_Start (&stru_ota_cfg) != OTAMN_OK)
    {
        mp_raise_msg (&mp_type_OSError, "Failed to trigger OTA update");
        return mp_const_false;
    }

    /* Successful */
    LOGI ("OTA update of file in master board's filesystem triggered");
    return mp_const_true;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Cancels the ongoing OTA update (if any)
**
** @return
**      @arg    None
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
mp_obj_t x_MP_Cancel (void)
{
    /* Request the ongoing OTA update process (if any) to cancel */
    s8_OTAMN_Cancel ();

    /* Successful */
    return mp_const_none;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
