/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_lcd_st7796s_demo.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Jun 30
**  @brief      : Implementation of Srvc_Lcd_ST7796s_Demo module
**  @namespace  : ST7796S
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup      Srvc_Lcd_ST7796s_Demo
** @brief           Encapsulates LCD ST7796S component on Itor3 EB1.1 master board
**
** @details
** <b> Instruction to use this module: </b>
**
** 1) Get the signle instance of the ST7796S controller with s8_ST7796S_Get_Inst(). When this function is called
**    the first time, the ST7796S controller shall be initialized.
**
** @note
**      In EB1.1 master board, reset signals of LCD and touch screen are connected together. This common reset signal
**      is controlled by Srvc_Touch_GT911 module. Therefore, instance of the touch screen must be gotten before
**      instance of this module. Otherwise, all ST7796S configuration made by this module shall be reset by
**      Srvc_Touch_GT911
**
** 2) To display a buffer of pixels on the LCD of ST7796S, use s8_GT911_Write_Pixels()
**
** @note
**      The suffix "_Demo" in this module name implies that this module is used for demo purpose of LCD on EB1.1 board.
**      At some point later, the module shall be reworked to have a better architecture.
**
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_lcd_st7796s_demo.h"          /* Public header of this module */
#include "srvc_io_tca9534.h"                /* TCA9534A is used to control ST7796S */
#include "hwa_gpio.h"                       /* GPIO is used to control ST7796S */

#include "string.h"                         /* Use memset(), memcpy() */

#include "freertos/FreeRTOS.h"              /* Use FreeRTOS */
#include "freertos/task.h"                  /* Use FreeRTOS task */
#include "driver/spi_master.h"              /* Use SPI master driver to send data to ST7796S */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Structure to manage a ST7796S controller object */
struct ST7796S_obj
{
    bool                    b_initialized;  //!< Specifies whether the object has been initialized or not
    bool                    b_bl_on;        //!< Indicates if LED backlight is on or off
    GPIOX_inst_t            x_gpiox_pwr;    //!< Instance of the expanded GPIO pin controlling power supply of the LCD
    GPIOX_inst_t            x_gpiox_reset;  //!< Instance of the expanded GPIO pin used to reset ST7796S
    GPIOX_inst_t            x_gpiox_csx;    //!< Instance of the expanded GPIO pin connected to CSX signal of ST7796S
    GPIOX_inst_t            x_gpiox_bl;     //!< Instance of the expanded GPIO pin controlling backlight of the LCD
    GPIO_inst_t             x_gpio_dcx;     //!< Instance of the GPIO pin connected to DCX signal of ST7796S
    spi_device_handle_t     x_spi_master;   //!< Handle of SPI master driver used to sending data to ST7796S
};

/** @brief  Level of DCX signal */
enum
{
    ST7796S_DCX_COMMAND     = 0,            //!< ST7796S interpretes received data on SPI bus as command
    ST7796S_DCX_PARAM       = 1,            //!< ST7796S interpretes received data on SPI bus as parameter
};

/** @brief  Some commands of ST7796S */
enum
{
    ST7796S_CMD_SWRESET     = 0x01,         //!< Software reset
    ST7796S_CMD_SLPOUT      = 0x11,         //!< Sleep out
    ST7796S_CMD_NORON       = 0x13,         //!< Normal Display Mode On
    ST7796S_CMD_DISPON      = 0x29,         //!< Display on
    ST7796S_CMD_CASET       = 0x2A,         //!< Column address set
    ST7796S_CMD_RASET       = 0x2B,         //!< Row address set
    ST7796S_CMD_RAMWR       = 0x2C,         //!< Memory write
    ST7796S_CMD_MADCTL      = 0x36,         //!< Memory data access control
    ST7796S_CMD_IDMOFF      = 0x38,         //!< Idle mode off
    ST7796S_CMD_IPF         = 0x3A,         //!< Interface pixel format
    ST7796S_CMD_DFC         = 0xB6,         //!< Display function control
    ST7796S_CMD_PWR2        = 0xC1,         //!< Power Control 2
    ST7796S_CMD_PWR3        = 0xC2,         //!< Power Control 3
    ST7796S_CMD_VCMPCTL     = 0xC5,         //!< Vcom Control
    ST7796S_CMD_PGC         = 0xE0,         //!< Positive Gamma Control
    ST7796S_CMD_NGC         = 0xE1,         //!< Negative Gamma Control
    ST7796S_CMD_DOCA        = 0xE8,         //!< Display Output CTRL Adjust
    ST7796S_CMD_CSCON       = 0xF0,         //!< Command Set Control

};

/** @brief  Macro sending a command without any parameter to ST7796S */
#define ST7796S_WRITE_CMD(u8_cmd)           v_ST7796S_Write_Command (x_inst, u8_cmd, NULL, 0)

/**
** @brief   Macro sending a command with parameters to ST7796S
** @note    This macro requires a DMA capable variable (DMA_ATTR static) named au8_params_dma to be available, e.g:
**          DMA_ATTR static uint8_t au8_params_dma[16];
*/
#define ST7796S_WRITE_CMD_PARAMS(u8_cmd, ...)                                   \
{                                                                               \
    uint8_t au8_params[] = { __VA_ARGS__ };                                     \
    ASSERT_PARAM (sizeof (au8_params) <= sizeof (au8_params_dma))               \
    memcpy (au8_params_dma, au8_params, sizeof (au8_params));                   \
    v_ST7796S_Write_Command (x_inst, u8_cmd, au8_params, sizeof(au8_params));   \
}

/** @brief  Maximum number of bytes transfered in one SPI transaction. Note that each pixel is an uint16_t */
#define ST7796S_MAX_TRANS_SIZE              (CONFIG_LCD_SPI_MAX_TRANSFER_PIXELS * sizeof (uint16_t))

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Lcd_ST7796s";

/** @brief  Indicates if this module has been initialized */
static bool g_b_initialized = false;

/** @brief  Single instance of ST7796S controller */
static struct ST7796S_obj g_stru_st7796s_obj =
{
    .b_initialized          = false,
    .b_bl_on                = false,
    .x_gpiox_pwr            = NULL,
    .x_gpiox_reset          = NULL,
    .x_gpiox_csx            = NULL,
    .x_gpiox_bl             = NULL,
    .x_gpio_dcx             = NULL,
    .x_spi_master           = NULL,
};

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifdef USE_MODULE_ASSERT
 static bool b_ST7796S_Is_Valid_Inst (ST7796S_inst_t x_inst);
#endif

static int8_t s8_ST7796S_Init_Module (void);
static int8_t s8_ST7796S_Init_Inst (ST7796S_inst_t x_inst);
static void v_ST7796S_Init_Chip (ST7796S_inst_t x_inst);
static void v_ST7796S_Spi_Pre_Transfer_Cb (spi_transaction_t * pstru_trans);
static void v_ST7796S_Write_Command (ST7796S_inst_t x_inst, uint8_t u8_cmd,
                                     const uint8_t * pu8_params, uint32_t u32_num_params);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets the single instance of ST7796S controller.
**
** @note
**      In EB1.1 master board, Reset signals of LCD and touch screen are connected together. This common reset signal
**      is controlled by Srvc_Touch_GT911 module. Therefore, instance of the touch screen must be gotten before
**      instance of this module. Otherwise, all ST7796S configuration made by this module shall be reset by
**      Srvc_Touch_GT911
**
** @param [out]
**      px_inst: Container to store the retrieved instance
**
** @return
**      @arg    ST7796S_OK
**      @arg    ST7796S_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_ST7796S_Get_Inst (ST7796S_inst_t * px_inst)
{
    ST7796S_inst_t  x_inst = NULL;
    int8_t          s8_result = ST7796S_OK;

    /* Validation */
    ASSERT_PARAM (px_inst != NULL);

    /* Initialize */
    *px_inst = NULL;

    /* If this module has not been initialized, do that now */
    if (s8_result >= ST7796S_OK)
    {
        if (!g_b_initialized)
        {
            s8_result = s8_ST7796S_Init_Module ();
            if (s8_result >= ST7796S_OK)
            {
                g_b_initialized = true;
            }
        }
    }

    /* If the retrieved instance has not been initialized yet, do that now */
    if (s8_result >= ST7796S_OK)
    {
        x_inst = &g_stru_st7796s_obj;
        if (!x_inst->b_initialized)
        {
            s8_result = s8_ST7796S_Init_Inst (x_inst);
            if (s8_result >= ST7796S_OK)
            {
                x_inst->b_initialized = true;
            }
        }
    }

    /* Return instance of the ST7796s controller */
    if (s8_result >= ST7796S_OK)
    {
        *px_inst = x_inst;
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Displays a buffer of pixels to ST7796S controller from point (xs, ys) to point (xe, ye)
**
** @note
**      (xs, ys) is start point and (xe, ye) is end point, i.e xs <= xe and ys <= ye
**
** @param [in]
**      x_inst: A specific ST7796S instance
**
** @param [in]
**      u16_xs: X-coordinate of start point
**
** @param [in]
**      u16_ys: Y-coordinate of start point
**
** @param [in]
**      u16_xe: X-coordinate of end point
**
** @param [in]
**      u16_ye: Y-coordinate of end point
**
** @param [in]
**      pstru_buffer: The buffer of pixels to display.
**                    Note that number of pixels in the buffer must be: (u16_xe - u16_xs + 1) * (u16_ye - u16_ys + 1)
**
** @note
**      pstru_buffer MUST be in DMA capable memory
**
** @return
**      @arg    ST7796S_OK
**      @arg    ST7796S_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_ST7796S_Write_Pixels (ST7796S_inst_t x_inst, uint16_t u16_xs, uint16_t u16_ys,
                                uint16_t u16_xe, uint16_t u16_ye, const ST7796S_pixel_t * pstru_buffer)
{
    DMA_ATTR static uint8_t au8_params_dma[4];

    /* Validation */
    ASSERT_PARAM (b_ST7796S_Is_Valid_Inst (x_inst) && x_inst->b_initialized);
    ASSERT_PARAM ((pstru_buffer != NULL) && (u16_xs <= u16_xe) && (u16_ys <= u16_ye));

    /* Set start and end column addresses */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_CASET, (u16_xs >> 8) & 0xFF, u16_xs & 0xFF,
                                                 (u16_xe >> 8) & 0xFF, u16_xe & 0xFF);

    /* Set start and end row addresses */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_RASET, (u16_ys >> 8) & 0xFF, u16_ys & 0xFF,
                                                 (u16_ye >> 8) & 0xFF, u16_ye & 0xFF);

    /* Transfer pixel data to frame memory */
    uint16_t u16_num_params = (u16_xe - u16_xs + 1) * (u16_ye - u16_ys + 1) * sizeof (uint16_t);
    v_ST7796S_Write_Command (x_inst, ST7796S_CMD_RAMWR, (uint8_t *)pstru_buffer, u16_num_params);

    return ST7796S_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Toggles LCD's LED backlight
**
** @note
**      LCD's LED backlight is turned on by default upon bootup
**
** @param [in]
**      x_inst: A specific ST7796S instance
**
** @param [in]
**      b_on
**      @arg    true: Turn on LED backlight
**      @arg    false: Turn off LED backlight
**
** @return
**      @arg    ST7796S_OK
**      @arg    ST7796S_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_ST7796S_Toggle_Backlight (ST7796S_inst_t x_inst, bool b_on)
{
    /* Validation */
    ASSERT_PARAM (b_ST7796S_Is_Valid_Inst (x_inst) && x_inst->b_initialized);

    /* Do nothing if LED backlight is already at the requested state */
    if (b_on == x_inst->b_bl_on)
    {
        return ST7796S_OK;
    }

    /* Toggle LED backlight */
    if (s8_GPIOX_Write_Active (x_inst->x_gpiox_bl, b_on) != GPIOX_OK)
    {
        return ST7796S_ERR;
    }

    /* Done */
    x_inst->b_bl_on = b_on;
    return ST7796S_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_Lcd_ST7796s_Demo module
**
** @return
**      @arg    ST7796S_OK
**      @arg    ST7796S_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_ST7796S_Init_Module (void)
{
    /* Do nothing */
    return ST7796S_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes ST7796s instance
**
** @param [in]
**      x_inst: A specific ST7796s instance
**
** @return
**      @arg    ST7796S_OK
**      @arg    ST7796S_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_ST7796S_Init_Inst (ST7796S_inst_t x_inst)
{
    /* Get instance of the expanded GPIO pin controlling power supply of the LCD */
    if (s8_GPIOX_Get_Inst (GPIOX_LCD_CAM_PWR, &x_inst->x_gpiox_pwr) != GPIOX_OK)
    {
        return ST7796S_ERR;
    }

    /* Get instance of the expanded GPIO connected to Reset signal of ST7796S */
    if (s8_GPIOX_Get_Inst (GPIOX_LCD_RST, &x_inst->x_gpiox_reset) != GPIOX_OK)
    {
        return ST7796S_ERR;
    }

    /* Get instance of the expanded GPIO pin connected to CS signal of ST7796S */
    if (s8_GPIOX_Get_Inst (GPIOX_LCD_CS, &x_inst->x_gpiox_csx) != GPIOX_OK)
    {
        return ST7796S_ERR;
    }

    /* Get instance of the expanded GPIO pin controlling backlight of the LCD */
    if (s8_GPIOX_Get_Inst (GPIOX_LCD_BL, &x_inst->x_gpiox_bl) != GPIOX_OK)
    {
        return ST7796S_ERR;
    }

    /* Get instance of the GPIO pin connected to DCX signal of ST7796S */
    if (s8_GPIO_Get_Inst (GPIO_ST7796S_DC, &x_inst->x_gpio_dcx) != GPIO_OK)
    {
        return ST7796S_ERR;
    }

    /* Turn on power supply of ST7796S */
    if (s8_GPIOX_Write_Active (x_inst->x_gpiox_pwr, true) != GPIOX_OK)
    {
        return ST7796S_ERR;
    }

    /* Reset ST7796S */
    s8_GPIOX_Write_Active (x_inst->x_gpiox_reset, true);
    vTaskDelay (pdMS_TO_TICKS (10));
    s8_GPIOX_Write_Active (x_inst->x_gpiox_reset, false);

    /* Turn on LED backlight of the LCD */
    x_inst->b_bl_on = true;
    if (s8_GPIOX_Write_Active (x_inst->x_gpiox_bl, true) != GPIOX_OK)
    {
        return ST7796S_ERR;
    }

    /* CSX of ST7796S is always asserted to enable SPI communication with ST7796S */
    if (s8_GPIOX_Write_Active (x_inst->x_gpiox_csx, true) != GPIOX_OK)
    {
        return ST7796S_ERR;
    }

    /* SPI bus configuration */
    spi_bus_config_t stru_bus_cfg =
    {
        .mosi_io_num = CONFIG_LCD_MOSI_IO_PIN,          // GPIO pin for Master Out Slave In
        .miso_io_num = -1,                              // GPIO pin for Master In Slave Out
        .sclk_io_num = CONFIG_LCD_CLK_IO_PIN,           // GPIO pin for Spi CLocK
        .quadwp_io_num = -1,                            // GPIO pin for WP (Write Protect)
        .quadhd_io_num = -1,                            // GPIO pin for HD (HolD)
        .max_transfer_sz = ST7796S_MAX_TRANS_SIZE,      // Maximum transfer size, in bytes, of one transaction
        .flags = 0,                                     // Abilities of bus to be checked by the driver
        .intr_flags = 0,                                // Interrupt flag for the bus
    };
    ESP_ERROR_CHECK (spi_bus_initialize (CONFIG_LCD_SPI_HOST_DEV, &stru_bus_cfg, SPI_DMA_CH_AUTO));

    /* SPI device configuration */
    spi_device_interface_config_t stru_dev_cfg =
    {
        .command_bits = 0,                              // Default amount of bits in command phase
        .address_bits = 0,                              // Default amount of bits in address phase
        .dummy_bits = 0,                                // Amount of dummy bits to insert between address and data phase
        .mode = 0,                                      // SPI mode 0: (CPOL, CPHA) = (0, 0)
        .duty_cycle_pos = 0,                            // Duty cycle of positive clock
        .cs_ena_pretrans = 0,                           // SPI bit-cycles the CS is activated before the transmission
        .cs_ena_posttrans = 0,                          // SPI bit-cycles the CS stay active after the transmission
        .clock_speed_hz = CONFIG_LCD_CLK_MHZ * 1000000, // Clock speed, divisors of 80MHz, in Hz
        .input_delay_ns = 0,                            // Maximum data valid time of slave
        .spics_io_num = -1,                             // CS GPIO pin for this device
        .flags = SPI_DEVICE_HALFDUPLEX,                 // Transmit data before receiving it, instead of simultaneously
        .queue_size = 7,                                // How many transactions can be queued
        .pre_cb = v_ST7796S_Spi_Pre_Transfer_Cb,        // Callback to be called before a transmission is started
        .post_cb = NULL,                                // Callback to be called after a transmission has completed
    };
    ESP_ERROR_CHECK (spi_bus_add_device (CONFIG_LCD_SPI_HOST_DEV, &stru_dev_cfg, &x_inst->x_spi_master));

    /* Initialize ST7796S */
    v_ST7796S_Init_Chip (x_inst);

    return ST7796S_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes ST7796s chip
**
** @param [in]
**      x_inst: A specific ST7796s instance
**
** @return
**      @arg    ST7796S_OK
**      @arg    ST7796S_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_ST7796S_Init_Chip (ST7796S_inst_t x_inst)
{
    DMA_ATTR static uint8_t au8_params_dma[14];

    /* Software reset */
    ST7796S_WRITE_CMD (ST7796S_CMD_SWRESET);
    vTaskDelay (pdMS_TO_TICKS (100));

    /* Enable extension Command 2 (part I and part II) */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_CSCON, 0xC3);
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_CSCON, 0x96);

    /*
    ** Memory data access control:
    ** + MY = 1         : Row Address Order
    ** + MX = 1         : Column Address Order
    ** + MV = 1         : Row/Column Exchange
    ** + ML = 0         : LCD vertical refresh Top to Bottom
    ** + RGB = 1        : BGR color filter panel
    ** + MH = 0         : LCD horizontal refresh Left to Right
    */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_MADCTL, 0xE8);

    /* Interface Pixel Format: 16bits/pixel for RGB and MCU interface */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_IPF, 0x55);

    /* Display Function Control */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_DFC, 0x80, 0x02, 0x3B);

    /*
    ** Display Output Ctrl Adjust:
    ** + S_END = 9      : Source timing Control is 22.5 us
    ** + G_EQ = 1       : Gate driver EQ function is ON
    ** + G_START = 0x19 : "Gate start" timing is 25 Tclk
    ** + G_END = 0x25   : "Gate end" timing is 37 Tclk
    */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_DOCA, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33);

    /* Power control 2: VAP(GVDD) = 3.85V + (vcom + vcom offset), VAN(GVCL) = -3.85V + (vcom + vcom offset) */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_PWR2, 0x06);

    /* Power control 3: Source driving current level (SOP) = Low, Gamma driving current level (GOP) = High */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_PWR3, 0xA7);

    /* VCOM Control: VCOM = 0.9V */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_VCMPCTL, 0x18);
    vTaskDelay (pdMS_TO_TICKS (100));

    /* Positive Gamma Control */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_PGC, 0xF0, 0x09, 0x0b, 0x06, 0x04, 0x15, 0x2F,
                                               0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B);

    /* Negative Gamma Control */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_NGC, 0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2B,
                                               0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B);
    vTaskDelay (pdMS_TO_TICKS (50));

    /* Disable extension Command 2 (part I and part II) */
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_CSCON, 0x3C);
    ST7796S_WRITE_CMD_PARAMS (ST7796S_CMD_CSCON, 0x69);

    /* Turn off sleep mode (DC/DC converter is enabled, internal display oscillator and panel scanning are started) */
    ST7796S_WRITE_CMD (ST7796S_CMD_SLPOUT);
    vTaskDelay (pdMS_TO_TICKS (50));

    /* Idle Mode Off */
    ST7796S_WRITE_CMD (ST7796S_CMD_IDMOFF);
    vTaskDelay (pdMS_TO_TICKS (50));

    /* Turn the display to normal mode */
    ST7796S_WRITE_CMD (ST7796S_CMD_NORON);
    vTaskDelay (pdMS_TO_TICKS (50));

    /* Display On (output from the Frame Memory is enabled) */
    ST7796S_WRITE_CMD (ST7796S_CMD_DISPON);
    vTaskDelay (pdMS_TO_TICKS (50));
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Callback invoked before a SPI transmission is started
**
** @note
**      This callback is invoked within interrupt context
**
** @param [in]
**      pstru_trans: The corresponding SPI transaction
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_ST7796S_Spi_Pre_Transfer_Cb (spi_transaction_t * pstru_trans)
{
    /* Drive DCX pin to the desired level which is hold by user data */
    s8_GPIO_Write_Level (g_stru_st7796s_obj.x_gpio_dcx, (uint32_t)pstru_trans->user);
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends a command to ST7796s
**
** @param [in]
**      x_inst: A specific ST7796s instance
**
** @param [in]
**      u8_cmd: Command code
**
** @param [in]
**      pu8_params: Pointer to the buffer containing parameters of this command, each parameters is an uint8_t
**                  This parameter is NULL in case the command has no parameter.
**
** @param [in]
**      u32_num_params: Number of parameters of this command.
**                      This parameter is 0 in case the command has no parameter
**
** @return
**      @arg    ST7796S_OK
**      @arg    ST7796S_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_ST7796S_Write_Command (ST7796S_inst_t x_inst, uint8_t u8_cmd,
                                     const uint8_t * pu8_params, uint32_t u32_num_params)
{
    DMA_ATTR static uint8_t u8_cmd_dma;
    spi_transaction_t       astru_spi_trans[2];
    uint8_t                 u8_trans_count = 0;

    /* Validate */
    ASSERT_PARAM (u32_num_params <= ST7796S_MAX_TRANS_SIZE);

    /* Initialize SPI transaction memory */
    memset (astru_spi_trans, 0x00, sizeof (astru_spi_trans));
    u8_cmd_dma = u8_cmd;

    /* Start SPI transaction sending command code */
    astru_spi_trans[0].flags = 0;
    astru_spi_trans[0].tx_buffer = &u8_cmd_dma;
    astru_spi_trans[0].length = sizeof (u8_cmd_dma) * 8;
    astru_spi_trans[0].user = (void *)ST7796S_DCX_COMMAND;
    ESP_ERROR_CHECK (spi_device_queue_trans (x_inst->x_spi_master, &astru_spi_trans[0], portMAX_DELAY));
    u8_trans_count++;

    /* Start SPI transaction sending command's parameters (if any) */
    if ((u32_num_params > 0) && (pu8_params != NULL))
    {
        astru_spi_trans[1].flags = 0;
        astru_spi_trans[1].tx_buffer = pu8_params;
        astru_spi_trans[1].length = u32_num_params * 8;
        astru_spi_trans[1].user = (void *)ST7796S_DCX_PARAM;
        ESP_ERROR_CHECK (spi_device_queue_trans (x_inst->x_spi_master, &astru_spi_trans[1], portMAX_DELAY));
        u8_trans_count++;
    }

    /* Wait for all transaction to finish */
    for (uint8_t u8_idx = 0; u8_idx < u8_trans_count; u8_idx++)
    {
        spi_transaction_t * pstru_spi_trans;
        ESP_ERROR_CHECK (spi_device_get_trans_result (x_inst->x_spi_master, &pstru_spi_trans, portMAX_DELAY));
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
static bool b_ST7796S_Is_Valid_Inst (ST7796S_inst_t x_inst)
{
    return (x_inst == &g_stru_st7796s_obj);
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
