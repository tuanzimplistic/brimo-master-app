/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : hwa_i2c_master.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 23
**  @brief      : Implementation of Hwa_I2C_Master module
**  @namespace  : I2C
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup      Hwa_I2C_Master
** @brief           Encapsulates the communication between a I2C Master and a I2C Slave from the perspective of the
**                  I2C Master
**
** @details
** <b> Instruction to use this module: </b>
**
** 1) Declare and configure properties of I2C Master and the corresponding I2C Slave in I2C_INST_TABLE of file
**    hwa_i2c_master_ext.h
**
** 2) Configure settings of the corresponding I2C port in I2C_PORT_TABLE of file hwa_i2c_master_ext.h
**
** 3) Get instance of an I2C Master with s8_I2C_Get_Inst(), this instance shall be used in other functions of this
**    module.
**
** 4) Write or read data from the corresponding slave with s8_I2C_Write() or s8_I2C_Read(), respectively.
**    If the corresponding slave requires access in memory mode (extra memory address is required beside slave address),
**    s8_I2C_Write_Mem() or s8_I2C_Read_Mem() can be used.
**
** 5) Slave address can be changed on-the-fly with s8_I2C_Set_Slave_Addr().
**
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "hwa_i2c_master.h"             /* Public header of this module */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/semphr.h"            /* Use FreeRTOS semaphore */
#include "driver/i2c.h"                 /* Use ESP-IDF's I2C driver */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
#define TAG                 "Hwa_I2C_Master"

/** @brief  Structure to manage an I2C Master object */
struct I2C_obj
{
    bool                    b_initialized;          //!< Specifies whether the object has been initialized or not
    I2C_inst_id_t           enm_inst_id;            //!< Instance ID of this object

    i2c_port_t              x_i2c_port;             //!< Port number of the corresponding I2C controller
    uint16_t                u16_slave_addr;         //!< Address of I2C slave (7-bit mode)
};

/** @brief  Macro expanding I2C_INST_TABLE as initialization value for I2C_obj struct */
#define INST_TABLE_EXPAND_AS_STRUCT_INIT(INST_ID, PORT_NUM, SLAVE_ADDR) \
{                                                                       \
    .b_initialized          = false,                                    \
    .enm_inst_id            = INST_ID,                                  \
                                                                        \
    .x_i2c_port             = I2C_NUM_##PORT_NUM,                       \
    .u16_slave_addr         = SLAVE_ADDR,                               \
},

/** @brief  Structure to manage an I2C port and its configuration */
struct I2C_port
{
    i2c_port_t              x_i2c_port;             //!< Port number of the corresponding I2C controller
    i2c_config_t            stru_i2c_cfg;           //!< I2C configuration structure
};

/** @brief  Macro expanding I2C_PORT_TABLE as initialization value for I2C_port struct */
#define PORT_TABLE_EXPAND_AS_STRUCT_INIT(PORT_NUM, SDA_PIN, SDA_PU,     \
                                         SCL_PIN, SCL_PU, KHZ)          \
{                                                                       \
    .x_i2c_port             = I2C_NUM_##PORT_NUM,                       \
    .stru_i2c_cfg           =                                           \
    {                                                                   \
        .mode               = I2C_MODE_MASTER,                          \
        .sda_io_num         = GPIO_NUM_##SDA_PIN,                       \
        .sda_pullup_en      = GPIO_PULLUP_##SDA_PU,                     \
        .scl_io_num         = GPIO_NUM_##SCL_PIN,                       \
        .scl_pullup_en      = GPIO_PULLUP_##SCL_PU,                     \
        .master.clk_speed   = KHZ * 1000,                               \
    },                                                                  \
},

/** @brief  Timeout (in milliseconds) waiting for I2C bus to be available */
#define I2C_BUS_WAIT_TIMEOUT        50          // portMAX_DELAY

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Array of all I2C Master objects */
static struct I2C_obj g_astru_i2c_objs[I2C_NUM_INST] =
{
    I2C_INST_TABLE (INST_TABLE_EXPAND_AS_STRUCT_INIT)
};

/** @brief  Array of all I2C ports */
static struct I2C_port g_astru_i2c_ports[] =
{
    I2C_PORT_TABLE (PORT_TABLE_EXPAND_AS_STRUCT_INIT)
};

/** @brief  Semaphore ensuring that there is only one access to one I2C bus at a time */
static SemaphoreHandle_t g_ax_port_sem[I2C_NUM_MAX] = { NULL };

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifdef USE_MODULE_ASSERT
 static bool b_I2C_Is_Valid_Inst (I2C_inst_t x_inst);
#endif

static int8_t s8_I2C_Init_Module (void);
static int8_t s8_I2C_Init_Inst (I2C_inst_t x_inst);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets instance of an I2C Master. This instance will be used for the other functions in this module
**
** @param [in]
**      enm_inst_id: Index of the I2C Master to get. The index is expanded from I2C_INST_TABLE
**
** @param [out]
**      px_inst: Container to store the retrieved instance
**
** @return
**      @arg    I2C_OK
**      @arg    I2C_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_I2C_Get_Inst (I2C_inst_id_t enm_inst_id, I2C_inst_t * px_inst)
{
    I2C_inst_t  x_inst = NULL;
    int8_t      s8_result = I2C_OK;

    /* Validation */
    ASSERT_PARAM ((enm_inst_id < I2C_NUM_INST) && (px_inst != NULL));

    /* Initialize */
    *px_inst = NULL;

    /* If this module has not been initialized, do that now */
    if (s8_result >= I2C_OK)
    {
        if (!g_b_initialized)
        {
            s8_result = s8_I2C_Init_Module ();
            if (s8_result >= I2C_OK)
            {
                g_b_initialized = true;
            }
        }
    }

    /* If the retrieved instance has not been initialized yet, do that now */
    if (s8_result >= I2C_OK)
    {
        x_inst = &g_astru_i2c_objs[enm_inst_id];
        if (!x_inst->b_initialized)
        {
            s8_result = s8_I2C_Init_Inst (x_inst);
            if (s8_result >= I2C_OK)
            {
                x_inst->b_initialized = true;
            }
        }
    }

    /* Return instance of the I2C Master */
    if (s8_result >= I2C_OK)
    {
        *px_inst = x_inst;
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Writes a block of data to the corresponding slave
**
** @note
**      To check if the slave respond to a write command, pu8_data can be NULL (or u16_len can be zero)
**
** @details
**      Data on I2C bus:
**               +-------+-----------+-----+----------+-----+------+
**      + Master | Start | Addr + Wr |     | pu8_data |     | Stop |
**               +-------+-----------+-----+----------+-----+------+
**      + Slave  |       |           | ACK |   (ACK)  | ACK |      |
**               +-------+-----------+-----+----------+-----+------+
**
** @param [in]
**      x_inst: A specific I2C Master instance
**
** @param [in]
**      pu8_data: Pointer to the buffer containing the data to send
**
** @param [in]
**      u16_len: Length in bytes of the data to send
**
** @return
**      @arg    I2C_OK
**      @arg    I2C_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_I2C_Write (I2C_inst_t x_inst, const uint8_t * pu8_data, uint16_t u16_len)
{
    /* Validation */
    ASSERT_PARAM (b_I2C_Is_Valid_Inst (x_inst) && x_inst->b_initialized);

    /* Prevent multiple access to the same I2C bus at the same time */
    if (xSemaphoreTake (g_ax_port_sem[x_inst->x_i2c_port], pdMS_TO_TICKS (I2C_BUS_WAIT_TIMEOUT)) == pdTRUE)
    {
        i2c_cmd_handle_t x_cmd = i2c_cmd_link_create ();

        /* Start signal */
        ESP_ERROR_CHECK (i2c_master_start (x_cmd));

        /* Slave address and write bit */
        ESP_ERROR_CHECK (i2c_master_write_byte (x_cmd, (uint8_t)((x_inst->u16_slave_addr << 1) | I2C_MASTER_WRITE), true));

        /* Write data */
        if ((pu8_data != NULL) && (u16_len > 0))
        {
            ESP_ERROR_CHECK (i2c_master_write (x_cmd, pu8_data, u16_len, true));
        }

        /* Stop signal */
        ESP_ERROR_CHECK (i2c_master_stop (x_cmd));

        /* Execute command */
        esp_err_t x_err = i2c_master_cmd_begin (x_inst->x_i2c_port, x_cmd, pdMS_TO_TICKS (I2C_BUS_WAIT_TIMEOUT));

        /* Clean up */
        i2c_cmd_link_delete (x_cmd);

        /* We have done using the I2C bus, release the relevant semaphore */
        xSemaphoreGive (g_ax_port_sem[x_inst->x_i2c_port]);

        /* Check result */
        if (x_err != ESP_OK)
        {
            return I2C_ERR;
        }
        return I2C_OK;
    }

    /* It's take too long waiting for the I2C bus to be available */
    return I2C_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Writes a block of data to the corresponding slave in memory access mode
**
** @details
**      Data on I2C bus:
**               +-------+-----------+-----+--------------+-----+----------+-----+------+
**      + Master | Start | Addr + Wr |     | pu8_mem_addr |     | pu8_data |     | Stop |
**               +-------+-----------+-----+--------------+-----+----------+-----+------+
**      + Slave  |       |           | ACK |     (ACK)    | ACK |   (ACK)  | ACK |      |
**               +-------+-----------+-----+--------------+-----+----------+-----+------+
**
** @param [in]
**      x_inst: A specific I2C Master instance
**
** @param [in]
**      pu8_mem_addr: Address of the memory in slave to write
**
** @param [in]
**      u16_addr_len: Length in bytes of memory address stored in pu8_mem_addr
**
** @param [in]
**      pu8_data: Pointer to the buffer containing the data to send
**
** @param [in]
**      u16_data_len: Length in bytes of the data to send
**
** @return
**      @arg    I2C_OK
**      @arg    I2C_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_I2C_Write_Mem (I2C_inst_t x_inst, const uint8_t * pu8_mem_addr, uint16_t u16_addr_len,
                                            const uint8_t * pu8_data, uint16_t u16_data_len)
{
    /* Validation */
    ASSERT_PARAM (b_I2C_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM ((pu8_mem_addr != NULL) && (u16_addr_len > 0) && (pu8_data != NULL) && (u16_data_len > 0));

    /* Prevent multiple access to the same I2C bus at the same time */
    if (xSemaphoreTake (g_ax_port_sem[x_inst->x_i2c_port], pdMS_TO_TICKS (I2C_BUS_WAIT_TIMEOUT)) == pdTRUE)
    {
        i2c_cmd_handle_t x_cmd = i2c_cmd_link_create ();

        /* Start signal */
        ESP_ERROR_CHECK (i2c_master_start (x_cmd));

        /* Slave address and write bit */
        ESP_ERROR_CHECK (i2c_master_write_byte (x_cmd, (uint8_t)((x_inst->u16_slave_addr << 1) | I2C_MASTER_WRITE), true));

        /* Memory address */
        ESP_ERROR_CHECK (i2c_master_write (x_cmd, pu8_mem_addr, u16_addr_len, true));

        /* Write data */
        ESP_ERROR_CHECK (i2c_master_write (x_cmd, pu8_data, u16_data_len, true));

        /* Stop signal */
        ESP_ERROR_CHECK (i2c_master_stop (x_cmd));

        /* Execute command */
        esp_err_t x_err = i2c_master_cmd_begin (x_inst->x_i2c_port, x_cmd, pdMS_TO_TICKS (I2C_BUS_WAIT_TIMEOUT));

        /* Clean up */
        i2c_cmd_link_delete (x_cmd);

        /* We have done using the I2C bus, release the relevant semaphore */
        xSemaphoreGive (g_ax_port_sem[x_inst->x_i2c_port]);

        /* Check result */
        if (x_err != ESP_OK)
        {
            return I2C_ERR;
        }
        return I2C_OK;
    }

    /* It's take too long waiting for the I2C bus to be available */
    return I2C_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Reads a block of data from the corresponding slave
**
** @note
**      To check if the slave respond to a read command, pu8_data can be NULL (or u16_len can be zero)
**
** @details
**      Data on I2C bus:
**               +-------+-----------+-----+----------+------+------+
**      + Master | Start | Addr + Rd |     |   (ACK)  | NACK | Stop |
**               +-------+-----------+-----+----------+------+------+
**      + Slave  |       |           | ACK | pu8_data |      |      |
**               +-------+-----------+-----+----------+------+------+
**
** @param [in]
**      x_inst: A specific I2C Master instance
**
** @param [out]
**      pu8_data: The buffer to store the read data
**
** @param [in]
**      u16_len: Length in bytes of the data to read
**
** @return
**      @arg    I2C_OK
**      @arg    I2C_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_I2C_Read (I2C_inst_t x_inst, uint8_t * pu8_data, uint16_t u16_len)
{
    /* Validation */
    ASSERT_PARAM (b_I2C_Is_Valid_Inst (x_inst) && x_inst->b_initialized);

    /* Prevent multiple access to the same I2C bus at the same time */
    if (xSemaphoreTake (g_ax_port_sem[x_inst->x_i2c_port], pdMS_TO_TICKS (I2C_BUS_WAIT_TIMEOUT)) == pdTRUE)
    {
        i2c_cmd_handle_t x_cmd = i2c_cmd_link_create ();

        /* Start signal */
        ESP_ERROR_CHECK (i2c_master_start (x_cmd));

        /* Slave address and read bit */
        ESP_ERROR_CHECK (i2c_master_write_byte (x_cmd, (uint8_t)((x_inst->u16_slave_addr << 1) | I2C_MASTER_READ), true));

        /* Read data */
        if ((pu8_data != NULL) && (u16_len > 0))
        {
            if (u16_len > 1)
            {
                ESP_ERROR_CHECK (i2c_master_read (x_cmd, pu8_data, u16_len - 1, I2C_MASTER_ACK));
            }
            ESP_ERROR_CHECK (i2c_master_read_byte (x_cmd, &pu8_data[u16_len - 1], I2C_MASTER_NACK));
        }

        /* Stop signal */
        ESP_ERROR_CHECK (i2c_master_stop (x_cmd));

        /* Execute command */
        esp_err_t x_err = i2c_master_cmd_begin (x_inst->x_i2c_port, x_cmd, pdMS_TO_TICKS (I2C_BUS_WAIT_TIMEOUT));

        /* Clean up */
        i2c_cmd_link_delete (x_cmd);

        /* We have done using the I2C bus, release the relevant semaphore */
        xSemaphoreGive (g_ax_port_sem[x_inst->x_i2c_port]);

        /* Check result */
        if (x_err != ESP_OK)
        {
            return I2C_ERR;
        }
        return I2C_OK;
    }

    /* It's take too long waiting for the I2C bus to be available */
    return I2C_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Reads a block of data from the corresponding slave
**
** @details
**      Data on I2C bus:
**               +-------+-----------+-----+--------------+-----+-------+-----------+-----+----------+------+------+
**      + Master | Start | Addr + Wr |     | pu8_mem_addr |     | Start | Addr + Rd |     |   (ACK)  | NACK | Stop |
**               +-------+-----------+-----+--------------+-----+-------+-----------+-----+----------+------+------+
**      + Slave  |       |           | ACK |     (ACK)    | ACK |       |           | ACK | pu8_data |      |      |
**               +-------+-----------+-----+--------------+-----+-------+-----------+-----+----------+------+------+
**
** @param [in]
**      x_inst: A specific I2C Master instance
**
** @param [in]
**      pu8_mem_addr: Address of the memory in slave to read
**
** @param [in]
**      u16_addr_len: Length in bytes of memory address stored in pu8_mem_addr
**
** @param [out]
**      pu8_data: The buffer to store the read data
**
** @param [in]
**      u16_data_len: Length in bytes of the data to read
**
** @return
**      @arg    I2C_OK
**      @arg    I2C_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_I2C_Read_Mem (I2C_inst_t x_inst, const uint8_t * pu8_mem_addr, uint16_t u16_addr_len,
                                           uint8_t * pu8_data, uint16_t u16_data_len)
{
    /* Validation */
    ASSERT_PARAM (b_I2C_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM ((pu8_mem_addr != NULL) && (u16_addr_len > 0) && (pu8_data != NULL) && (u16_data_len > 0));

    /* Prevent multiple access to the same I2C bus at the same time */
    if (xSemaphoreTake (g_ax_port_sem[x_inst->x_i2c_port], pdMS_TO_TICKS (I2C_BUS_WAIT_TIMEOUT)) == pdTRUE)
    {
        i2c_cmd_handle_t x_cmd = i2c_cmd_link_create ();

        /* Start signal */
        ESP_ERROR_CHECK (i2c_master_start (x_cmd));

        /* Slave address and write bit */
        ESP_ERROR_CHECK (i2c_master_write_byte (x_cmd, (uint8_t)((x_inst->u16_slave_addr << 1) | I2C_MASTER_WRITE), true));

        /* Memory address */
        ESP_ERROR_CHECK (i2c_master_write (x_cmd, pu8_mem_addr, u16_addr_len, true));

        /* Restart signal */
        ESP_ERROR_CHECK (i2c_master_start (x_cmd));

        /* Slave address and read bit */
        ESP_ERROR_CHECK (i2c_master_write_byte (x_cmd, (uint8_t)((x_inst->u16_slave_addr << 1) | I2C_MASTER_READ), true));

        /* Read data */
        if (u16_data_len > 1)
        {
            ESP_ERROR_CHECK (i2c_master_read (x_cmd, pu8_data, u16_data_len - 1, I2C_MASTER_ACK));
        }
        ESP_ERROR_CHECK (i2c_master_read_byte (x_cmd, &pu8_data[u16_data_len - 1], I2C_MASTER_NACK));

        /* Stop signal */
        ESP_ERROR_CHECK (i2c_master_stop (x_cmd));

        /* Execute command */
        esp_err_t x_err = i2c_master_cmd_begin (x_inst->x_i2c_port, x_cmd, pdMS_TO_TICKS (I2C_BUS_WAIT_TIMEOUT));

        /* Clean up */
        i2c_cmd_link_delete (x_cmd);

        /* We have done using the I2C bus, release the relevant semaphore */
        xSemaphoreGive (g_ax_port_sem[x_inst->x_i2c_port]);

        /* Check result */
        if (x_err != ESP_OK)
        {
            return I2C_ERR;
        }
        return I2C_OK;
    }

    /* It's take too long waiting for the I2C bus to be available */
    return I2C_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Changes I2C address of the corresponding slave
**
** @param [in]
**      x_inst: A specific I2C Master instance
**
** @param [in]
**      u16_slave_addr: New slave address (7-bit address without read/write bit)
**
** @return
**      @arg    I2C_OK
**      @arg    I2C_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_I2C_Set_Slave_Addr (I2C_inst_t x_inst, uint16_t u16_slave_addr)
{
    /* Validation */
    ASSERT_PARAM (b_I2C_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM (u16_slave_addr < 128);

    /* Change slave address */
    x_inst->u16_slave_addr = u16_slave_addr;

    return I2C_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Hwa_I2C_Master module
**
** @return
**      @arg    I2C_OK
**      @arg    I2C_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_I2C_Init_Module (void)
{
    /* Create mutexs protecting multiple accesses to one I2C bus */
    for (uint8_t u8_idx = 0; u8_idx < I2C_NUM_MAX; u8_idx++)
    {
        g_ax_port_sem[u8_idx] = xSemaphoreCreateMutex ();
        if (g_ax_port_sem[u8_idx] == NULL)
        {
            return I2C_ERR;
        }
    }

    /* Configure all I2C ports used */
    uint8_t u8_num_ports = sizeof (g_astru_i2c_ports) / sizeof (g_astru_i2c_ports[0]);
    for (uint8_t u8_idx = 0; u8_idx < u8_num_ports; u8_idx++)
    {
        /* Initialize the corresponding I2C port */
        ESP_ERROR_CHECK (i2c_param_config (g_astru_i2c_ports[u8_idx].x_i2c_port,
                                           &g_astru_i2c_ports[u8_idx].stru_i2c_cfg));

        /* Install the corresponding I2C port */
        ESP_ERROR_CHECK (i2c_driver_install (g_astru_i2c_ports[u8_idx].x_i2c_port,
                                             I2C_MODE_MASTER,       // I2C Master
                                             0,                     // Rx buffer, master doesn't use this
                                             0,                     // Tx buffer, master doesn't use this
                                             0));                   // Interrupt allocation flag

        /* Increase I2C timeout to maximum value to support clock stretching */
        ESP_ERROR_CHECK (i2c_set_timeout (g_astru_i2c_ports[u8_idx].x_i2c_port, 0xFFFFF));
    }

    return I2C_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes an I2C Master instance
**
** @param [in]
**      x_inst: A specific I2C Master instance
**
** @return
**      @arg    I2C_OK
**      @arg    I2C_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_I2C_Init_Inst (I2C_inst_t x_inst)
{
    /* Do nothing */
    return I2C_OK;
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
static bool b_I2C_Is_Valid_Inst (I2C_inst_t x_inst)
{
    uint32_t u32_idx = 0;

    /* Searching instance */
    for (u32_idx = 0; u32_idx < (uint32_t)I2C_NUM_INST; u32_idx++)
    {
        if (x_inst == &g_astru_i2c_objs[u32_idx])
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
