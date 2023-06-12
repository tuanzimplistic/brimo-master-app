/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_micropy.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 May 7
**  @brief      : Implementation of Srvc_Micropy module
**  @namespace  : MP
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Micropy
** @brief       Manages MicroPython engine and provides helper APIs for other modules to use MicroPython engine
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_micropy.h"               /* Public header of this module */

#include <string.h>                     /* Use strncpy() */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/task.h"              /* Use FreeRTOS task */
#include "freertos/event_groups.h"      /* Use FreeRTOS event group */

#include "py/stackctrl.h"               /* Use mp_stack_set_top(), mp_stack_set_limit() */
#include "py/runtime.h"                 /* Use main APIs of MicroPython */
#include "py/gc.h"                      /* Use gc_init() */
#include "py/mphal.h"                   /* Use external mp_main_task_handle variable */
#include "shared/readline/readline.h"   /* Use readline_init0() */
#include "shared/runtime/pyexec.h"      /* Use MicroPython pyexec module */
#include "modmachine.h"                 /* Use machine_init(), etc. */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @brief   ID of the CPU that Srvc_Micropy task runs on
** @note    Srvc_Micropy MUST run on core 0 for synchronisation with the ringbuffer and scheduler.
**          Otherwise there can be problem such as memory corruption and so on.
*/
#define MP_TASK_CPU_ID                  0

/** @brief  Stack size (in bytes) of Srvc_Micropy task */
#define MP_TASK_STACK_SIZE              12288

/** @brief  Priority of Srvc_Micropy task */
#define MP_TASK_PRIORITY                (tskIDLE_PRIORITY + 0)

/** @brief  Cycle in milliseconds of Srvc_Micropy task */
#define MP_TASK_PERIOD_MS               50

/** @brief  FreeRTOS event to execute a Python file */
#define MP_FILE_EXECUTE_BIT             (BIT0)

/** @brief  Heap size (in bytes) of MicroPython engine */
#define MP_MICROPYTHON_HEAP_SIZE        (512 * 1024)

/** @brief  Name of the MicroPython script starting WebREPL */
#ifdef CONFIG_MP_WEBREPL_OVER_TLS
#   define MP_WEBREPL_STARTUP_SCRIPT    "_start_webrepl_wss.py"
#else
#   define MP_WEBREPL_STARTUP_SCRIPT    "_start_webrepl.py"
#endif

/** @brief  Name of the script starting Zimplistic_Pyapp */
#define MP_ZIMPLISTIC_PYAPP_STARTUP_SCRIPT   "_start_zimplistic_pyapp.py"

/** @brief  Automatically start WebREPL on bootup */
#ifdef CONFIG_MP_WEBREPL_AUTO_RUN
#   define MP_WEBREPL_AUTO_RUN          true
#else
#   define MP_WEBREPL_AUTO_RUN          false
#endif


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Micropy";

/** @brief  Structure that will hold the TCB of the task being created */
static StaticTask_t g_x_task_buffer;

/** @brief  Buffer that the task being created will use as its stack */
static StackType_t g_x_task_stack [MP_TASK_STACK_SIZE];

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  FreeRTOS event group */
static EventGroupHandle_t g_x_event_group;

/** @brief  Buffer storing path of the Python file to execute */
static char g_stri_file_to_run [MAX_FILE_PATH_LEN];

/** @brief  Indicates if WebREPL service has been started */
static bool g_b_webrepl_started = false;

/** @brief  Indicates if WebREPL service is running */
static bool g_b_webrepl_running = MP_WEBREPL_AUTO_RUN;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static void v_MP_Main_Task (void * pv_param);
static int8_t s8_MP_Init_Env (void * pv_task_sp);

extern int8_t s8_MP_Que_Init (void);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_Micropy module
**
** @return
**      @arg    MP_OK
**      @arg    MP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MP_Init (void)
{
    /* Do nothing if this module has been initialized */
    if (g_b_initialized)
    {
        return MP_OK;
    }

    LOGD ("Initializing Srvc_Micropy module");

    /* Initialise queues between C and MicroPython environments */
    s8_MP_Que_Init ();

    /* Create FreeRTOS event group */
    g_x_event_group = xEventGroupCreate ();

    /* Create task running this module */
    mp_main_task_handle =
        xTaskCreateStaticPinnedToCore ( v_MP_Main_Task,             /* Function that implements the task */
                                        "Srvc_Micropy",             /* Text name for the task */
                                        MP_TASK_STACK_SIZE,         /* Stack size in bytes, not words */
                                        NULL,                       /* Parameter passed into the task */
                                        MP_TASK_PRIORITY,           /* Priority at which the task is created */
                                        g_x_task_stack,             /* Array to use as the task's stack */
                                        &g_x_task_buffer,           /* Variable to hold the task's data structure */
                                        MP_TASK_CPU_ID);            /* ID of the CPU that the task runs on */

    /* Done */
    LOGD ("Initialization of Srvc_Micropy module is done");
    g_b_initialized = true;
    return MP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Executes a Python file
**
** @param [in]
**      pstri_file_path: Path of the Python file to execute
**
** @return
**      @arg    MP_OK
**      @arg    MP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MP_Execute_File (const char * pstri_file_path)
{
    ASSERT_PARAM (g_b_initialized && (pstri_file_path != NULL));

    /* Don't execute the file if WebREPL service is running */
    if (g_b_webrepl_running)
    {
        LOGE ("Failed to execute file because WebREPL mode is enabled.");
        LOGE ("Disable WebREPL by pressing Ctrl+D in WebREPL console and try again.");
        return MP_ERR;
    }

    /* Store path of the file to execute */
    strncpy (g_stri_file_to_run, pstri_file_path, sizeof (g_stri_file_to_run));
    g_stri_file_to_run [sizeof (g_stri_file_to_run) - 1] = 0;

    /* Request the file to be executed */
    xEventGroupSetBits (g_x_event_group, MP_FILE_EXECUTE_BIT);
    return MP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs WebREPL mode
**
** @note
**      To stop WebREPL mode:
**      + Restart ESP32
**      + Press Ctrl-D inside WebREPL console
**
** @note
**      Pressing Ctrl-A inside WebREPL console while WebREPL mode is running switches between raw mode and friendly mode
**
** @note
**      The default password is '123456'. This can be changed in 'srvc_micropy\micropy\esp32\modules\_start_webrepl.py'
**
** @param [in]
**      pstri_file_path: Path of the Python file to execute
**
** @return
**      @arg    MP_OK
**      @arg    MP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MP_Run_WebRepl (void)
{
    ASSERT_PARAM (g_b_initialized);

    /* Run WebREPL service */
    if (!g_b_webrepl_running)
    {
        LOGI ("Run WebREPL service");
        g_b_webrepl_running = true;
    }
    else
    {
        LOGI ("WebREPL is already running");
    }

    return MP_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Task running Srvc_Micropy module
**
** @param [in]
**      pv_param: Parameter passed into the task
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_MP_Main_Task (void * pv_param)
{
    /* Read current stack pointer address */
    void * pv_task_sp = get_sp ();

    LOGD ("Srvc_Micropy task started");

    /* Initialize Python environment */
    s8_MP_Init_Env (pv_task_sp);

    /* Endless loop of the task */
    while (true)
    {
        /* Waiting for task tick or a FreeRTOS event occurs */
        EventBits_t x_event_bits = xEventGroupWaitBits (g_x_event_group, MP_FILE_EXECUTE_BIT, pdTRUE, pdFALSE,
                                                        pdMS_TO_TICKS (MP_TASK_PERIOD_MS));

        /* If a file needs to be executed */
        if ((x_event_bits & MP_FILE_EXECUTE_BIT) &&
            (g_stri_file_to_run[0] != 0))
        {
            LOGI ("Execute Python file %s...", g_stri_file_to_run);
            pyexec_file_if_exists (g_stri_file_to_run);
            g_stri_file_to_run[0] = 0;
        }

        /* Run WebREPL service if requested so */
        if (g_b_webrepl_running)
        {
            /* Start WebREPL service if it's not started yet */
            if (!g_b_webrepl_started)
            {
                LOGI ("Starting WebREPL service...");
                g_b_webrepl_started = true;
                pyexec_frozen_module (MP_WEBREPL_STARTUP_SCRIPT);
            }

            /*
            ** Run WebREPL service
            ** Note that pyexec_friendly_repl() only returns if WebREPL is stopped by pressing Ctrl-D inside WebREPL
            ** console.
            */
            if (pyexec_friendly_repl ())
            {
                g_b_webrepl_running = false;
                LOGI ("Pause WebREPL service");
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
**      Provides extra features for Python environment
**
** @param [in]
**      pv_task_sp: Stack pointer of MicroPython task
**
** @return
**      @arg    MP_OK
**      @arg    MP_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MP_Init_Env (void * pv_task_sp)
{
    /* Initialize MicroPython thread object */
    mp_thread_init (pxTaskGetStackStart (NULL), MP_TASK_STACK_SIZE / sizeof (uintptr_t));

    /* Initialize MicroPython machine module */
    machine_init ();

    /* Allocate MicroPython heap */
    void * pv_micropython_heap = malloc (MP_MICROPYTHON_HEAP_SIZE);
    if (pv_micropython_heap == NULL)
    {
        LOGE ("Failed to allocate heap memory for MicroPython");
        return MP_ERR;
    }

    /* Initialize MicroPython engine */
    mp_stack_set_top (pv_task_sp);
    mp_stack_set_limit (MP_TASK_STACK_SIZE - 1024);
    gc_init (pv_micropython_heap, pv_micropython_heap + MP_MICROPYTHON_HEAP_SIZE);
    mp_init ();
    mp_obj_list_append (mp_sys_path, MP_OBJ_NEW_QSTR (MP_QSTR__slash_lib));
    readline_init0 ();

    /* Run boot-up scripts */
    pyexec_frozen_module ("_boot.py");
    pyexec_file_if_exists ("boot.py");
    
    /* Run startup script of Itor3_Pyapp */
    pyexec_frozen_module (MP_ZIMPLISTIC_PYAPP_STARTUP_SCRIPT);

    return MP_OK;
}

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
