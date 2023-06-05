/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_touch_gt911.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 28
**  @brief      : Implementation of Srvc_Touch_GT911 module
**  @namespace  : GT911
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup      Srvc_Touch_GT911
** @brief           Encapsulates touch controller GT911 on Itor3 hardware (EB1.1 master board)
**
** @details
** <b> Instruction to use this module: </b>
**
** 1) Get the single instance of the GT911 touch controller with s8_GT911_Get_Inst(). When this function is called
**    the first time, the GT911 controller shall be initialized.
**
** 2) Whenener needed, call s8_GT911_Get_Touch() to get coordinate of the touch position. If there is no touch,
**    X and/or Y component of the position shall be -1.
**
** Note that the module doesn't support multi-touch although GT911 controller itself does (5 points).
**
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_touch_gt911.h"       /* Public header of this module */
#include "srvc_io_tca9534.h"        /* TCA9534A is used to control GT911 */
#include "hwa_i2c_master.h"         /* I2C Master is used to communicate with GT911 */
#include "hwa_gpio.h"               /* GPIO is used to for GT911's INT signal */

#include "freertos/FreeRTOS.h"      /* Use FreeRTOS */
#include "freertos/task.h"          /* Use FreeRTOS task */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Structure to manage a GT911 controller object */
struct GT911_obj
{
    bool                b_initialized;          //!< Specifies whether the object has been initialized or not
    I2C_inst_t          x_i2c_master;           //!< Instance of the I2C Master used to communicate with GT911
    GPIOX_inst_t        x_gpiox_pwr;            //!< Instance of the expanded GPIO pin controlling power supply of GT911
    GPIOX_inst_t        x_gpiox_reset;          //!< Instance of the expanded GPIO pin used to reset GT911
    GPIO_inst_t         x_gpio_int;             //!< Instance of the GPIO pin connected to INT signal of GT911
    int16_t             s16_touch_x;            //!< X-coordinate of touch position, -1 if no touch
    int16_t             s16_touch_y;            //!< Y-coordinate of touch position, -1 if no touch
};

/** @brief  Some registers of GT911 */
enum
{
    GT911_REG_CONFIG_VERSION        = 0x8047,   //!< This is the first register of configuration area
    GT911_REG_CONFIG_FRESH          = 0x8100,   //!< Configuration updated flag
    GT911_REG_X_COORDINATE_1        = 0x8150,   //!< X-coordinate of point 1
    GT911_REG_TOUCH_STATUS          = 0x814E,   //!< Touch status
};

/** @brief   ID of the CPU that Srvc_Touch_GT911 task runs on */
#define GT911_TASK_CPU_ID           1

/** @brief  Stack size (in bytes) of Srvc_Touch_GT911 task */
#define GT911_TASK_STACK_SIZE       4096

/** @brief  Priority of Srvc_Touch_GT911 task */
#define GT911_TASK_PRIORITY         (tskIDLE_PRIORITY + 1)

/** @brief  FreeRTOS event fired when there is falling/rising edge of GT911's INT signal */
#define GT911_INT_EDGE_DETECTED     (BIT0)

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Touch_GT911";

/** @brief  Structure that will hold the TCB of the task being created */
static StaticTask_t g_x_task_buffer;

/** @brief  Buffer that the task being created will use as its stack */
static StackType_t g_x_task_stack [GT911_TASK_STACK_SIZE];

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Single instance of GT911 controller */
static struct GT911_obj g_stru_gt911_obj =
{
    .b_initialized          = false,
    .x_i2c_master           = NULL,
    .x_gpiox_pwr            = NULL,
    .x_gpiox_reset          = NULL,
    .x_gpio_int             = NULL,
    .s16_touch_x            = -1,
    .s16_touch_y            = -1,
};

/** @brief  Handle of the task of this module */
static TaskHandle_t g_x_gt911_task;

/** @brief  Configuration values of GT911 registers starting from the register at 0x8047 */
static uint8_t g_au8_gt911_cfg[] =
{
    0x46, 0x40, 0x01, 0xE0, 0x01, 0x01, 0x05, 0x00, 0x01, 0x08,
    0x28, 0x05, 0x28, 0x20, 0x03, 0x05, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87, 0x28, 0x0A,
    0x17, 0x15, 0x31, 0x0D, 0x00, 0x00, 0x02, 0xBD, 0x04, 0x24,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x64, 0x32, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10,
    0x12, 0x14, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x26,
    0x24, 0x22, 0x21, 0x20, 0x1F, 0x1E, 0x1D, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifdef USE_MODULE_ASSERT
 static bool b_GT911_Is_Valid_Inst (GT911_inst_t x_inst);
#endif

static int8_t s8_GT911_Init_Module (void);
static int8_t s8_GT911_Init_Inst (GT911_inst_t x_inst);
static void v_GT911_Main_Task (void * pv_param);
static void v_GT911_Int_Handler (GPIO_evt_data_t * pstru_evt_data);
static int8_t s8_GT911_Write_Regs (GT911_inst_t x_inst, uint16_t u16_start_reg, uint16_t u16_num_regs,
                                   const uint8_t * pu8_values);
static int8_t s8_GT911_Read_Regs (GT911_inst_t x_inst, uint16_t u16_start_reg, uint16_t u16_num_regs,
                                  uint8_t * pu8_values);
static uint8_t u8_GT911_Calc_LRC (const uint8_t * pu8_data, uint16_t u16_len);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets the single instance of GT911 controller.
**
** @param [out]
**      px_inst: Container to store the retrieved instance
**
** @return
**      @arg    GT911_OK
**      @arg    GT911_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GT911_Get_Inst (GT911_inst_t * px_inst)
{
    GT911_inst_t    x_inst = NULL;
    int8_t          s8_result = GT911_OK;

    /* Validation */
    ASSERT_PARAM (px_inst != NULL);

    /* Initialize */
    *px_inst = NULL;

    /* If this module has not been initialized, do that now */
    if (s8_result >= GT911_OK)
    {
        if (!g_b_initialized)
        {
            s8_result = s8_GT911_Init_Module ();
            if (s8_result >= GT911_OK)
            {
                g_b_initialized = true;
            }
        }
    }

    /* If the retrieved instance has not been initialized yet, do that now */
    if (s8_result >= GT911_OK)
    {
        x_inst = &g_stru_gt911_obj;
        if (!x_inst->b_initialized)
        {
            s8_result = s8_GT911_Init_Inst (x_inst);
            if (s8_result >= GT911_OK)
            {
                x_inst->b_initialized = true;
            }
        }
    }

    /* Return instance of the GT911 controller */
    if (s8_result >= GT911_OK)
    {
        *px_inst = x_inst;
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets coordinates of current touch position. There is no touch if X or Y coordinate is -1
**
** @param [in]
**      x_inst: A specific GT911 instance
**
** @param [out]
**      ps16_touch_x: Container of X-coordinate, -1 if there is no touch
**
** @param [out]
**      ps16_touch_y: Container of Y-coordinate, -1 if there is no touch
**
** @return
**      @arg    GT911_OK
**      @arg    GT911_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GT911_Get_Touch (GT911_inst_t x_inst, int16_t * ps16_touch_x, int16_t * ps16_touch_y)
{
    /* Validation */
    ASSERT_PARAM (b_GT911_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM ((ps16_touch_x != NULL) && (ps16_touch_y != NULL));

    /* Touch coordinates */
    *ps16_touch_x = x_inst->s16_touch_x;
    *ps16_touch_y = x_inst->s16_touch_y;

    return GT911_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_Touch_GT911 module
**
** @return
**      @arg    GT911_OK
**      @arg    GT911_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GT911_Init_Module (void)
{
    /* Create task running this module */
    g_x_gt911_task =
        xTaskCreateStaticPinnedToCore ( v_GT911_Main_Task,          /* Function that implements the task */
                                        "Srvc_Touch_GT911",         /* Text name for the task */
                                        GT911_TASK_STACK_SIZE,      /* Stack size in bytes, not words */
                                        NULL,                       /* Parameter passed into the task */
                                        GT911_TASK_PRIORITY,        /* Priority at which the task is created */
                                        g_x_task_stack,             /* Array to use as the task's stack */
                                        &g_x_task_buffer,           /* Variable to hold the task's data structure */
                                        GT911_TASK_CPU_ID);         /* ID of the CPU that the task runs on */

    return GT911_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes GT911 controller
**
** @param [in]
**      x_inst: A specific GT911 instance
**
** @return
**      @arg    GT911_OK
**      @arg    GT911_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GT911_Init_Inst (GT911_inst_t x_inst)
{
    /* Get instance of the I2C Master that used to communicate with GT911 */
    if (s8_I2C_Get_Inst (I2C_GT911, &x_inst->x_i2c_master) != I2C_OK)
    {
        return GT911_ERR;
    }

    /* Get instance of the expanded GPIO pin controlling power supply of the LCD */
    if (s8_GPIOX_Get_Inst (GPIOX_LCD_CAM_PWR, &x_inst->x_gpiox_pwr) != GPIOX_OK)
    {
        return GT911_ERR;
    }

    /* Get instance of the expanded GPIO connected to Reset signal of GT911 */
    if (s8_GPIOX_Get_Inst (GPIOX_TOUCH_RST, &x_inst->x_gpiox_reset) != GPIOX_OK)
    {
        return GT911_ERR;
    }

    /* Get instance of the GPIO connected to INT signal of GT911 */
    if (s8_GPIO_Get_Inst (GPIO_TOUCH_INT, &x_inst->x_gpio_int) != GPIO_OK)
    {
        return GT911_ERR;
    }

    /* Initialize touch coordinates */
    x_inst->s16_touch_x = -1;
    x_inst->s16_touch_y = -1;

    /* Turn on power supply of GT911 */
    if (s8_GPIOX_Write_Active (x_inst->x_gpiox_pwr, true) != GPIOX_OK)
    {
        return GT911_ERR;
    }

    /* Configure address of GT911 to 0x5D (see section 4.2 of "GT911 Programming Guide" document) */

    /* 1) Keep both Reset and INT signals at 0 for at least 100 us */
    s8_GPIO_Change_Dir (x_inst->x_gpio_int, GPIO_DIR_OUTPUT);
    s8_GPIOX_Write_Level (x_inst->x_gpiox_reset, 0);
    s8_GPIO_Write_Level (x_inst->x_gpio_int, 0);
    vTaskDelay (pdMS_TO_TICKS (10));

    /* 2) Set Reset signal to 1 while keeping INT signal at 0 for at least 5 ms */
    s8_GPIOX_Write_Level (x_inst->x_gpiox_reset, 1);
    vTaskDelay (pdMS_TO_TICKS (60));

    /* 3) Set INT floating */
    s8_GPIO_Change_Dir (x_inst->x_gpio_int, GPIO_DIR_INPUT);

    /* Calculate checksum of configuration data */
    g_au8_gt911_cfg [sizeof (g_au8_gt911_cfg) - 1] = u8_GT911_Calc_LRC (g_au8_gt911_cfg, sizeof (g_au8_gt911_cfg) - 1);

    /* Configure registers of GT911 */
    if (s8_GT911_Write_Regs (x_inst, GT911_REG_CONFIG_VERSION, sizeof (g_au8_gt911_cfg), g_au8_gt911_cfg) != GT911_OK)
    {
        return GT911_ERR;
    }

    /* Tell GT911 to apply the configuration */
    uint8_t u8_update_flag = 1;
    if (s8_GT911_Write_Regs (x_inst, GT911_REG_CONFIG_FRESH, 1, &u8_update_flag) != GT911_OK)
    {
        return GT911_ERR;
    }

    /*
    ** When touched, GT911 sends a pulse via INT pin in every scanning cycle to notify the host to read coordinates.
    ** Enable GPIO interrupt triggered when there is falling edge at INT pin.
    */
    if (s8_GPIO_Enable_Interrupt (x_inst->x_gpio_int, GPIO_INT_FALLING_EDGE, v_GT911_Int_Handler, NULL) != GPIO_OK)
    {
        return GT911_ERR;
    }

    return GT911_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Task running Srvc_Touch_GT911 module
**
** @param [in]
**      pv_param: Parameter passed into the task
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GT911_Main_Task (void * pv_param)
{
    const uint32_t  u32_bits_to_clear_on_entry = 0x00000000;
    const uint32_t  u32_bits_to_clear_on_exit = 0xFFFFFFFF;
    uint32_t        u32_notify_value;

    /* Endless loop of the task */
    while (true)
    {
        /* Wait until TCA9534A detects an edge on any of its input pins, including INT pin of GT911 */
        xTaskNotifyWait (u32_bits_to_clear_on_entry, u32_bits_to_clear_on_exit, &u32_notify_value, portMAX_DELAY);

        /* If the touch screen is touched */
        if (u32_notify_value & GT911_INT_EDGE_DETECTED)
        {
            GT911_inst_t x_inst = &g_stru_gt911_obj;

            /* Read touch status to determine if we have a touch */
            uint8_t u8_status;
            if ((s8_GT911_Read_Regs (x_inst, GT911_REG_TOUCH_STATUS, 1, &u8_status) != GT911_OK) ||
                (!(u8_status & BIT7)))
            {
                x_inst->s16_touch_x = -1;
                x_inst->s16_touch_y = -1;
                continue;
            }

            /* Number of touch points (4-bit LSB) */
            uint8_t u8_num_touch = u8_status & 0x0F;
            if (u8_num_touch > 0)
            {
                /* Read touch coordinates */
                uint8_t au8_values[4];
                if (s8_GT911_Read_Regs (x_inst, GT911_REG_X_COORDINATE_1, sizeof (au8_values), au8_values) == GT911_OK)
                {
                    x_inst->s16_touch_x = au8_values[0] + (au8_values[1] * 256);
                    x_inst->s16_touch_y = au8_values[2] + (au8_values[3] * 256);
                }
                else
                {
                    x_inst->s16_touch_x = -1;
                    x_inst->s16_touch_y = -1;
                }
            }
            else
            {
                /* No touch */
                x_inst->s16_touch_x = -1;
                x_inst->s16_touch_y = -1;
            }

            /* Clear status register to receive next touch coordinate */
            uint8_t u8_zero = 0;
            s8_GT911_Write_Regs (x_inst, GT911_REG_TOUCH_STATUS, 1, &u8_zero);
        }

        /* Display remaining stack space every 30s */
        // PRINT_STACK_USAGE (30000);
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler invoked when there is a falling edge at GT911's INT pin (user touches the LCD screen)
**
** @note
**      This function is called under the context of GPIO interrupt
**
** @param [in]
**      pstru_evt_data: Context data of the event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GT911_Int_Handler (GPIO_evt_data_t * pstru_evt_data)
{
    BaseType_t x_higher_priority_task_woken;

    /* Notify Srvc_Touch_GT911 task to wake up immediately to check touch status from GT911 */
    xTaskNotifyFromISR (g_x_gt911_task, GT911_INT_EDGE_DETECTED, eSetBits, &x_higher_priority_task_woken);

    /* If Srvc_Touch_GT911 task has higher priority than the currently running task */
    if (x_higher_priority_task_woken)
    {
        /* Request a context switch before the interrupt handler exits */
        portYIELD_FROM_ISR ();
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Writes value of consecutive registers, starting from a given register
**
** @param [in]
**      x_inst: A specific GT911 instance
**
** @param [in]
**      u16_start_reg: Address of the register starting to write
**
** @param [in]
**      u16_num_regs: Number of registers to write
**
** @param [in]
**      pu8_values: Pointer to array of the register values to write, pu8_data[0] is value of the 1st register,
**                  pu8_data[1] is value of the 2st register, and so on
**
** @return
**      @arg    GT911_OK
**      @arg    GT911_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GT911_Write_Regs (GT911_inst_t x_inst, uint16_t u16_start_reg, uint16_t u16_num_regs,
                                   const uint8_t * pu8_values)
{
    /* Invert 2 bytes of u16_start_reg from little endian to big endian */
    u16_start_reg = ENDIAN_GET16_BE (&u16_start_reg);

    /* Write the registers with their values */
    if (s8_I2C_Write_Mem (x_inst->x_i2c_master, (uint8_t *)&u16_start_reg, sizeof (u16_start_reg),
                                                pu8_values, u16_num_regs) != I2C_OK)
    {
        return GT911_ERR;
    }

    return GT911_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Writes value of consecutive registers, starting from a given register
**
** @param [in]
**      x_inst: A specific GT911 instance
**
** @param [in]
**      u16_start_reg: Address of the register starting to read
**
** @param [in]
**      u16_num_regs: Number of registers to read
**
** @param [in]
**      pu8_values: Container of the read values, pu8_data[0] is value of the 1st register, pu8_data[1] is value of
**                  the 2st register, and so on
**
** @return
**      @arg    GT911_OK
**      @arg    GT911_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GT911_Read_Regs (GT911_inst_t x_inst, uint16_t u16_start_reg, uint16_t u16_num_regs,
                                  uint8_t * pu8_values)
{
    /* Invert 2 bytes of u16_start_reg from little endian to big endian */
    u16_start_reg = ENDIAN_GET16_BE (&u16_start_reg);

    /* Read values from the registers */
    if (s8_I2C_Read_Mem (x_inst->x_i2c_master, (uint8_t *)&u16_start_reg, sizeof (u16_start_reg),
                                               pu8_values, u16_num_regs) != I2C_OK)
    {
        return GT911_ERR;
    }

    return GT911_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Calculate LRC checksum for a block of data
**
** @param [in]
**      pu8_data: The data block to calculate LRC checksum
**
** @param [in]
**      u16_len: Length in byte of the data block
**
** @return
**      8-bit LRC checksum
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static uint8_t u8_GT911_Calc_LRC (const uint8_t * pu8_data, uint16_t u16_len)
{
    uint8_t u8_lrc = 0;
    for (uint16_t u16_idx = 0; u16_idx < u16_len; u16_idx++)
    {
        u8_lrc += pu8_data[u16_idx];
    }
    u8_lrc = ~u8_lrc + 1;

    return u8_lrc;
}

#ifdef USE_MODULE_ASSERT

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Check if an instance is a vaild instance of this module
**
** @param [in]
**      x_inst: instance to check
**
** @return
**      Result
**      @arg    true: Valid instance
**      @arg    false: Invalid instance
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static bool b_GT911_Is_Valid_Inst (GT911_inst_t x_inst)
{
    return (x_inst == &g_stru_gt911_obj);
}

#endif

/**
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
