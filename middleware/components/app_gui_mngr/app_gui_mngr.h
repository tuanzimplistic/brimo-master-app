/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : app_gui_mngr.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 May 19
**  @brief      : Public header of App_Gui_Mngr module
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @{
*/

#ifndef __APP_GUI_MNGR_H__
#define __APP_GUI_MNGR_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "common_hdr.h"             /* Use common definitions */
#include "app_gui_mngr_ext.h"       /* Table of GUI binding data */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Status returned by APIs of App_Gui_Mngr module */
enum
{
    GUI_DATA_NOT_CHANGED    = 1,        //!< GUI data is not changed
    GUI_OK                  = 0,        //!< The function executed successfully
    GUI_ERR                 = -1,       //!< There is unknown error while executing the function
    GUI_ERR_NOT_YET_INIT    = -2,       //!< The given instance is not initialized yet
    GUI_ERR_BUSY            = -3,       //!< The function failed because the given instance is busy
};

/** @brief  Expand an entry in GUI_BINDING_DATA_TABLE table as enum of binding data alias */
#define GUI_BINDING_DATA_TABLE_EXPAND_AS_DATA_ENUM(DATA_ID, ...)    DATA_ID,
typedef enum
{
    GUI_BINDING_DATA_TABLE (GUI_BINDING_DATA_TABLE_EXPAND_AS_DATA_ENUM)
    GUI_NUM_DATA

} GUI_data_id_t;

/** @brief  Base type of GUI binding data */
typedef enum
{
    GUI_DATA_TYPE_uint8_t,              //!< unsigned 8-bit integer
    GUI_DATA_TYPE_int8_t,               //!< 8-bit integer
    GUI_DATA_TYPE_uint16_t,             //!< unsigned 16-bit integer
    GUI_DATA_TYPE_int16_t,              //!< 16-bit integer
    GUI_DATA_TYPE_uint32_t,             //!< unsigned 32-bit integer
    GUI_DATA_TYPE_int32_t,              //!< 32-bit integer
    GUI_DATA_TYPE_float,                //!< Single precision floating point number
    GUI_DATA_TYPE_string,               //!< NULL-terminated string
    GUI_DATA_TYPE_blob,                 //!< variable length binary data (blob)

} GUI_data_type_t;

/** @brief  Type of notify and query message */
typedef enum
{
    GUI_MSG_INFO,                       //!< Information message
    GUI_MSG_WARNING,                    //!< Warning message
    GUI_MSG_ERROR,                      //!< Error message

} GUI_msg_t;

/** @brief  Maximum number of query options */
#define GUI_MAX_QUERY_OPTIONS           4

/** @brief  Structure encapsulating a notify message to display on GUI */
typedef struct
{
    /** @brief  Type of notify message */
    GUI_msg_t enm_type;

    /** @brief  Brief description about the notify (NULL-terminated string) */
    const char * pstri_brief;

    /** @brief  Detailed description about the notify (NULL-terminated string) */
    const char * pstri_detail;

    /** @brief  Timeout (in millisecond) waiting for the notify to be acknowledged, 0 for waiting forever */
    uint32_t u32_wait_time;

} GUI_notify_t;

/** @brief  Structure encapsulating a query message to display on GUI */
typedef struct
{
    /** @brief  Type of query message */
    GUI_msg_t enm_type;

    /** @brief  Brief description about the query (NULL-terminated string) */
    const char * pstri_brief;

    /** @brief  Detailed description about the query (NULL-terminated string) */
    const char * pstri_detail;

    /** @brief  Timeout (in millisecond) waiting for an option of the query to be selected, 0 for waiting forever */
    uint32_t u32_wait_time;

    /** @brief  Array of option strings (NULL-terminated strings) */
    const char * apstri_options[GUI_MAX_QUERY_OPTIONS];

    /** @brief  Number of options in apstri_options. Maximum number of options is GUI_MAX_QUERY_OPTIONS */
    uint8_t u8_num_options;

    /** @brief  Index of the option to be selected by default if u32_wait_time expires */
    uint8_t u8_default_option;

} GUI_query_t;

/** @brief  Type of the job that the firmware is performing */
typedef enum
{
    GUI_JOB_SYSTEM,                     //!< Progress of system jobs
    GUI_JOB_APP,                        //!< Progress of user application jobs
    GUI_NUM_JOBS

} GUI_job_t;

/** @brief  Structure encapsulating information of a progress */
typedef struct
{
    /** @brief  Type of the job that the firmware is performing */
    GUI_job_t enm_type;

    /** @brief  Brief description about the progress (NULL-terminated string) */
    const char * pstri_brief;

    /** @brief  Detailed description about the progress (NULL-terminated string) */
    const char * pstri_detail;

    /** @brief  Status description about the progress (NULL-terminated string) */
    const char * pstri_status;

    /** @brief  Min value of the progress */
    int32_t s32_min;

    /** @brief  Max value of the progress */
    int32_t s32_max;

    /**
    ** @brief   Current value of the progress.
    ** @note    The progress disappears if the current progress is not within s32_min and s32_max
    */
    int32_t s32_progress;

} GUI_progress_t;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Initializes App_Gui_Mngr module */
extern int8_t s8_GUI_Init (void);

/* Sets value of a GUI binding data */
extern int8_t s8_GUI_Set_Data (GUI_data_id_t enm_data, const void * pv_data, uint16_t u16_len);

/* Gets value of a GUI binding data */
extern int8_t s8_GUI_Get_Data (GUI_data_id_t enm_data, void * pv_data, uint16_t * pu16_len);

/* Gets value of a GUI binding data if its value has been changed and the new value has never been read before */
extern int8_t s8_GUI_Get_Data_If_Changed (GUI_data_id_t enm_data, void * pv_data, uint16_t * pu16_len);

/* Gets data type of a GUI binding data */
extern int8_t s8_GUI_Get_Data_Type (GUI_data_id_t enm_data, GUI_data_type_t * penm_data_type);

/* Displays a notify message on GUI */
extern int8_t s8_GUI_Notify (const GUI_notify_t * pstru_notify);

/* Displays a message on GUI with some options and wait for user to select an option */
extern int8_t s8_GUI_Query (const GUI_query_t * pstru_query, uint8_t * pu8_selection);

/* Displays progress information of a doing job on GUI */
extern int8_t s8_GUI_Progress (const GUI_progress_t * pstru_progress);

/* Gets elapsed time (in millisecond) since last user activity on GUI */
extern int8_t s8_GUI_Get_Idle_Time (uint32_t * pu32_idle_ms);

/* Triggers a GUI activity (do-nothing) to keep the GUI active */
extern int8_t s8_GUI_Keep_Active (void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __APP_GUI_MNGR_H__ */

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
