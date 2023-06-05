/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_io_tca9534.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 25
**  @brief      : Implementation of Srvc_IO_TCA9534 module
**  @namespace  : GPIOX
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup      Srvc_IO_TCA9534
** @brief           Encapsulates expanded GPIO pins of TCA9534 component on EB1.1 master board
**
** @details
** <b> Instruction to use this module: </b>
**
** 1) Declare and configure needed GPIOX instances in GPIOX_INST_TABLE of srvc_io_tca9534_ext.h file
**
** 2) Get instance of a GPIOX using s8_GPIOX_Get_Inst(), this instance shall be used in other functions of this module.
**    When a GPIOX instance is got the first time, it will be initialized.
**
** 3) Direction and active level of the openning GPIOX can be changed during run-time with s8_GPIOX_Change_Dir()
**    and s8_GPIOX_Change_Active_Level(), respectively.
**
** 4) If the GPIOX is an output, its level can be controlled using s8_GPIOX_Write_Level() or s8_GPIOX_Write_Active().
**
**    If the GPIOX is an input:
**
**    - Its level can be polled using s8_GPIOX_Read_Level() or s8_GPIOX_Read_Active()
**
**    - Alternately, s8_GPIOX_Enable_Interrupt() can be used to detect if value of any GPIOX input pins changes.
**      To return to polling mode, s8_GPIOX_Disable_Interrupt() can be used. By default, interrupt is disabled.
**
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_io_tca9534.h"        /* Public header of this module */
#include "hwa_gpio.h"               /* Use GPIO abstraction module */
#include "hwa_i2c_master.h"         /* Use I2C Master abstraction module */

#include "freertos/FreeRTOS.h"      /* Use FreeRTOS */
#include "freertos/semphr.h"        /* Use FreeRTOS semaphore */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Structure to manage a GPIOX object */
struct GPIOX_obj
{
    bool                    b_initialized;          //!< Specifies whether the object has been initialized or not
    GPIOX_inst_id_t         enm_inst_id;            //!< Instance ID of this object
    uint8_t                 u8_output_level;        //!< Current level (0, 1) output at the GPIOX if it is an output pin
    GPIOX_cb_t              pfnc_input_cb;          //!< Callback function invoked when the input interrupt triggers

    uint8_t                 u8_port_num;            //!< Port number (0 -> 7) of the GPIOX pin on TCA9534
    GPIOX_dir_t             enm_direction;          //!< Pin direction
    uint8_t                 u8_active_level;        //!< Level (0, 1) at which the GPIOX is active
};

/** @brief  Macro expanding GPIOX_INST_TABLE as initialization value for GPIOX_obj struct */
#define INST_TABLE_EXPAND_AS_STRUCT_INIT(INST_ID, PORT, DIR, ACTIVE)    \
{                                                                       \
    .b_initialized          = false,                                    \
    .enm_inst_id            = INST_ID,                                  \
    .u8_output_level        = !ACTIVE,                                  \
    .pfnc_input_cb          = NULL,                                     \
                                                                        \
    .u8_port_num            = PORT,                                     \
    .enm_direction          = GPIOX_DIR_##DIR,                          \
    .u8_active_level        = ACTIVE,                                   \
},

/** @brief  Address of TCA9534's registers */
enum
{
    GPIOX_REG_INPUT         = 0x00,                 //!< Input register
    GPIOX_REG_OUTPUT        = 0x01,                 //!< Output register
    GPIOX_REG_POLARITY      = 0x02,                 //!< Polarity inversion register
    GPIOX_REG_CONFIG        = 0x03,                 //!< Configuration register
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_IO_TCA9534";

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Array of all GPIOX objects */
static struct GPIOX_obj g_astru_gpiox_objs[GPIOX_NUM_INST] =
{
    GPIOX_INST_TABLE (INST_TABLE_EXPAND_AS_STRUCT_INIT)
};

/** @brief  Mutex ensuring that there is only one access to TCA9534 at a time */
static SemaphoreHandle_t g_x_tca9534_sem = NULL;

/** @brief  Instance of the I2C Master to access TCA9534 */
static I2C_inst_t g_x_i2c_inst = NULL;

/** @brief  Instance of the GPIO pin connected to INT signal of TCA9534 */
static GPIO_inst_t g_x_gpio_int_inst = NULL;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifdef USE_MODULE_ASSERT
 static bool b_GPIOX_Is_Valid_Inst (GPIOX_inst_t x_inst);
#endif

static int8_t s8_GPIOX_Init_Module (void);
static int8_t s8_GPIOX_Init_Inst (GPIOX_inst_t x_inst);
static void v_GPIOX_Isr_Handler (GPIO_evt_data_t * pstru_evt_data);
static int8_t s8_GPIOX_Set_TCA9534_Direction (I2C_inst_t x_i2c_inst, uint8_t u8_port, GPIOX_dir_t enm_dir);
static int8_t s8_GPIOX_Set_TCA9534_Output (I2C_inst_t x_i2c_inst, uint8_t u8_port, uint8_t u8_level);
static int8_t s8_GPIOX_Get_TCA9534_Input (I2C_inst_t x_i2c_inst, uint8_t u8_port, uint8_t * pu8_level);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets instance of a GPIOX. This instance will be used for the other functions in this module
**
** @param [in]
**      enm_inst_id: Index of the GPIOX to get. The index is expanded from GPIOX_INST_TABLE
**
** @param [out]
**      px_inst: Container to store the retrieved instance
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIOX_Get_Inst (GPIOX_inst_id_t enm_inst_id, GPIOX_inst_t * px_inst)
{
    GPIOX_inst_t    x_inst = NULL;
    int8_t          s8_result = GPIOX_OK;

    /* Validation */
    ASSERT_PARAM ((enm_inst_id < GPIOX_NUM_INST) && (px_inst != NULL));

    /* Initialize */
    *px_inst = NULL;

    /* If this module has not been initialized, do that now */
    if (s8_result >= GPIOX_OK)
    {
        if (!g_b_initialized)
        {
            s8_result = s8_GPIOX_Init_Module ();
            if (s8_result >= GPIOX_OK)
            {
                g_b_initialized = true;
            }
        }
    }

    /* If the retrieved instance has not been initialized yet, do that now */
    if (s8_result >= GPIOX_OK)
    {
        x_inst = &g_astru_gpiox_objs[enm_inst_id];
        if (!x_inst->b_initialized)
        {
            s8_result = s8_GPIOX_Init_Inst (x_inst);
            if (s8_result >= GPIOX_OK)
            {
                x_inst->b_initialized = true;
            }
        }
    }

    /* Return instance of the GPIOX */
    if (s8_result >= GPIOX_OK)
    {
        *px_inst = x_inst;
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Changes direction of a GPIOX
**
** @param [in]
**      x_inst: A specific GPIOX instance
**
** @param [in]
**      enm_dir: GPIOX pin direction
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIOX_Change_Dir (GPIOX_inst_t x_inst, GPIOX_dir_t enm_dir)
{
    /* Validation */
    ASSERT_PARAM (b_GPIOX_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM (enm_dir < GPIOX_NUM_DIRS);

    /* Check if direction is changed */
    if (x_inst->enm_direction != enm_dir)
    {
        /* Change GPIO pin's direction */
        if (s8_GPIOX_Set_TCA9534_Direction (g_x_i2c_inst, x_inst->u8_port_num, enm_dir) != GPIOX_OK)
        {
            return GPIOX_ERR;
        }

        /* Initialize value of an output pin */
        if (enm_dir == GPIOX_DIR_OUTPUT)
        {
            s8_GPIOX_Set_TCA9534_Output (g_x_i2c_inst, x_inst->u8_port_num, x_inst->u8_output_level);
        }

        /* Store new GPIO direction */
        x_inst->enm_direction = enm_dir;
    }

    return GPIOX_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Changes active level of a GPIOX pin
**
** @param [in]
**      x_inst: A specific GPIOX instance
**
** @param [in]
**      u8_active_level: Level at which the component connected to the GPIOX pin is active
**      @arg 0 : GPIOX level is "0" when active and "1" when not active
**      @arg 1 : GPIOX level is "1" when active and "0" when not active
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIOX_Change_Active_Level (GPIOX_inst_t x_inst, uint8_t u8_active_level)
{
    /* Validation */
    ASSERT_PARAM (b_GPIOX_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM (u8_active_level <= 1);

    /* Change GPIOX active level */
    x_inst->u8_active_level = u8_active_level;

    return GPIOX_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Writes a GPIOX output pin to either level 0 or 1
**
** @param [in]
**      x_inst: A specific GPIOX instance
**
** @param [in]
**      u8_level: Level to drive to the GPIOX pin
**      @arg 0: pulls the specified GPIOX to logic 0
**      @arg 1: pulls the specified GPIOX to logic 1
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIOX_Write_Level (GPIOX_inst_t x_inst, uint8_t u8_level)
{
    /* Validation */
    ASSERT_PARAM (b_GPIOX_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM (u8_level <= 1);

    /* Ensure that the instance is an output pin */
    if (x_inst->enm_direction != GPIOX_DIR_OUTPUT)
    {
        return GPIOX_ERR;
    }

    /* Set the GPIO pin to the desired value */
    if (s8_GPIOX_Set_TCA9534_Output (g_x_i2c_inst, x_inst->u8_port_num, u8_level) != GPIOX_OK)
    {
        return GPIOX_ERR;
    }

    /* Store pin level */
    x_inst->u8_output_level = u8_level;

    return GPIOX_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Writes a GPIOX output pin to its active level
**
** @param [in]
**      x_inst: A specific GPIOX instance
**
** @param [in]
**      b_active: Specify whether the specified GPIOX is pulled to its active level or not.
**
** @note
**      Active level of a GPIOX pin is either logic 0 or logic 1, depending on what logic is considered as active level
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIOX_Write_Active (GPIOX_inst_t x_inst, bool b_active)
{
    uint8_t u8_level = 0;

    /* Validation */
    ASSERT_PARAM (b_GPIOX_Is_Valid_Inst (x_inst) && x_inst->b_initialized);

    /* Ensure that the instance is an output pin */
    if (x_inst->enm_direction != GPIOX_DIR_OUTPUT)
    {
        return GPIOX_ERR;
    }

    /* Determine value to write to the GPIO pin */
    if ((b_active && (x_inst->u8_active_level == 1)) ||
        (!b_active && (x_inst->u8_active_level == 0)))
    {
        u8_level = 1;
    }

    /* Set the GPIO pin to the desired value */
    if (s8_GPIOX_Set_TCA9534_Output (g_x_i2c_inst, x_inst->u8_port_num, u8_level) != GPIOX_OK)
    {
        return GPIOX_ERR;
    }

    /* Store pin level */
    x_inst->u8_output_level = u8_level;

    return GPIOX_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Inverts level of a GPIOX output pin, i.e if current output level is 0 it shall be 1 and vice versa
**
** @param [in]
**      x_inst: A specific GPIOX instance
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIOX_Write_Inverted (GPIOX_inst_t x_inst)
{
    uint8_t u8_level;

    /* Validation */
    ASSERT_PARAM (b_GPIOX_Is_Valid_Inst (x_inst) && x_inst->b_initialized);

    /* Ensure that the instance is an output pin */
    if (x_inst->enm_direction != GPIOX_DIR_OUTPUT)
    {
        return GPIOX_ERR;
    }

    /* Determine value to write to the GPIOX pin */
    u8_level = !x_inst->u8_output_level;

    /* Set the GPIOX pin to the desired value */
    if (s8_GPIOX_Set_TCA9534_Output (g_x_i2c_inst, x_inst->u8_port_num, u8_level) != GPIOX_OK)
    {
        return GPIOX_ERR;
    }

    /* Store pin level */
    x_inst->u8_output_level = u8_level;

    return GPIOX_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets current level of a GPIOX input or output pin
**
** @param [in]
**      x_inst: A specific GPIOX instance
**
** @param [out]
**      pu8_level: Pointer to the buffer to contain read value, which shall be either 0 or 1
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIOX_Read_Level (GPIOX_inst_t x_inst, uint8_t * pu8_level)
{
    /* Validation */
    ASSERT_PARAM (b_GPIOX_Is_Valid_Inst (x_inst) && x_inst->b_initialized && (pu8_level != NULL));

    /* Check pin direction */
    if (x_inst->enm_direction == GPIOX_DIR_INPUT)
    {
        /* Read value of the input GPIOX pin */
        if (s8_GPIOX_Get_TCA9534_Input (g_x_i2c_inst, x_inst->u8_port_num, pu8_level) != GPIOX_OK)
        {
            return GPIOX_ERR;
        }
    }
    else
    {
        /* If this is an output pin, just return the value which is being output */
        *pu8_level = x_inst->u8_output_level;
    }

    return GPIOX_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Checks if a GPIOX input or output pin is at its active level or not.
**
** @param [in]
**      x_inst: A specific GPIOX instance
**
** @param [out]
**      pb_active: Container to store the result
**
** @note
**      Active level of a GPIOX pin is either logic 0 or logic 1, depending on what logic is considered as active level
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIOX_Read_Active (GPIOX_inst_t x_inst, bool * pb_active)
{
    uint8_t u8_level = 0;

    /* Validation */
    ASSERT_PARAM (b_GPIOX_Is_Valid_Inst (x_inst) && x_inst->b_initialized && (pb_active != NULL));

    /* Check pin direction */
    if (x_inst->enm_direction == GPIOX_DIR_INPUT)
    {
        /* Read value of the input GPIOX pin */
        if (s8_GPIOX_Get_TCA9534_Input (g_x_i2c_inst, x_inst->u8_port_num, &u8_level) != GPIOX_OK)
        {
            return GPIOX_ERR;
        }
    }
    else
    {
        /* If this is an output pin, use the value which is being output */
        u8_level = x_inst->u8_output_level;
    }

    /* Determine active level */
    *pb_active = (x_inst->u8_active_level == u8_level);

    return GPIOX_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Enables interrupt firing when value of any GPIOX input pins changes
**
** @note
**      The callback function provided will be invoked in GPIO interrupt context
**
** @param [in]
**      x_inst: A specific GPIOX instance
**
** @param [in]
**      enm_mode: Specifies when the interrupt is triggered
**
** @param [in]
**      pfnc_cb: The calback to be invoked when the interrupt is triggered
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIOX_Enable_Interrupt (GPIOX_inst_t x_inst, GPIOX_cb_t pfnc_cb)
{
    int8_t s8_result = GPIOX_OK;

    /* Validation */
    ASSERT_PARAM (b_GPIOX_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM (pfnc_cb != NULL);

    /* Ensure that the instance is an input pin */
    if (x_inst->enm_direction != GPIOX_DIR_INPUT)
    {
        return GPIOX_ERR;
    }

    /* Serialize access to callback function */
    xSemaphoreTake (g_x_tca9534_sem, portMAX_DELAY);

    /* Check if GPIO interrupt of INT signal is already enabled */
    bool b_int_enabled = false;
    for (uint8_t u8_idx = 0; u8_idx < GPIOX_NUM_INST; u8_idx++)
    {
        if (g_astru_gpiox_objs[u8_idx].pfnc_input_cb != NULL)
        {
            b_int_enabled = true;
            break;
        }
    }
    if (!b_int_enabled)
    {
        s8_result = s8_GPIO_Enable_Interrupt (g_x_gpio_int_inst, GPIO_INT_FALLING_EDGE, v_GPIOX_Isr_Handler, NULL);
    }

    /* Store the callback */
    if (s8_result == GPIOX_OK)
    {
        x_inst->pfnc_input_cb = pfnc_cb;
    }

    /* Done */
    xSemaphoreGive (g_x_tca9534_sem);
    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Disables interrupt firing when value of any GPIOX input pins changes
**
** @param [in]
**      x_inst: A specific GPIOX instance
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_GPIOX_Disable_Interrupt (GPIOX_inst_t x_inst)
{
    int8_t s8_result = GPIOX_OK;

    /* Validation */
    ASSERT_PARAM (b_GPIOX_Is_Valid_Inst (x_inst) && x_inst->b_initialized);

    /* Ensure that the instance is an input pin */
    if (x_inst->enm_direction != GPIOX_DIR_INPUT)
    {
        return GPIOX_ERR;
    }

    /* Serialize access to callback function */
    xSemaphoreTake (g_x_tca9534_sem, portMAX_DELAY);

    /* Clear the callback */
    x_inst->pfnc_input_cb = NULL;

    /* Check if GPIO interrupt of INT signal needs to be disabled */
    bool b_int_enabled = false;
    for (uint8_t u8_idx = 0; u8_idx < GPIOX_NUM_INST; u8_idx++)
    {
        if (g_astru_gpiox_objs[u8_idx].pfnc_input_cb != NULL)
        {
            b_int_enabled = true;
            break;
        }
    }
    if (!b_int_enabled)
    {
        s8_result = s8_GPIO_Disable_Interrupt (g_x_gpio_int_inst);
    }

    /* Done */
    xSemaphoreGive (g_x_tca9534_sem);
    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_IO_TCA9534 module
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GPIOX_Init_Module (void)
{
    /* Create mutexs preventing multiple accesses to one TCA9534 at a time */
    g_x_tca9534_sem = xSemaphoreCreateMutex ();
    if (g_x_tca9534_sem == NULL)
    {
        return GPIOX_ERR;
    }

    /* Get instance of I2C Master */
    if (s8_I2C_Get_Inst (I2C_TCA9534, &g_x_i2c_inst) != I2C_OK)
    {
        return GPIOX_ERR;
    }

    /* Get instance of GPIO connected to INT signal */
    if (s8_GPIO_Get_Inst (GPIO_TCA9534_INT, &g_x_gpio_int_inst) != GPIO_OK)
    {
        return GPIOX_ERR;
    }

    /* Initialize all pins of TCA9534 to desired direction */
    for (uint8_t u8_idx = 0; u8_idx < GPIOX_NUM_INST; u8_idx++)
    {
        s8_GPIOX_Init_Inst (&g_astru_gpiox_objs[u8_idx]);
    }

    /* Read all inputs at least once to refresh input register. This is required for input interrupt to work */
    uint8_t u8_dummy;
    s8_GPIOX_Get_TCA9534_Input (g_x_i2c_inst, 0, &u8_dummy);

    return GPIOX_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes a GPIOX instance
**
** @param [in]
**      x_inst: A specific GPIOX instance
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GPIOX_Init_Inst (GPIOX_inst_t x_inst)
{
    /* Configure desired direction */
    if (s8_GPIOX_Set_TCA9534_Direction (g_x_i2c_inst, x_inst->u8_port_num, x_inst->enm_direction) != GPIOX_OK)
    {
        return GPIOX_ERR;
    }

    /* Initialize output level if it's an output pin */
    if (x_inst->enm_direction == GPIOX_DIR_OUTPUT)
    {
        if (s8_GPIOX_Set_TCA9534_Output (g_x_i2c_inst, x_inst->u8_port_num, x_inst->u8_output_level) != GPIOX_OK)
        {
            return GPIOX_ERR;
        }
    }

    return GPIOX_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Handler of GPIO external interrupt
**
** @param [in]
**      pstru_evt_data: Context data of the event
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_GPIOX_Isr_Handler (GPIO_evt_data_t * pstru_evt_data)
{
    /* Invoke callback functions of all input GPIOX pins */
    for (uint8_t u8_idx = 0; u8_idx < GPIOX_NUM_INST; u8_idx++)
    {
        GPIOX_inst_t x_inst = &g_astru_gpiox_objs[u8_idx];
        if ((x_inst->enm_direction == GPIOX_DIR_INPUT) && (x_inst->pfnc_input_cb != NULL))
        {
            x_inst->pfnc_input_cb (x_inst);
        }
    }
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Changes direction of a TCA9534 port
**
** @param [in]
**      x_i2c_inst: Instance of the I2C master that communicates with TCA9534
**
** @param [in]
**      u8_port: The port to change direction
**
** @param [in]
**      enm_dir: New direction of the port
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GPIOX_Set_TCA9534_Direction (I2C_inst_t x_i2c_inst, uint8_t u8_port, GPIOX_dir_t enm_dir)
{
    static uint8_t  u8_val = 0xFF;
    uint8_t         u8_reg = GPIOX_REG_CONFIG;
    int8_t          s8_result = GPIOX_OK;

    /* Serialize access to TCA9534 */
    xSemaphoreTake (g_x_tca9534_sem, portMAX_DELAY);

    /* Determine new value of configuration register */
    if (enm_dir == GPIOX_DIR_INPUT)
    {
        SET_BITS (u8_val, 1 << u8_port);
    }
    else
    {
        CLR_BITS (u8_val, 1 << u8_port);
    }

    /* Write configuration register with the new value */
    if (s8_I2C_Write_Mem (x_i2c_inst, &u8_reg, sizeof (u8_reg), &u8_val, sizeof (u8_val)) != I2C_OK)
    {
        s8_result = GPIOX_ERR;
    }

    /* Done */
    xSemaphoreGive (g_x_tca9534_sem);
    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sets output of a TCA9534 port to a given level
**
** @param [in]
**      x_i2c_inst: Instance of the I2C master that communicates with TCA9534
**
** @param [in]
**      u8_port: The port to set ouput
**
** @param [in]
**      u8_level: New level of the port
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GPIOX_Set_TCA9534_Output (I2C_inst_t x_i2c_inst, uint8_t u8_port, uint8_t u8_level)
{
    static uint8_t  u8_val = 0xFF;
    uint8_t         u8_reg = GPIOX_REG_OUTPUT;
    int8_t          s8_result = GPIOX_OK;

    /* Serialize access to TCA9534 */
    xSemaphoreTake (g_x_tca9534_sem, portMAX_DELAY);

    /* Determine new value of output register */
    if (u8_level == 1)
    {
        SET_BITS (u8_val, 1 << u8_port);
    }
    else
    {
        CLR_BITS (u8_val, 1 << u8_port);
    }

    /* Write output register with the new value */
    if (s8_I2C_Write_Mem (x_i2c_inst, &u8_reg, sizeof (u8_reg), &u8_val, sizeof (u8_val)) != I2C_OK)
    {
        s8_result = GPIOX_ERR;
    }

    /* Done */
    xSemaphoreGive (g_x_tca9534_sem);
    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets current level of a TCA9534 input port
**
** @param [in]
**      x_i2c_inst: Instance of the I2C master that communicates with TCA9534
**
** @param [in]
**      u8_port: The port to get input
**
** @param [out]
**      pu8_level: Pointer to the buffer to contain read value, which shall be either 0 or 1
**
** @return
**      @arg    GPIOX_OK
**      @arg    GPIOX_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_GPIOX_Get_TCA9534_Input (I2C_inst_t x_i2c_inst, uint8_t u8_port, uint8_t * pu8_level)
{
    uint8_t u8_val = 0x00;
    uint8_t u8_reg = GPIOX_REG_INPUT;
    int8_t  s8_result = GPIOX_OK;

    /* Serialize access to TCA9534 */
    xSemaphoreTake (g_x_tca9534_sem, portMAX_DELAY);

    /* Read input register */
    if (s8_I2C_Read_Mem (x_i2c_inst, &u8_reg, sizeof (u8_reg), &u8_val, sizeof (u8_val)) != I2C_OK)
    {
        s8_result = GPIOX_ERR;
    }

    /* Determine level */
    if (s8_result == GPIOX_OK)
    {
        *pu8_level = ALL_BITS_SET (u8_val, 1 << u8_port);
    }

    /* Done */
    xSemaphoreGive (g_x_tca9534_sem);
    return s8_result;
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
static bool b_GPIOX_Is_Valid_Inst (GPIOX_inst_t x_inst)
{
    uint32_t u32_idx = 0;

    /* Searching instance */
    for (u32_idx = 0; u32_idx < (uint32_t)GPIOX_NUM_INST; u32_idx++)
    {
        if (x_inst == &g_astru_gpiox_objs[u32_idx])
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
