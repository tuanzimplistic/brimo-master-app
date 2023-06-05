/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : app_gui_mngr.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 May 19
**  @brief      : Implementation of App_Gui_Mngr module
**  @namespace  : GUI
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  App_Gui_Mngr
** @brief       Performs frontend processing using LVGL library and provides helper APIs for other modules to interract
**              with the frontend
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "app_gui_mngr.h"               /* Public header of this module */
#include "screen_common.h"              /* Common header of all screens */
#include "control_common.h"             /* Common header of all user controls */
#include "srvc_lvgl.h"                  /* Use LVGL service */
#include "notify_msgbox.h"              /* Use notify message box */
#include "query_msgbox.h"               /* Use query message box */
#include "progress_msgbox.h"            /* Use progress message box */
#include "srvc_recovery.h"              /* Register callback when power supply is lost */

#include <string.h>                     /* Use strlen() */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/task.h"              /* Use FreeRTOS task */
#include "freertos/event_groups.h"      /* Use FreeRTOS event group */
#include "freertos/semphr.h"            /* Use FreeRTOS semaphore */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  ID of the CPU that App_Gui_Mngr task runs on */
#define GUI_TASK_CPU_ID                 0

/** @brief  Stack size (in bytes) of App_Gui_Mngr task */
#define GUI_TASK_STACK_SIZE             8192

/** @brief  Priority of App_Gui_Mngr task */
#define GUI_TASK_PRIORITY               (tskIDLE_PRIORITY + 2)

/** @brief  Cycle in milliseconds of App_Gui_Mngr task */
#define GUI_TASK_PERIOD_MS              10

/** @brief  GUI inactivity time in milliseconds */
#define GUI_INACT_TIME_MS               600000

/** @brief  Cycle in milliseconds to do house keeping jobs */
#define GUI_HOUSE_KEEPING_CYCLE         500

/** @brief  FreeRTOS events */
enum
{
    GUI_NOTIFY_MSG_EVENT    = BIT0,     //!< FreeRTOS event to display notify message
    GUI_QUERY_MSG_EVENT     = BIT1,     //!< FreeRTOS event to display query message
    GUI_PROGRESS_MSG_EVENT  = BIT2,     //!< FreeRTOS event to display progress information
};

/** @brief  Base type of a string data */
#define string                          char

/** @brief  Base type of a blob data */
#define blob                            uint8_t

/** @brief  Specify if a data type is an array type or not */
#define ARRAY_SYMBOL_uint8_t
#define ARRAY_SYMBOL_int8_t
#define ARRAY_SYMBOL_uint16_t
#define ARRAY_SYMBOL_int16_t
#define ARRAY_SYMBOL_uint32_t
#define ARRAY_SYMBOL_int32_t
#define ARRAY_SYMBOL_float
#define ARRAY_SYMBOL_string             []
#define ARRAY_SYMBOL_blob               []

/** @brief  Macro to expand an entry in GUI_BINDING_DATA_TABLE as variable definition */
#define GUI_EXPAND_AS_VARIABLE_DEFINITION(DATA_ID, TYPE, ...)    \
    TYPE GUI_##DATA_ID ARRAY_SYMBOL_##TYPE = __VA_ARGS__;

/** @brief  Structure type to manage a GUI binding data */
typedef struct
{
    GUI_data_type_t     enm_type;       //!< Data type
    void *              pv_data;        //!< Pointer to the data value
    uint16_t            u16_data_len;   //!< Length in bytes of the data value
    bool                b_is_changed;   //!< Indicated if the data has just been changed

} GUI_data_t;

/** @brief  Macros to expand an entry in GUI_BINDING_DATA_TABLE as initialization value for GUI_data_t */
#define GUI_EXPAND_AS_INFO_STRUCT_INIT(DATA_ID, TYPE, ...)      \
{                                                               \
    .enm_type           = GUI_DATA_TYPE_##TYPE,                 \
    .pv_data            = (void *)&GUI_##DATA_ID,               \
    .u16_data_len       = sizeof (GUI_##DATA_ID),               \
    .b_is_changed       = true,                                 \
},

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "App_Gui_Mngr";

/** @brief  Structure that will hold the TCB of the task being created */
static StaticTask_t g_x_task_buffer;

/** @brief  Buffer that the task being created will use as its stack */
static StackType_t g_x_task_stack [GUI_TASK_STACK_SIZE];

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  FreeRTOS event group */
static EventGroupHandle_t g_x_event_group;

/** @brief  Variables of all GUI binding data */
static GUI_BINDING_DATA_TABLE (GUI_EXPAND_AS_VARIABLE_DEFINITION);

/** @brief  Structures manage all binding data */
static GUI_data_t g_astru_data [GUI_NUM_DATA] = { GUI_BINDING_DATA_TABLE (GUI_EXPAND_AS_INFO_STRUCT_INIT) };

/** @brief  Recursive mutex ensuring that there is one task access binding data at a time */
static SemaphoreHandle_t g_x_mutex_binding_data;

/** @brief  Binary semaphore to ensure that there is one task sending notify message at a time */
static SemaphoreHandle_t g_x_sem_notify;

/** @brief  Binary semaphore to ensure that there is one query message at a time */
static SemaphoreHandle_t g_x_sem_query;

/** @brief  Binary semaphore to ensure that there is one task accessing progress information at a time */
static SemaphoreHandle_t g_x_sem_progress;

/** @brief  Buffer storing information of the notify message to display on GUI */
static GUI_notify_t g_stru_notify;

/** @brief  Buffer storing information of the query message to display on GUI */
static GUI_query_t g_stru_query;

/** @brief  Buffer storing information of the progress to display on GUI */
static GUI_progress_t g_stru_progress;

/** @brief  Indicates if the requested notify message has been displayed on GUI */
static bool g_b_notify_displayed = false;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static void v_GUI_Main_Task (void * pv_param);
static void v_GUI_Power_Loss_Handler (void * pv_arg);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes App_Gui_Mngr module
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Init (void)
{
    /* Do nothing if this module has been initialized */
    if (g_b_initialized)
    {
        return GUI_OK;
    }

    LOGD ("Initializing App_Gui_Mngr module");

    /* Allocate memory for GUI binding data of type string and blob */
    for (uint16_t u16_id = 0; u16_id < GUI_NUM_DATA; u16_id++)
    {
        GUI_data_t * pstru_data = &g_astru_data[u16_id];
        if ((pstru_data->enm_type == GUI_DATA_TYPE_string) ||
            (pstru_data->enm_type == GUI_DATA_TYPE_blob))
        {
            void * pv_buf = malloc (pstru_data->u16_data_len);
            if (pv_buf == NULL)
            {
                LOGE ("Failed to allocate memory for GUI binding data");
                return GUI_ERR;
            }

            /* Get init data for the allocated memory */
            memcpy (pv_buf, pstru_data->pv_data, pstru_data->u16_data_len);
            pstru_data->pv_data = pv_buf;
        }
    }

    /* Initialize LVGL service */
    if (s8_LVGL_Init () != LVGL_OK)
    {
        LOGE ("Failed to initialize LVGL service");
        return GUI_ERR;
    }

    /* Create FreeRTOS event group */
    g_x_event_group = xEventGroupCreate ();

    /* Create recursive mutex prevent race condition of binding data */
    g_x_mutex_binding_data = xSemaphoreCreateRecursiveMutex ();
    if (g_x_mutex_binding_data == NULL)
    {
        return GUI_ERR;
    }

    /* Create binary semaphore of notify & query message & progress information */
    g_x_sem_notify = xSemaphoreCreateBinary ();
    g_x_sem_query = xSemaphoreCreateBinary ();
    g_x_sem_progress = xSemaphoreCreateBinary ();
    if ((g_x_sem_notify == NULL) || (g_x_sem_query == NULL) || (g_x_sem_progress == NULL))
    {
        return GUI_ERR;
    }
    xSemaphoreGive (g_x_sem_notify);
    xSemaphoreGive (g_x_sem_query);
    xSemaphoreGive (g_x_sem_progress);

    /* Register handler that will be invoked when power supply is interrupted */
    enm_RCVR_Register_Cb (v_GUI_Power_Loss_Handler, NULL);

    /* Create task running this module */
    xTaskCreateStaticPinnedToCore ( v_GUI_Main_Task,            /* Function that implements the task */
                                    "App_Gui_Mngr",             /* Text name for the task */
                                    GUI_TASK_STACK_SIZE,        /* Stack size in bytes, not words */
                                    NULL,                       /* Parameter passed into the task */
                                    GUI_TASK_PRIORITY,          /* Priority at which the task is created */
                                    g_x_task_stack,             /* Array to use as the task's stack */
                                    &g_x_task_buffer,           /* Variable to hold the task's data structure */
                                    GUI_TASK_CPU_ID);           /* ID of the CPU that App_Gui_Mngr task runs on */

    /* Done */
    LOGD ("Initialization of App_Gui_Mngr module is done");
    g_b_initialized = true;
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets value of a GUI binding data
**
** @param [in]
**      enm_data: Alias of the data to set value
**
** @param [in]
**      pv_data: Data value
**
** @param [in]
**      u16_len: Length (in bytes) of the data, including NULL-character if the binding data is a string.
**               If the binding data is not a blob, this parameter can be zero. In this case, if the binding data is
**               a string, strlen() shall be used to determine its length; otherwise, length of the native data type
**               will be used.
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Set_Data (GUI_data_id_t enm_data, const void * pv_data, uint16_t u16_len)
{
    GUI_data_t * pstru_data = &g_astru_data[enm_data];

    /* Validate parameters */
    ASSERT_PARAM (g_b_initialized && (enm_data < GUI_NUM_DATA) && (pv_data != NULL));

    if ((pstru_data->enm_type == GUI_DATA_TYPE_string) ||
        (pstru_data->enm_type == GUI_DATA_TYPE_blob))
    {
        /* Validate data length */
        if (u16_len == 0)
        {
            if (pstru_data->enm_type == GUI_DATA_TYPE_blob)
            {
                return GUI_ERR;
            }
            else
            {
                u16_len = strlen (pv_data) + 1;
            }
        }

        /* Allocate memory for new data */
        void * pv_buf = malloc (u16_len);
        if (pv_buf == NULL)
        {
            LOGE ("Failed to allocate memory for GUI binding data");
            return GUI_ERR;
        }

        /* Get new data value */
        memcpy (pv_buf, pv_data, u16_len);
        void * pv_old_buf = pstru_data->pv_data;

        /* Protect the binding data from race condition */
        xSemaphoreTakeRecursive (g_x_mutex_binding_data, portMAX_DELAY);
        pstru_data->pv_data = pv_buf;
        pstru_data->u16_data_len = u16_len;
        pstru_data->b_is_changed = true;
        xSemaphoreGiveRecursive (g_x_mutex_binding_data);

        /* Free the memory containing old value */
        free (pv_old_buf);
    }
    else
    {
        /* Validate data length */
        if (u16_len == 0)
        {
            u16_len = pstru_data->u16_data_len;
        }
        else if (u16_len != pstru_data->u16_data_len)
        {
            return GUI_ERR;
        }

        /* Protect the binding data from race condition */
        xSemaphoreTakeRecursive (g_x_mutex_binding_data, portMAX_DELAY);
        memcpy (pstru_data->pv_data, pv_data, u16_len);
        pstru_data->b_is_changed = true;
        xSemaphoreGiveRecursive (g_x_mutex_binding_data);
    }

    /* Done */
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of a GUI binding data
**
** @note
**      If the binding data is a string or blob and the provided buffer is not sufficient to store its value, part
**      of the data shall be stored in the buffer. If the binding data is a string, it shall be always terminated by
**      a NULL-character.
**
** @param [in]
**      enm_data: Alias of the data to get value
**
** @param [out]
**      pv_data: Container of the data value
**               This parameter can be NULL. In this case, pu16_len shall contain length of the binding data
**
** @param [in, out]
**      pu16_len: [Input] Length (in bytes) of the buffer pointed to by pv_data
**                [Output] Length (in bytes) of the data contained in the buffer
**                This parameter can be NULL. In this case, the whole binding data value shall be copied to pv_data.
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Get_Data (GUI_data_id_t enm_data, void * pv_data, uint16_t * pu16_len)
{
    GUI_data_t * pstru_data = &g_astru_data[enm_data];

    /* Validate parameters */
    ASSERT_PARAM (g_b_initialized && (enm_data < GUI_NUM_DATA));

    /* If the caller only wants to get length of the binding data's value */
    if (pv_data == NULL)
    {
        if (pu16_len != NULL)
        {
            *pu16_len = pstru_data->u16_data_len;
        }
        return GUI_OK;
    }

    /* Get value of the binding data */
    if ((pstru_data->enm_type == GUI_DATA_TYPE_string) ||
        (pstru_data->enm_type == GUI_DATA_TYPE_blob))
    {
        /* Protect the binding data from race condition */
        xSemaphoreTakeRecursive (g_x_mutex_binding_data, portMAX_DELAY);

        /*
        ** For a string or blob binding data, if the provided buffer is not sufficient to store its value, part
        ** of the data shall be stored in the buffer. If the binding data is a string, it shall be always terminated by
        ** a NULL-character.
        */
        uint16_t u16_len = pstru_data->u16_data_len;
        if ((pu16_len != NULL) && (*pu16_len < u16_len))
        {
            u16_len = *pu16_len;
        }
        memcpy (pv_data, pstru_data->pv_data, u16_len);
        xSemaphoreGiveRecursive (g_x_mutex_binding_data);
        if (pstru_data->enm_type == GUI_DATA_TYPE_string)
        {
            ((uint8_t *)pv_data)[u16_len - 1] = '\0';
        }
    }
    else
    {
        /* Validate data length */
        if ((pu16_len != NULL) && (*pu16_len < pstru_data->u16_data_len))
        {
            return GUI_ERR;
        }

        /* Protect the binding data from race condition */
        xSemaphoreTakeRecursive (g_x_mutex_binding_data, portMAX_DELAY);
        memcpy (pv_data, pstru_data->pv_data, pstru_data->u16_data_len);
        xSemaphoreGiveRecursive (g_x_mutex_binding_data);
    }

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets value of a GUI binding data if its data has been changed and the new value has never been read before
**      by this function.
**
** @note
**      This function only works properly if the GUI data being read has only one reader using this function. If there
**      are multiple reader read the same GUI data with this function, only the first reader obtain the data value,
**      the others don't.
**
** @note
**      If the binding data is a string or blob and the provided buffer is not sufficient to store its value, part
**      of the data shall be stored in the buffer. If the binding data is a string, it shall be always terminated by
**      a NULL-character.
**
** @param [in]
**      enm_data: Alias of the data to get value
**
** @param [out]
**      pv_data: Container of the data value
**               This parameter can be NULL. In this case, pu16_len shall contain length of the binding data
**
** @param [in, out]
**      pu16_len: [Input] Length (in bytes) of the buffer pointed to by pv_data
**                [Output] Length (in bytes) of the data contained in the buffer
**                This parameter can be NULL. In this case, the whole binding data value shall be copied to pv_data.
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**      @arg    GUI_DATA_NOT_CHANGED
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Get_Data_If_Changed (GUI_data_id_t enm_data, void * pv_data, uint16_t * pu16_len)
{
    int8_t          result = GUI_OK;
    GUI_data_t *    pstru_data = &g_astru_data[enm_data];

    /* Validate parameters */
    ASSERT_PARAM (g_b_initialized && (enm_data < GUI_NUM_DATA));

    /* Protect the binding data from race condition */
    xSemaphoreTakeRecursive (g_x_mutex_binding_data, portMAX_DELAY);

    /* Check if the data has been changed recently */
    if (pstru_data->b_is_changed)
    {
        if (s8_GUI_Get_Data (enm_data, pv_data, pu16_len) == GUI_OK)
        {
            pstru_data->b_is_changed = false;
        }
        else
        {
            result = GUI_ERR;
        }
    }
    else
    {
        result = GUI_DATA_NOT_CHANGED;
    }

    /* Done */
    xSemaphoreGiveRecursive (g_x_mutex_binding_data);
    return result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets data type of a GUI binding data
**
** @param [in]
**      enm_data: Alias of the data to get data type
**
** @param [out]
**      penm_data_type: Type of the given binding data
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Get_Data_Type (GUI_data_id_t enm_data, GUI_data_type_t * penm_data_type)
{
    GUI_data_t * pstru_data = &g_astru_data[enm_data];

    /* Validate parameters */
    ASSERT_PARAM (g_b_initialized && (enm_data < GUI_NUM_DATA) && (penm_data_type != NULL));

    /* Return data type of the given binding data */
    *penm_data_type = pstru_data->enm_type;
    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Displays a notify message on GUI
**
** @note
**      The function is asynchonous, i.e. it returns as soon as the notify message is displayed on GUI, it does not
**      wait for the notify message to be acknowledged by user.
**
** @param [in]
**      pstru_notify: information of the notify message to display
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Notify (const GUI_notify_t * pstru_notify)
{
    /* Validate parameters */
    ASSERT_PARAM (g_b_initialized && (pstru_notify != NULL) &&
                  (pstru_notify->pstri_brief != NULL) && (pstru_notify->pstri_detail != NULL));

    /* Ensure that new notify message can only be displayed if the previous one has been displayed */
    xSemaphoreTake (g_x_sem_notify, portMAX_DELAY);

    /* Manually trigger an activity on GUI display */
    lv_disp_trig_activity (NULL);

    /* Store the given notify message */
    g_stru_notify = *pstru_notify;
    g_b_notify_displayed = false;

    /* Request GUI manager to display the message */
    xEventGroupSetBits (g_x_event_group, GUI_NOTIFY_MSG_EVENT);

    /* Wait until the notify message is displayed on GUI */
    do
    {
        vTaskDelay (pdMS_TO_TICKS (50));
    }
    while (!g_b_notify_displayed);

    /* Done, the notify message box is available now */
    xSemaphoreGive (g_x_sem_notify);

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Displays a message on GUI with some options and wait for user to select an option
**
** @note
**      The function is synchonous, i.e. it only returns when user selects an option provided or the wait time of
**      the query expires.
**
** @note
**      If multiple tasks concurrently send queries while another query is being served, they are blocked waiting
**      for each query is served, one by one.
**
** @param [in]
**      pstru_query: information of the query message to display
**
** @param [out]
**      pu8_selection: index of the option that user selects
**                     This parameter can be NULL.
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Query (const GUI_query_t * pstru_query, uint8_t * pu8_selection)
{
    /* Validate parameters */
    ASSERT_PARAM (g_b_initialized && (pstru_query != NULL) &&
                  (pstru_query->pstri_brief != NULL) && (pstru_query->pstri_detail != NULL));

    /* Ensure that there is one task using query message box at a time */
    xSemaphoreTake (g_x_sem_query, portMAX_DELAY);

    /* Manually trigger an activity on GUI display */
    lv_disp_trig_activity (NULL);

    /* Store the given query message */
    g_stru_query = *pstru_query;

    /* Clear user selection */
    int8_t s8_option = -1;
    s8_GUI_Set_Data (GUI_DATA_USER_QUERY, &s8_option, sizeof (s8_option));

    /* Request GUI manager to display the message */
    xEventGroupSetBits (g_x_event_group, GUI_QUERY_MSG_EVENT);

    /* Wait until user selects an option */
    do
    {
        s8_GUI_Get_Data (GUI_DATA_USER_QUERY, &s8_option, NULL);
        vTaskDelay (pdMS_TO_TICKS (50));
    }
    while (s8_option < 0);

    /* Get user selection */
    if (pu8_selection != NULL)
    {
        *pu8_selection = s8_option;
    }

    /* Done, the query message box is available now */
    xSemaphoreGive (g_x_sem_query);

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Displays progress information of a doing job on GUI.
**
** @note
**      The function is asynchonous, i.e. it returns as soon as the progress information is displayed on GUI.
**      To dispose the progress display, call this function again with progress value is outside of min and max range.
**
** @param [in]
**      pstru_progress: information of the protgess to display
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Progress (const GUI_progress_t * pstru_progress)
{
    /* Validate parameters */
    ASSERT_PARAM (g_b_initialized && (pstru_progress != NULL) && (pstru_progress->pstri_brief != NULL) &&
                  (pstru_progress->pstri_detail != NULL) && (pstru_progress->pstri_status != NULL));

    /* Manually trigger an activity on GUI display */
    lv_disp_trig_activity (NULL);

    /* Store the given progress information */
    xSemaphoreTake (g_x_sem_progress, portMAX_DELAY);
    g_stru_progress = *pstru_progress;
    xSemaphoreGive (g_x_sem_progress);

    /* Request GUI manager to display the progress */
    xEventGroupSetBits (g_x_event_group, GUI_PROGRESS_MSG_EVENT);

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets elapsed time (in millisecond) since last user activity on GUI
**
** @param [out]
**      pu32_idle_ms: pointer to the variable to store inactivity time
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Get_Idle_Time (uint32_t * pu32_idle_ms)
{
    /* Validate parameters */
    ASSERT_PARAM (g_b_initialized && (pu32_idle_ms != NULL));

    /* Get GUI inactivity time */
    *pu32_idle_ms = lv_disp_get_inactive_time (NULL);

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Keeps GUI display active regardless whether there is user activity on GUI or not
**
** @note
**      This function should be called frequently (less than the time specified by GUI_INACT_TIME_MS) to keep GUI
**      display active in case there is no user activity
**
** @return
**      @arg    GUI_OK
**      @arg    GUI_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GUI_Keep_Active (void)
{
    /* Validate parameters */
    ASSERT_PARAM (g_b_initialized);

    /* Manually trigger an activity on GUI display */
    lv_disp_trig_activity (NULL);

    return GUI_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Task running App_Gui_Mngr module
**
** @param [in]
**      pv_param: Parameter passed into the task
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Main_Task (void * pv_param)
{
    EventBits_t     x_event_wait_bits = GUI_NOTIFY_MSG_EVENT | GUI_QUERY_MSG_EVENT | GUI_PROGRESS_MSG_EVENT;
    GUI_screen_t *  pstru_screen = NULL;
    TickType_t      x_house_keeping_timer = 0;
    TickType_t      x_lvgl_timer = 0;

	/* Because GUI manager has high priority, wait here a bit for other tasks has the chance to stat */
	vTaskDelay (pdMS_TO_TICKS (100));

    /* Splash screen is the first screen to display */
    s8_GUI_Get_Screen (GUI_SCREEN_SPLASH, &pstru_screen);
    lv_scr_load (pstru_screen->px_lv_screen);

    /* Start and display the screen */
    if (pstru_screen->pfnc_start != NULL)
    {
        pstru_screen->pfnc_start ();
    }

    /* Endless loop of the task */
    while (true)
    {
        /* Waiting for task tick or a FreeRTOS event occurs */
        EventBits_t x_event_bits =
            xEventGroupWaitBits (g_x_event_group,           /* The event group to test the bits */
                                 x_event_wait_bits,         /* Bits to test */
                                 pdTRUE,                    /* If the tested bits are automatically cleared on exit */
                                 pdFALSE,                   /* Whether to wait for all test bits to be set */
                                 pdMS_TO_TICKS (GUI_TASK_PERIOD_MS));

        /* If a notify message is requested to be displayed on GUI */
        if (x_event_bits & GUI_NOTIFY_MSG_EVENT)
        {
            /* Display the message */
            s8_GUI_Show_Notify_Msgbox (&g_stru_notify);
            g_b_notify_displayed = true;
        }

        /* If a query message is requested to be displayed on GUI */
        if (x_event_bits & GUI_QUERY_MSG_EVENT)
        {
            /* Display the message */
            s8_GUI_Show_Query_Msgbox (&g_stru_query);
        }

        /* If a progress information is requested to be displayed on GUI */
        if (x_event_bits & GUI_PROGRESS_MSG_EVENT)
        {
            /* Display the progress */
            xSemaphoreTake (g_x_sem_progress, portMAX_DELAY);
            s8_GUI_Show_Progress_Msgbox (&g_stru_progress);
            xSemaphoreGive (g_x_sem_progress);
        }

        /* Run the current screen displayed */
        if (pstru_screen->pfnc_run != NULL)
        {
            pstru_screen->pfnc_run ();
        }

        /* Run all user controls if required */
        for (uint8_t u8_control_idx = 0; u8_control_idx < GUI_NUM_CONTROLS; u8_control_idx++)
        {
            GUI_control_t * pstru_control;
            if ((s8_GUI_Get_Control (u8_control_idx, &pstru_control) == GUI_OK) &&
                (pstru_control != NULL) && (pstru_control->pfnc_run != NULL))
            {
                pstru_control->pfnc_run ();
            }
        }

        /* Run LVGL service */
        TickType_t x_ticks_elapsed = TIMER_ELAPSED (x_lvgl_timer);
        TIMER_RESET (x_lvgl_timer);
        s8_LVGL_Run (TIMER_TICKS_TO_MS (x_ticks_elapsed));

        /* Check result of the current screen */
        if (pstru_screen->enm_result != GUI_SCREEN_RESULT_NONE)
        {
            /* The current screen has been done its job, stop it */
            if (pstru_screen->pfnc_stop != NULL)
            {
                pstru_screen->pfnc_stop ();
            }

            /* Determine the next screen to display */
            if ((pstru_screen->enm_result == GUI_SCREEN_RESULT_NEXT) &&
                (pstru_screen->pstru_next != NULL))
            {
                pstru_screen->pstru_next->pstru_prev = pstru_screen;
                pstru_screen = pstru_screen->pstru_next;
            }
            else if ((pstru_screen->enm_result == GUI_SCREEN_RESULT_BACK) &&
                     (pstru_screen->pstru_prev != NULL))
            {
                pstru_screen = pstru_screen->pstru_prev;
            }
            else
            {
                /* Should not be here */
                s8_GUI_Get_Screen (GUI_SCREEN_SPLASH, &pstru_screen);
            }

            /* Display the next screen and start it */
            lv_scr_load (pstru_screen->px_lv_screen);
            if (pstru_screen->pfnc_start != NULL)
            {
                pstru_screen->pfnc_start ();
            }
        }

        /* Run house keeping jobs */
        if (TIMER_ELAPSED (x_house_keeping_timer) >= pdMS_TO_TICKS (GUI_HOUSE_KEEPING_CYCLE))
        {
            TIMER_RESET (x_house_keeping_timer);

            /* If GUI is inactive for a predefined period of time, put GUI into idle mode */
            if (lv_disp_get_inactive_time (NULL) >= GUI_INACT_TIME_MS)
            {
                s8_LVGL_Set_Idle_Mode (true);
            }
            else
            {
                s8_LVGL_Set_Idle_Mode (false);
            }
        }

        /* Display remaining stack space every 30s */
        // PRINT_STACK_USAGE (30000);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handles power interruption event
**
** @param [in]
**      pv_arg: Context data passed when this handler was registered
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GUI_Power_Loss_Handler (void * pv_arg)
{
    /* Notify the power interruption on the LCD */
    GUI_notify_t stru_notify =
    {
        .enm_type       = GUI_MSG_WARNING,
        .pstri_brief    = "Power interrupted",
        .pstri_detail   = "AC power supply is interrupted. Saving state...",
        .u32_wait_time  = 0
    };
    s8_GUI_Notify (&stru_notify);
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
