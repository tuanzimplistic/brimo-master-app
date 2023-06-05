/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : hwa_gpio.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 22
**  @brief      : Implementation of Hwa_GPIO module
**  @namespace  : GPIO
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Hwa_GPIO
** @brief       Encapsulates GPIO pins of ESP32 and provides helper APIs to manipulate those pins
**
** @details
** <b> Instruction to use this module: </b>
**
** 1) Declare and configure needed GPIO instances in GPIO_INST_TABLE of hwa_gpio_ext.h file
**
** 2) Get instance of a GPIO using s8_GPIO_Get_Inst(), this instance shall be used in other functions of this module.
**    When a GPIO instance is got the first time, it will be initialized.
**
** 3) Direction and active level of the open GPIO can be changed during run-time using s8_GPIO_Change_Dir()
**    and s8_GPIO_Change_Active_Level(), respectively.
**
** 4) If the GPIO is an output, its level can be controlled using s8_GPIO_Write_Level() or s8_GPIO_Write_Active().
**
**    If the GPIO is an input:
**
**    - Its level can be polled using s8_GPIO_Read_Level() or s8_GPIO_Read_Active()
**
**    - Alternately, s8_GPIO_Enable_Interrupt() can be used to put the associated GPIO pin into interrupt mode. If
**      there is a rising edge and/or falling edge at the pin, the registered callback function shall be invoked in
**      interrupt context. To return to polling mode, s8_GPIO_Disable_Interrupt() can be used.
**      By default, interrupt mode is disabled.
**
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "hwa_gpio.h"           /* Public header of this module */
#include "driver/gpio.h"        /* Use ESP-IDF's GPIO driver */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Structure to manage a GPIO object */
struct GPIO_obj
{
    bool                    b_initialized;          //!< Specifies whether the object has been initialized or not
    GPIO_inst_id_t          enm_inst_id;            //!< Instance ID of this object
    uint8_t                 u8_output_level;        //!< Current level (0, 1) output at the GPIO if it is an output pin

    GPIO_callback_t         pfnc_input_cb;          //!< Callback function invoked when an external interrupt triggers
    void *                  pv_cb_arg;              //!< Argument passed when the callback function was registered

    gpio_num_t              enm_gpio_num;           //!< GPIO number
    GPIO_dir_t              enm_direction;          //!< GPIO direction
    uint8_t                 u8_active_level;        //!< Level (0, 1) at which the GPIO is active
    gpio_pull_mode_t        enm_pull_mode;          //!< Internal resitor pull mode
    bool                    b_is_od;                //!< Specifies if the pin in open drain mode (output pin only)
    gpio_drive_cap_t        enm_drive_strength;     //!< Pad drive capacity
};

/** @brief  Macro expanding GPIO_INST_TABLE as initialization value for GPIO_obj struct */
#define INST_TABLE_EXPAND_AS_STRUCT_INIT(INST_ID, NUM, DIR, ACTIVE, PULL, OD, STRENGTH)     \
{                                                                                           \
    .b_initialized          = false,                                                        \
    .enm_inst_id            = INST_ID,                                                      \
    .u8_output_level        = !ACTIVE,                                                      \
                                                                                            \
    .pfnc_input_cb          = NULL,                                                         \
    .pv_cb_arg              = NULL,                                                         \
                                                                                            \
    .enm_gpio_num           = GPIO_NUM_##NUM,                                               \
    .enm_direction          = GPIO_DIR_##DIR,                                               \
    .u8_active_level        = ACTIVE,                                                       \
    .enm_pull_mode          = GPIO_##PULL,                                                  \
    .b_is_od                = OD,                                                           \
    .enm_drive_strength     = GPIO_DRIVE_CAP_##STRENGTH,                                    \
},

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Hwa_GPIO";

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Array of all GPIO objects */
static struct GPIO_obj g_astru_gpio_objs[GPIO_NUM_INST] =
{
    GPIO_INST_TABLE (INST_TABLE_EXPAND_AS_STRUCT_INIT)
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifdef USE_MODULE_ASSERT
 static bool b_GPIO_Is_Valid_Inst (GPIO_inst_t x_inst);
#endif

static int8_t s8_GPIO_Init_Module (void);
static int8_t s8_GPIO_Init_Inst (GPIO_inst_t x_inst);
static void v_GPIO_Isr_Handler (void * pv_arg);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets instance of a GPIO. This instance will be used for the other functions in this module
**
** @param [in]
**      enm_inst_id: Index of the GPIO to get. The index is expanded from GPIO_INST_TABLE
**
** @param [out]
**      px_inst: Container to store the retrieved instance
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIO_Get_Inst (GPIO_inst_id_t enm_inst_id, GPIO_inst_t * px_inst)
{
    GPIO_inst_t     x_inst = NULL;
    int8_t          s8_result = GPIO_OK;

    /* Validation */
    ASSERT_PARAM ((enm_inst_id < GPIO_NUM_INST) && (px_inst != NULL));

    /* Initialize */
    *px_inst = NULL;

    /* If this module has not been initialized, do that now */
    if (s8_result >= GPIO_OK)
    {
        if (!g_b_initialized)
        {
            s8_result = s8_GPIO_Init_Module ();
            if (s8_result >= GPIO_OK)
            {
                g_b_initialized = true;
            }
        }
    }

    /* If the retrieved instance has not been initialized yet, do that now */
    if (s8_result >= GPIO_OK)
    {
        x_inst = &g_astru_gpio_objs[enm_inst_id];
        if (!x_inst->b_initialized)
        {
            s8_result = s8_GPIO_Init_Inst (x_inst);
            if (s8_result >= GPIO_OK)
            {
                x_inst->b_initialized = true;
            }
        }
    }

    /* Return instance of the GPIO */
    if (s8_result >= GPIO_OK)
    {
        *px_inst = x_inst;
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Changes direction of a GPIO
**
** @param [in]
**      x_inst: A specific GPIO instance
**
** @param [in]
**      enm_dir: GPIO pin direction
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIO_Change_Dir (GPIO_inst_t x_inst, GPIO_dir_t enm_dir)
{
    /* Validation */
    ASSERT_PARAM (b_GPIO_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM (enm_dir < GPIO_NUM_DIRS);

    /* Check if direction is changed */
    if (x_inst->enm_direction != enm_dir)
    {
        /* Change GPIO pin's direction */
        if (enm_dir == GPIO_DIR_INPUT)
        {
            ESP_ERROR_CHECK (gpio_set_direction (x_inst->enm_gpio_num, GPIO_MODE_INPUT));
        }
        else
        {
            ESP_ERROR_CHECK (gpio_set_direction (x_inst->enm_gpio_num,
                                                 x_inst->b_is_od ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT));
        }

        /* Initialize value of an output pin */
        if (enm_dir == GPIO_DIR_OUTPUT)
        {
            ESP_ERROR_CHECK (gpio_set_level (x_inst->enm_gpio_num, x_inst->u8_output_level));
        }

        /* Store new GPIO direction */
        x_inst->enm_direction = enm_dir;
    }

    return GPIO_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Changes active level of a GPIO pin
**
** @param [in]
**      x_inst: A specific GPIO instance
**
** @param [in]
**      u8_active_level: Level at which the component connected to the GPIO pin is active
**      @arg 0 : GPIO level is "0" when active and "1" when not active
**      @arg 1 : GPIO level is "1" when active and "0" when not active
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIO_Change_Active_Level (GPIO_inst_t x_inst, uint8_t u8_active_level)
{
    /* Validation */
    ASSERT_PARAM (b_GPIO_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM (u8_active_level <= 1);

    /* Change GPIO active level */
    x_inst->u8_active_level = u8_active_level;

    return GPIO_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Writes an output pin to either level 0 or 1
**
** @param [in]
**      x_inst: A specific GPIO instance
**
** @param [in]
**      u8_level: Level to drive to the GPIO pin
**      @arg 0: pulls the specified GPIO to logic 0
**      @arg 1: pulls the specified GPIO to logic 1
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIO_Write_Level (GPIO_inst_t x_inst, uint8_t u8_level)
{
    /* Validation */
    ASSERT_PARAM (b_GPIO_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM (u8_level <= 1);

    /* Ensure that the instance is an output pin */
    if (x_inst->enm_direction != GPIO_DIR_OUTPUT)
    {
        return GPIO_ERR;
    }

    /* Set the GPIO pin to the desired value */
    ESP_ERROR_CHECK (gpio_set_level (x_inst->enm_gpio_num, u8_level));

    /* Store pin level */
    x_inst->u8_output_level = u8_level;

    return GPIO_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Writes an output pin to its active level
**
** @param [in]
**      x_inst: A specific GPIO instance
**
** @param [in]
**      b_active: Specify whether the specified GPIO is pulled to its active level or not.
**
** @note
**      Active level of a GPIO pin is either logic 0 or logic 1, depending on what logic is considered as active level
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIO_Write_Active (GPIO_inst_t x_inst, bool b_active)
{
    uint8_t u8_level = 0;

    /* Validation */
    ASSERT_PARAM (b_GPIO_Is_Valid_Inst (x_inst) && x_inst->b_initialized);

    /* Ensure that the instance is an output pin */
    if (x_inst->enm_direction != GPIO_DIR_OUTPUT)
    {
        return GPIO_ERR;
    }

    /* Determine value to write to the GPIO pin */
    if ((b_active && (x_inst->u8_active_level == 1)) ||
        (!b_active && (x_inst->u8_active_level == 0)))
    {
        u8_level = 1;
    }

    /* Set the GPIO pin to the desired value */
    ESP_ERROR_CHECK (gpio_set_level (x_inst->enm_gpio_num, u8_level));

    /* Store pin level */
    x_inst->u8_output_level = u8_level;

    return GPIO_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Inverts value output at an output pin, i.e if current output value is 0 it shall be 1 and vice versa
**
** @param [in]
**      x_inst: A specific GPIO instance
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIO_Write_Inverted (GPIO_inst_t x_inst)
{
    uint8_t u8_level;

    /* Validation */
    ASSERT_PARAM (b_GPIO_Is_Valid_Inst (x_inst) && x_inst->b_initialized);

    /* Ensure that the instance is an output pin */
    if (x_inst->enm_direction != GPIO_DIR_OUTPUT)
    {
        return GPIO_ERR;
    }

    /* Determine value to write to the GPIO pin */
    u8_level = !x_inst->u8_output_level;

    /* Set the GPIO pin to the desired value */
    ESP_ERROR_CHECK (gpio_set_level (x_inst->enm_gpio_num, u8_level));

    /* Store pin level */
    x_inst->u8_output_level = u8_level;

    return GPIO_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets current level of an input or output pin
**
** @param [in]
**      x_inst: A specific GPIO instance
**
** @param [out]
**      pu8_level: Pointer to the buffer to contain read value, which shall be either 0 or 1
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIO_Read_Level (GPIO_inst_t x_inst, uint8_t * pu8_level)
{
    /* Validation */
    ASSERT_PARAM (b_GPIO_Is_Valid_Inst (x_inst) && x_inst->b_initialized && (pu8_level != NULL));

    /* Check pin direction */
    if (x_inst->enm_direction == GPIO_DIR_INPUT)
    {
        /* Read value of the input GPIO pin */
        *pu8_level = gpio_get_level (x_inst->enm_gpio_num);
    }
    else
    {
        /* If this is an output pin, just return the value which is being output */
        *pu8_level = x_inst->u8_output_level;
    }

    return GPIO_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Checks if an input or output pin is at its active level or not.
**
** @param [in]
**      x_inst: A specific GPIO instance
**
** @param [out]
**      pb_active: Container to store the result
**
** @note
**      Active level of a GPIO pin is either logic 0 or logic 1, depending on what logic is considered as active level
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIO_Read_Active (GPIO_inst_t x_inst, bool * pb_active)
{
    uint8_t u8_level = 0;

    /* Validation */
    ASSERT_PARAM (b_GPIO_Is_Valid_Inst (x_inst) && x_inst->b_initialized && (pb_active != NULL));

    /* Check pin direction */
    if (x_inst->enm_direction == GPIO_DIR_INPUT)
    {
        /* Read value of the input GPIO pin */
        u8_level = gpio_get_level (x_inst->enm_gpio_num);
    }
    else
    {
        /* If this is an output pin, use the value which is being output */
        u8_level = x_inst->u8_output_level;
    }

    /* Determine active level */
    *pb_active = (x_inst->u8_active_level == u8_level);

    return GPIO_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Enables external interrupt of an input pin
**
** @note
**      The callback function provided will be invoked in GPIO interrupt context
**
** @param [in]
**      x_inst: A specific GPIO instance
**
** @param [in]
**      enm_mode: Specifies when the interrupt is triggered
**
** @param [in]
**      pfnc_cb: The calback to be invoked when the interrupt is triggered
**
** @param [in]
**      pv_arg: Optional argument which will be forwarded to the data of callback function when it's invoked
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIO_Enable_Interrupt (GPIO_inst_t x_inst, GPIO_int_mode_t enm_mode, GPIO_callback_t pfnc_cb, void * pv_arg)
{
    /* Validation */
    ASSERT_PARAM (b_GPIO_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM ((enm_mode < GPIO_NUM_INT_MODES) && (pfnc_cb != NULL));

    /* Ensure that the instance is an input pin */
    if (x_inst->enm_direction != GPIO_DIR_INPUT)
    {
        return GPIO_ERR;
    }

    /* Store the callback */
    x_inst->pfnc_input_cb = pfnc_cb;
    x_inst->pv_cb_arg = pv_arg;

    /* Add ISR handler for the corresponding GPIO pin */
    ESP_ERROR_CHECK (gpio_isr_handler_add (x_inst->enm_gpio_num, v_GPIO_Isr_Handler, x_inst));

    /* Configure interrupt type. Interrupt of the corresponding GPIO pin will be enabled after this */
    ESP_ERROR_CHECK (gpio_set_intr_type (x_inst->enm_gpio_num,
                                         (enm_mode == GPIO_INT_RISING_EDGE) ? GPIO_INTR_POSEDGE :
                                         (enm_mode == GPIO_INT_FALLING_EDGE) ? GPIO_INTR_NEGEDGE :
                                         GPIO_INTR_ANYEDGE));

    return GPIO_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Disables external interrupt of an input pin
**
** @param [in]
**      x_inst: A specific GPIO instance
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIO_Disable_Interrupt (GPIO_inst_t x_inst)
{
    /* Validation */
    ASSERT_PARAM (b_GPIO_Is_Valid_Inst (x_inst) && x_inst->b_initialized);

    /* Ensure that the instance is an input pin */
    if (x_inst->enm_direction != GPIO_DIR_INPUT)
    {
        return GPIO_ERR;
    }

    /* Disable external interrupt */
    ESP_ERROR_CHECK (gpio_intr_disable (x_inst->enm_gpio_num));

    /* Remove ISR handler for the corresponding GPIO pin */
    ESP_ERROR_CHECK (gpio_isr_handler_remove (x_inst->enm_gpio_num));

    /* Clear the callback */
    x_inst->pfnc_input_cb = NULL;

    return GPIO_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Hwa_GPIO module
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GPIO_Init_Module (void)
{
    /* Install GPIO ISR service */
    ESP_ERROR_CHECK (gpio_install_isr_service (0));

    return GPIO_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes a GPIO instance
**
** @param [in]
**      x_inst: Specific instance
**
** @return
**      @arg    GPIO_OK
**      @arg    GPIO_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GPIO_Init_Inst (GPIO_inst_t x_inst)
{
    /* Reset the gpio to default state (select gpio function, enable pullup and disable input and output) */
    ESP_ERROR_CHECK (gpio_reset_pin (x_inst->enm_gpio_num));

    /* Configure GPIO direction */
    if (x_inst->enm_direction == GPIO_DIR_INPUT)
    {
        ESP_ERROR_CHECK (gpio_set_direction (x_inst->enm_gpio_num, GPIO_MODE_INPUT));
    }
    else
    {
        ESP_ERROR_CHECK (gpio_set_direction (x_inst->enm_gpio_num,
                                             x_inst->b_is_od ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT));
    }

    /* Configure pull resistor */
    ESP_ERROR_CHECK (gpio_set_pull_mode (x_inst->enm_gpio_num, x_inst->enm_pull_mode));

    /* Configure drive strength */
    if (x_inst->enm_direction == GPIO_DIR_OUTPUT)
    {
        ESP_ERROR_CHECK (gpio_set_drive_capability (x_inst->enm_gpio_num, x_inst->enm_drive_strength));
    }

    /* Initialize output level if it's an output pin */
    if (x_inst->enm_direction == GPIO_DIR_OUTPUT)
    {
        ESP_ERROR_CHECK (gpio_set_level (x_inst->enm_gpio_num, x_inst->u8_output_level));
    }

    return GPIO_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of GPIO external interrupt
**
** @param [in]
**      pv_arg: Argument passed to the handler
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GPIO_Isr_Handler (void * pv_arg)
{
    /* The passed argument is the GPIO instance of the pin that the interrupt triggered */
    GPIO_inst_t x_inst = (GPIO_inst_t) pv_arg;

    /* Invoke callback function of the corresponding GPIO pin */
    if (x_inst->pfnc_input_cb != NULL)
    {
        GPIO_evt_data_t stru_evt_data =
        {
            .x_inst             = x_inst,
            .pv_arg             = x_inst->pv_cb_arg,
            .enm_evt            = GPIO_EVT_EDGE_DETECTED,
        };
        x_inst->pfnc_input_cb (&stru_evt_data);
    }
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
static bool b_GPIO_Is_Valid_Inst (GPIO_inst_t x_inst)
{
    uint32_t u32_idx = 0;

    /* Searching instance */
    for (u32_idx = 0; u32_idx < (uint32_t)GPIO_NUM_INST; u32_idx++)
    {
        if (x_inst == &g_astru_gpio_objs[u32_idx])
        {
            /* Stop searching if there is one matching instance  */
            return true;
        }
    }

    return false;
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
