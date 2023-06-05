/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_master_datalink.c
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2021 Dec 13
**  @brief      : Implementation of Srvc_Master_Datalink module
**  @namespace  : MDL
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_Master_Datalink
** @brief       Abstracts data-link layer (client side) of Bootloader protocol
** @{
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "srvc_master_datalink.h"       /* Public header of this module */
#include "driver/uart.h"                /* Use ESP-IDF's UART driver component */
#include "driver/gpio.h"                /* Use ESP-IDF's GPIO driver component */

#include "freertos/FreeRTOS.h"          /* Use FreeRTOS */
#include "freertos/task.h"              /* Use FreeRTOS task */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Maximum number of callback functions */
#define MDL_NUM_CB          1

/** @brief  Structure wrapping data of a Master data-link channel */
struct MDL_obj
{
    bool                    b_initialized;              //!< Specifies whether the object has been initialized or not
    bool                    b_raw_mode;                 //!< Whether raw mode is enabled
    MDL_cb_t                apfnc_cb [MDL_NUM_CB];      //!< Callback function invoked when an event occurs
};

/** @brief  Structure of Master data-link packet */
typedef struct
{
    uint8_t                 au8_sof[4];                 //!< Start of frame
    uint8_t                 u8_type;                    //!< Frame type
    uint8_t                 u8_len;                     //!< Frame length
    uint16_t                u16_cks;                    //!< LRC checksum
    uint8_t                 au8_payload[];              //!< Data-link payload

} MDL_pkt_t;

/** @brief  Octets of Start-Of-Frame pattern */
enum
{
    MDL_SOF_1               = 0xAA,
    MDL_SOF_2               = 0x33,
    MDL_SOF_3               = 0x55,
    MDL_SOF_4               = 0xCC,
    MDL_SOF_STUFF           = 0xFF
};

/**
** @brief   Index of the UART port used
** @note    This must be the same port that FreeModbus module uses
*/
#define MDL_UART_PORT                   (CONFIG_MB_UART_PORT_NUM)

/** @brief  UART pin mapping */
#define MDL_UART_TXD_PIN                (CONFIG_MB_UART_TXD)
#define MDL_UART_RXD_PIN                (CONFIG_MB_UART_RXD)

/** @brief  Baudrate of UART interface for Master data-link */
#define MDL_UART_BAUD_RATE              115200

/** @brief  Communication window interval in milliseconds */
#define MDL_COMM_WINDOW                 30

/** @brief  Size in bytes of UART TX ring buffer */
#define MDL_UART_TX_RING_BUF_SIZE       1024

/** @brief  Size in bytes of UART RX ring buffer */
#define MDL_UART_RX_RING_BUF_SIZE       1024

/** @brief  Maximum length in bytes of a Master data-link packet */
#define MDL_MAX_PKT_LEN                 255

/** @brief  Maximum number of stuff bytes in a packet */
#define MDL_MAX_STUFF_OCTETS            32

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/** @brief  Logging tag of this module */
static const char * TAG = "Srvc_Master_Datalink";

/** @brief  Single instance of Master data-link channel */
static struct MDL_obj g_stru_mdl_obj =
{
    .b_initialized      = false,
    .b_raw_mode         = false,
    .apfnc_cb           = { NULL }
};

/** @brief  Indicates if this module has been initialized or not */
static bool g_b_initialized = false;

/** @brief  Semaphore protecting UART Tx */
static SemaphoreHandle_t g_x_sem_tx;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifdef USE_MODULE_ASSERT
 static bool b_MDL_Is_Valid_Inst (MDL_inst_t x_inst);
#endif

static int8_t s8_MDL_Init_Module (void);
static int8_t s8_MDL_Init_Inst (MDL_inst_t x_inst);
static bool b_MDL_Process_Rx_Data (MDL_inst_t x_inst, uint8_t u8_octet);
static int8_t s8_MDL_Construct_Packet (const void * pv_payload, uint16_t u16_payload_len,
                                         uint8_t * pu8_packet, uint16_t * pu16_packet_len);
static uint16_t u16_MDL_Cal_Checksum (const void * pv_data, uint16_t u16_len);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Gets instance of Master data-link channel
**
** @param [out]
**      px_inst: Container to store the retrieved instance
**
** @return
**      @arg    MDL_OK
**      @arg    MDL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MDL_Get_Inst (MDL_inst_t * px_inst)
{
    MDL_inst_t  x_inst = NULL;
    int8_t      s8_result = MDL_OK;

    /* Validation */
    ASSERT_PARAM (px_inst != NULL);

    /* Initialize */
    *px_inst = NULL;

    /* If this module has not been initialized, do that now */
    if (s8_result >= MDL_OK)
    {
        if (!g_b_initialized)
        {
            s8_result = s8_MDL_Init_Module ();
            if (s8_result >= MDL_OK)
            {
                g_b_initialized = true;
            }
        }
    }

    /* If the retrieved instance has not been initialized yet, do that now */
    if (s8_result >= MDL_OK)
    {
        x_inst = &g_stru_mdl_obj;
        if (!x_inst->b_initialized)
        {
            s8_result = s8_MDL_Init_Inst (x_inst);
            if (s8_result >= MDL_OK)
            {
                x_inst->b_initialized = true;
            }
        }
    }

    /* Return instance of the Master data-link channel */
    if (s8_result >= MDL_OK)
    {
        *px_inst = x_inst;
    }

    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Runs Master data-link channel
**
** @note
**      This function must be called periodically for UART receiver to work
**
** @param [in]
**      x_inst: Specific instance
**
** @return
**      @arg    MDL_OK
**      @arg    MDL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MDL_Run_Inst (MDL_inst_t x_inst)
{
    static uint8_t  au8_rx_packet[32];
    int16_t         s16_rx_len;

    /* Validation */
    ASSERT_PARAM (b_MDL_Is_Valid_Inst (x_inst));

    /* Get and process the UART data received if any */
    do
    {
        s16_rx_len = uart_read_bytes (MDL_UART_PORT, au8_rx_packet, sizeof (au8_rx_packet), 0);

        /* Process each byte of the received data */
        for (int16_t s16_idx = 0; s16_idx < s16_rx_len; s16_idx++)
        {
            b_MDL_Process_Rx_Data (x_inst, au8_rx_packet[s16_idx]);
        }
    }
    while (s16_rx_len > 0);

    return MDL_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Registers callack function to a Master data-link channel
**
** @note
**      This function is not thread-safe
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pfnc_cb: Pointer of the callback function
**
** @return
**      @arg    MDL_OK
**      @arg    MDL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MDL_Register_Cb (MDL_inst_t x_inst, MDL_cb_t pfnc_cb)
{
    ASSERT_PARAM (b_MDL_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized && (pfnc_cb != NULL));

    /* Store callback function pointer */
    for (uint8_t u8_idx = 0; u8_idx < MDL_NUM_CB; u8_idx++)
    {
        if (x_inst->apfnc_cb [u8_idx] == NULL)
        {
            x_inst->apfnc_cb [u8_idx] = pfnc_cb;
            return MDL_OK;
        }
    }

    LOGE ("Failed to register callback function");
    return MDL_ERR;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends data to a Master data-link channel
**
** @note
**      This function only works while raw mode is disabled
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pv_data: The data to send
**
** @param [in]
**      u16_len: Length in bytes of pv_data
**
** @return
**      @arg    MDL_OK
**      @arg    MDL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MDL_Send (MDL_inst_t x_inst, const void * pv_data, uint16_t u16_len)
{
    static uint8_t  au8_packet [MDL_MAX_PKT_LEN + MDL_MAX_STUFF_OCTETS];
    int8_t          s8_result = MDL_OK;

    /* Validation */
    ASSERT_PARAM (b_MDL_Is_Valid_Inst (x_inst));
    ASSERT_PARAM (x_inst->b_initialized && (pv_data != NULL) && (u16_len != 0));

    /* Do nothing if raw mode is enabled */
    if (x_inst->b_raw_mode)
    {
        return MDL_ERR;
    }

    /* Prevent race condition of concurent accesses to data link layer */
    xSemaphoreTake (g_x_sem_tx, portMAX_DELAY);

    /* Construct packet to send */
    uint16_t u16_pkt_len = sizeof (au8_packet);
    s8_result = s8_MDL_Construct_Packet (pv_data, u16_len, au8_packet, &u16_pkt_len);
    if (s8_result == MDL_OK)
    {
        /* Send the Tx packet */
        uart_write_bytes (MDL_UART_PORT, au8_packet, u16_pkt_len);
    }

    /* Release semaphore */
    xSemaphoreGive (g_x_sem_tx);
    return s8_result;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Enables or disables raw mode of a channel
**
** @details
**      Raw mode is the mode that communication via Bootloader protocol is disabled, instead raw data is sent and
**      received without any protocol overhead. While raw mode is enabled, s8_MDL_Send() cannot be used, instead
**      s8_MDL_Send_Raw(), s8_MDL_Receive_Raw(), and s8_MDL_Transceive_Raw() are used.
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      b_enabled: Specifies if raw mode is to be enabled or disabled
**
** @return
**      @arg    MDL_OK
**      @arg    MDL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MDL_Toggle_Raw_Mode (MDL_inst_t x_inst, bool b_enabled)
{
    ASSERT_PARAM (b_MDL_Is_Valid_Inst (x_inst));

    if (x_inst->b_raw_mode != b_enabled)
    {
        x_inst->b_raw_mode = b_enabled;
        LOGI ("UART raw mode is %s", b_enabled ? "enabled" : "disabled");
    }

    return MDL_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends data over a channel in raw mode
**
** @note
**      This function can only be used when raw mode of the relevant channel is enabled
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pv_data: The data to send over the given channel
**
** @param [in]
**      u16_len: Length in bytes of the data to send
**
** @return
**      @arg    MDL_OK
**      @arg    MDL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MDL_Send_Raw (MDL_inst_t x_inst, const void * pv_data, uint16_t u16_len)
{
    ASSERT_PARAM (b_MDL_Is_Valid_Inst (x_inst));
    ASSERT_PARAM ((pv_data != NULL) && (u16_len > 0));

    /* This function is for raw mode only */
    if (!x_inst->b_raw_mode)
    {
        LOGE ("Raw mode is not enabled");
        return MDL_ERR;
    }

    /* Prevent race condition of concurent accesses to data link layer */
    xSemaphoreTake (g_x_sem_tx, portMAX_DELAY);

    /* Send the Tx packet */
    if (uart_write_bytes (MDL_UART_PORT, pv_data, u16_len) != u16_len)
    {
        LOGE ("Failed to send raw data over UART data-link channel");
        xSemaphoreGive (g_x_sem_tx);
        return MDL_ERR;
    }

    xSemaphoreGive (g_x_sem_tx);
    return MDL_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Receives data from a channel in raw mode
**
** @note
**      This function can only be used when raw mode of the relevant channel is enabled
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pv_data: The buffer to store the received data
**
** @param [in, out]
**      pu16_len: [In] Length in bytes of the buffer pointed by pv_data or number of data octets to receive
**                [Out] Length in bytes of the data received over the channel, 0 if there is no data
**
** @param [in]
**      u16_timeout: Timeout in (milliseconds) waiting for incomming data
**                   Use MDL_WAIT_FOREVER if want to wait forever
**
** @return
**      @arg    MDL_OK
**      @arg    MDL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MDL_Receive_Raw (MDL_inst_t x_inst, void * pv_data, uint16_t * pu16_len, uint16_t u16_timeout)
{
    ASSERT_PARAM (b_MDL_Is_Valid_Inst (x_inst));
    ASSERT_PARAM ((pu16_len != NULL) && (*pu16_len > 0));

    /* This function is for raw mode only */
    if (!x_inst->b_raw_mode)
    {
        LOGE ("Raw mode is not enabled");
        return MDL_ERR;
    }

    /* Prevent race condition of concurent accesses to data link layer */
    xSemaphoreTake (g_x_sem_tx, portMAX_DELAY);

    /* Wait for incoming data over the given channel */
    int16_t s16_rx_len = 0;
    if (u16_timeout != MDL_WAIT_FOREVER)
    {
        s16_rx_len = uart_read_bytes (MDL_UART_PORT, pv_data, *pu16_len, pdMS_TO_TICKS (u16_timeout));
    }
    else
    {
        s16_rx_len = uart_read_bytes (MDL_UART_PORT, pv_data, *pu16_len, portMAX_DELAY);
    }

    /* Check result */
    if (s16_rx_len < 0)
    {
        LOGE ("Failed to receive raw data over UART data-link channel");
        *pu16_len = 0;
        xSemaphoreGive (g_x_sem_tx);
        return MDL_ERR;
    }

    /* Done */
    *pu16_len = s16_rx_len;
    xSemaphoreGive (g_x_sem_tx);
    return MDL_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Sends data over a channel in raw mode and waits for incoming data. The function returns when the provided
**      receive buffer is full or receive timeout expires
**
** @note
**      Receive buffer is flushed before sending.
**      This function can only be used when raw mode is enabled.
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      pv_tx_data: The data to send over the given channel
**
** @param [in]
**      u16_tx_len: Length in bytes of the data to send
**
** @param [in]
**      pv_rx_data: The buffer to store the received data
**
** @param [in, out]
**      pu16_rx_len: [In] Length in bytes of the buffer pointed by pv_data or number of data octets to receive
**                   [Out] Length in bytes of the data received over the channel, 0 if there is no data
**
** @param [in]
**      u16_rx_timeout: Timeout in (milliseconds) waiting for incomming data
**                      Use MDL_WAIT_FOREVER if want to wait forever
**
** @return
**      @arg    MDL_OK
**      @arg    MDL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int8_t s8_MDL_Transceive_Raw (MDL_inst_t x_inst, const void * pv_tx_data, uint16_t u16_tx_len,
                              void * pv_rx_data, uint16_t * pu16_rx_len, uint16_t u16_rx_timeout)
{
    ASSERT_PARAM (b_MDL_Is_Valid_Inst (x_inst));
    ASSERT_PARAM ((pv_tx_data != NULL) && (u16_tx_len > 0));
    ASSERT_PARAM ((pv_rx_data != NULL) && (*pu16_rx_len > 0));

    /* This function is for raw mode only */
    if (!x_inst->b_raw_mode)
    {
        LOGE ("Raw mode is not enabled");
        return MDL_ERR;
    }

    /* Prevent race condition of concurent accesses to data link layer */
    xSemaphoreTake (g_x_sem_tx, portMAX_DELAY);

    /* Flush UART Rx ring buffer */
    uart_flush (MDL_UART_PORT);

    /* Send the Tx packet */
    if (uart_write_bytes (MDL_UART_PORT, pv_tx_data, u16_tx_len) != u16_tx_len)
    {
        LOGE ("Failed to send raw data over UART data-link channel");
        xSemaphoreGive (g_x_sem_tx);
        return MDL_ERR;
    }

    /* Wait for incoming data over the given channel until receive buffer is full or receive timeout expires */
    uint16_t u16_time_elapsed = 0;
    while (true)
    {
        vTaskDelay (pdMS_TO_TICKS (MDL_COMM_WINDOW));
        u16_time_elapsed += MDL_COMM_WINDOW;

        size_t x_rx_length = 0;
        uart_get_buffered_data_len (MDL_UART_PORT, &x_rx_length);
        if ((x_rx_length >= *pu16_rx_len) ||
            ((u16_rx_timeout != MDL_WAIT_FOREVER) && (u16_time_elapsed >= u16_rx_timeout)))
        {
            break;
        }
    }

    /* Get the incoming data (if any) */
    int16_t s16_rx_len = uart_read_bytes (MDL_UART_PORT, pv_rx_data, *pu16_rx_len, 0);

    /* Check result */
    if (s16_rx_len < 0)
    {
        LOGE ("Failed to receive raw data over UART data-link channel");
        *pu16_rx_len = 0;
        xSemaphoreGive (g_x_sem_tx);
        return MDL_ERR;
    }

    /* Done */
    *pu16_rx_len = s16_rx_len;
    xSemaphoreGive (g_x_sem_tx);
    return MDL_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes Srvc_Master_Datalink module
**
** @return
**      @arg    MDL_OK
**      @arg    MDL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MDL_Init_Module (void)
{
    /*
    ** UART interface may be already initialized by FreeModbus module. In that case, we just use it.
    ** Otherwise (e.g, FreeModbus module is not used), we need to do the initialization.
    */
    if (!uart_is_driver_installed (MDL_UART_PORT))
    {
        LOGW ("UART interface is not initialized yet. Initializing it...");

        /* Configure UART driver */
        uart_config_t stru_uart_config =
        {
            .baud_rate  = MDL_UART_BAUD_RATE,
            .data_bits  = UART_DATA_8_BITS,
            .parity     = UART_PARITY_DISABLE,
            .stop_bits  = UART_STOP_BITS_2,
            .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
        };
        ESP_ERROR_CHECK (uart_set_pin (MDL_UART_PORT, MDL_UART_TXD_PIN, MDL_UART_RXD_PIN,
                                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK (uart_param_config (MDL_UART_PORT, &stru_uart_config));
        ESP_ERROR_CHECK (uart_driver_install (MDL_UART_PORT,
                                              MDL_UART_RX_RING_BUF_SIZE, MDL_UART_TX_RING_BUF_SIZE, 0, NULL, 0));
        ESP_ERROR_CHECK (uart_set_mode (MDL_UART_PORT, UART_MODE_UART));
    }

    /* Create mutex preventing race condition of multiple accesses to data-link layer */
    g_x_sem_tx = xSemaphoreCreateMutex();

    return MDL_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Initializes a Master data-link channel
**
** @param [in]
**      x_inst: Specific instance
**
** @return
**      @arg    MDL_OK
**      @arg    MDL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MDL_Init_Inst (MDL_inst_t x_inst)
{
    /* Initialize callback function pointers */
    for (uint8_t u8_idx = 0; u8_idx < MDL_NUM_CB; u8_idx++)
    {
        x_inst->apfnc_cb [u8_idx] = NULL;
    }

    return MDL_OK;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Processes each byte of UART received data
**
** @param [in]
**      x_inst: Specific instance
**
** @param [in]
**      u8_octet: New UART data byte received
**
** @return
**      @arg    true: a data-link packet has been received completely
**      @arg    false: still waiting for more data of the Rx packet
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static bool b_MDL_Process_Rx_Data (MDL_inst_t x_inst, uint8_t u8_octet)
{
    static uint8_t      au8_packet [MDL_MAX_PKT_LEN];
    static uint16_t     u16_len = 0;
    static bool         b_stuff_byte_received = false;
    MDL_pkt_t *         pstru_pkt = (MDL_pkt_t *)au8_packet;

    /* If packet length is too long (which is not likely), discard it */
    if (u16_len >= MDL_MAX_PKT_LEN)
    {
        au8_packet [0] = au8_packet [MDL_MAX_PKT_LEN - 4];
        au8_packet [1] = au8_packet [MDL_MAX_PKT_LEN - 3];
        au8_packet [2] = au8_packet [MDL_MAX_PKT_LEN - 2];
        au8_packet [3] = au8_packet [MDL_MAX_PKT_LEN - 1];
        u16_len = 4;
    }

    /* Check if Start-Of-Frame pattern appears in the received data */
    if ((u16_len > 4) &&
        (au8_packet [u16_len - 4] == MDL_SOF_1) &&
        (au8_packet [u16_len - 3] == MDL_SOF_2) &&
        (au8_packet [u16_len - 2] == MDL_SOF_3) &&
        (au8_packet [u16_len - 1] == MDL_SOF_4) &&
        (b_stuff_byte_received == false))
    {
        if (u8_octet == MDL_SOF_STUFF)
        {
            /* Received octet is a stuff byte, just ignore the received octet */
            b_stuff_byte_received = true;
        }
        else
        {
            /* New UART packet is received */
            au8_packet [0] = MDL_SOF_1;
            au8_packet [1] = MDL_SOF_2;
            au8_packet [2] = MDL_SOF_3;
            au8_packet [3] = MDL_SOF_4;
            au8_packet [4] = u8_octet;
            u16_len = 5;
        }
    }
    else
    {
        /* Put the received octet to receive buffer */
        au8_packet [u16_len++] = u8_octet;
        b_stuff_byte_received = false;

        /* Check if a completed packet has been received */
        if ((u16_len >= sizeof (MDL_pkt_t)) && (u16_len == pstru_pkt->u8_len))
        {
            /* Validate checksum */
            uint16_t u16_cks = pstru_pkt->u16_cks;
            pstru_pkt->u16_cks = 0;
            if (u16_MDL_Cal_Checksum (pstru_pkt, pstru_pkt->u8_len) == u16_cks)
            {
                /* A valid data-link packet has been received, pass it to other modules for further processing */
                for (uint8_t u8_idx = 0; u8_idx < MDL_NUM_CB; u8_idx++)
                {
                    if (x_inst->apfnc_cb [u8_idx] != NULL)
                    {
                        x_inst->apfnc_cb [u8_idx] (x_inst, MDL_EVT_MSG_RECEIVED,
                                                   pstru_pkt->au8_payload, pstru_pkt->u8_len - sizeof (MDL_pkt_t));
                    }
                }

                /* Start waiting for new packet */
                u16_len = 0;
                return true;
            }
            else
            {
                LOGW ("Invalid checksum");
            }
        }
    }

    return false;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Calculates LRC checksum of a block of data
**
** @param [in]
**      pv_data: The data to calculate checksum
**
** @param [in]
**      u16_len: Length in bytes of pv_data
**
** @return
**      LRC checksum of the data block
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static uint16_t u16_MDL_Cal_Checksum (const void * pv_data, uint16_t u16_len)
{
    uint16_t    u16_checksum = 0;
    uint8_t *   pu8_data = (uint8_t *)pv_data;

    for (uint16_t u16_idx = 0; u16_idx < u16_len; u16_idx++)
    {
        u16_checksum += pu8_data[u16_idx];
    }
    u16_checksum = ~u16_checksum;

    return u16_checksum;
}

/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
** @brief
**      Constructs a packet of data-link channel provided data payload
**
** @param [in]
**      pv_data: Data-link payload
**
** @param [in]
**      u16_data_len: Length in bytes of pv_data
**
** @param [out]
**      pu8_packet: The buffer to contain the data-link packet. This buffer must have large enough size to store the
**                  result packet.
**
** @param [in, out]
**      pu16_packet_len: [In] Length in bytes of the buffer pointed by pu8_packet
**                       [Out] Length in bytes of the constructed packet
**
** @return
**      @arg    MDL_OK
**      @arg    MDL_ERR
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int8_t s8_MDL_Construct_Packet (const void * pv_payload, uint16_t u16_payload_len,
                                         uint8_t * pu8_packet, uint16_t * pu16_packet_len)
{
    uint8_t *       pu8_payload = (uint8_t *)pv_payload;
    MDL_pkt_t *     pstru_pkt = (MDL_pkt_t *)pu8_packet;
    uint16_t        u16_checksum = 0;
    uint16_t        u16_pkt_len = 0;

    /* Validation */
    ASSERT_PARAM ((pv_payload != NULL) && (pu8_packet != NULL) && (pu16_packet_len != NULL));
    if (u16_payload_len > MDL_MAX_PKT_LEN - sizeof (MDL_pkt_t))
    {
        LOGE ("Invalid message length %d", u16_payload_len);
        return MDL_ERR;
    }

    /* Construct header of the data-link packet to send */
    pstru_pkt->au8_sof [0]  = MDL_SOF_1;
    pstru_pkt->au8_sof [1]  = MDL_SOF_2;
    pstru_pkt->au8_sof [2]  = MDL_SOF_3;
    pstru_pkt->au8_sof [3]  = MDL_SOF_4;
    pstru_pkt->u8_type      = 0;
    pstru_pkt->u8_len       = (uint8_t)(sizeof (MDL_pkt_t) + u16_payload_len);
    pstru_pkt->u16_cks      = 0;

    /* Calculate sum of packet header bytes */
    for (uint16_t u16_idx = 0; u16_idx < sizeof (MDL_pkt_t); u16_idx++)
    {
        u16_checksum += pu8_packet[u16_idx];
    }

    /* Calculate checksum, copy payload data, and add stuff bytes if needed */
    for (uint16_t u16_idx = 0; u16_idx < u16_payload_len; u16_idx++)
    {
        u16_checksum += pu8_payload[u16_idx];
        pstru_pkt->au8_payload[u16_pkt_len++] = pu8_payload[u16_idx];
        if ((u16_idx >= 3) &&
            (pu8_payload[u16_idx - 3] == MDL_SOF_1) &&
            (pu8_payload[u16_idx - 2] == MDL_SOF_2) &&
            (pu8_payload[u16_idx - 1] == MDL_SOF_3) &&
            (pu8_payload[u16_idx - 0] == MDL_SOF_4))
        {
            pstru_pkt->au8_payload[u16_pkt_len++] = MDL_SOF_STUFF;
        }
    }
    pstru_pkt->u16_cks = ~u16_checksum;
    u16_pkt_len += sizeof (MDL_pkt_t);
    ASSERT_PARAM (u16_pkt_len <= *pu16_packet_len)

    /* Done */
    *pu16_packet_len = u16_pkt_len;
    return MDL_OK;
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
static bool b_MDL_Is_Valid_Inst (MDL_inst_t x_inst)
{
    if (x_inst == &g_stru_mdl_obj)
    {
        return true;
    }

    LOGE ("Invalid instance");
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
