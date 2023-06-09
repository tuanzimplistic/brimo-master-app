/* Copyright 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * FreeModbus Libary: ESP32 Port Demo Application
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portserial.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_m.h"
#include "mbport.h"
#include "mbzpl.h"
#include "mbconfig.h"

#include <string.h>
#include "driver/uart.h"
#include "soc/dport_access.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "sdkconfig.h"

/* ----------------------- Static variables ---------------------------------*/
static const CHAR *TAG = "MB_MASTER_SERIAL";

static EventGroupHandle_t mbm_event_group;
static TaskHandle_t mbm_task_handle;
#define MB_EVENT_STACK_STARTED 0x00000001

// A queue to handle UART event.
static QueueHandle_t xMbUartQueue;
static TaskHandle_t xMbTaskHandle;

// The UART hardware port number
static UCHAR ucUartNumber = UART_NUM_MAX - 1;

static BOOL bRxStateEnabled = FALSE; // Receiver enabled flag
static BOOL bTxStateEnabled = FALSE; // Transmitter enabled flag
static volatile UCHAR ucMasterRcvBufTmp[MB_SERIAL_BUF_SIZE];
static volatile USHORT ucMasterRcvBufTmpIdx = 0;
static volatile USHORT ucMasterRcvBufTmpCnt = 0;

void vMBMasterPortSerialEnable(BOOL bRxEnable, BOOL bTxEnable)
{
    // This function can be called from xMBRTUTransmitFSM() of different task
    if (bTxEnable)
    {
        bTxStateEnabled = TRUE;
    }
    else
    {
        bTxStateEnabled = FALSE;
    }
    if (bRxEnable)
    {
        bRxStateEnabled = TRUE;
        vTaskResume(xMbTaskHandle); // Resume receiver task
    }
    else
    {
        vTaskSuspend(xMbTaskHandle); // Block receiver task
        bRxStateEnabled = FALSE;
    }
}

static BOOL xMBMasterPortSerialGetByteFake(CHAR *pucByte)
{
    assert(pucByte != NULL);
    USHORT usLength = uart_read_bytes(ucUartNumber, (uint8_t *)pucByte, 1, MB_SERIAL_RX_TOUT_TICKS);
    return (usLength == 1);
}

static USHORT usMBMasterPortSerialRxPoll(size_t xEventSize)
{
    BOOL xReadStatus = TRUE;
    USHORT usCnt = 0;
    CHAR pucByte;

    ucMasterRcvBufTmpIdx = 0;
    ucMasterRcvBufTmpCnt = 0;

    if (bRxStateEnabled)
    {
        while (xReadStatus && (usCnt++ <= MB_SERIAL_BUF_SIZE))
        {
            // Call the Modbus stack callback function and let it fill the stack buffers.
            xReadStatus = xMBMasterPortSerialGetByteFake(&pucByte);
            ucMasterRcvBufTmp[ucMasterRcvBufTmpIdx] = pucByte;
            ucMasterRcvBufTmpIdx++;
        }
        // The buffer is transferred into Modbus stack and is not needed here any more
        uart_flush_input(ucUartNumber);
        if (ucMasterRcvBufTmp[1] <= 0x7F)
        {
            for (int i = 0; i < ucMasterRcvBufTmpIdx; i++)
            {
                pxMBMasterFrameCBByteReceived();
            }
        }
        else
        {
            for (int i = 0; i < ucMasterRcvBufTmpIdx; i++)
            {
                ESP_LOGE("tuan", "data[%d] = %d", i, ucMasterRcvBufTmp[i]);
            }
        }
        ESP_LOGD(TAG, "Received data: %d(bytes in buffer)\n", (uint32_t)usCnt);
    }
    else
    {
        ESP_LOGE(TAG, "%s: bRxState disabled but junk data (%d bytes) received. ", __func__, xEventSize);
    }
    return usCnt;
}

BOOL xMBMasterPortSerialTxPoll(void)
{
    USHORT usCount = 0;
    BOOL bNeedPoll = TRUE;

    if (bTxStateEnabled)
    {
        // Continue while all response bytes put in buffer or out of buffer
        while (bNeedPoll && (usCount++ < MB_SERIAL_BUF_SIZE))
        {
            // Calls the modbus stack callback function to let it fill the UART transmit buffer.
            bNeedPoll = pxMBMasterFrameCBTransmitterEmpty(); // callback to transmit FSM
        }
        ESP_LOGD(TAG, "MB_TX_buffer sent: (%d) bytes.", (uint16_t)(usCount - 1));
        // Waits while UART sending the packet
        esp_err_t xTxStatus = uart_wait_tx_done(ucUartNumber, MB_SERIAL_TX_TOUT_TICKS);
        vMBMasterPortSerialEnable(TRUE, FALSE);
        MB_PORT_CHECK((xTxStatus == ESP_OK), FALSE, "mb serial sent buffer failure.");
        return TRUE;
    }
    return FALSE;
}

BOOL xMBMasterPortEnable(BOOL enable)
{
    MB_PORT_CHECK((mbm_event_group != NULL), FALSE, "mbm_event_group not initialized.");
    if (enable)
    {
        xEventGroupSetBits(mbm_event_group, (EventBits_t)MB_EVENT_STACK_STARTED);
    }
    else
    {
        xEventGroupClearBits(mbm_event_group, (EventBits_t)MB_EVENT_STACK_STARTED);
    }
    return TRUE;
}
// Modbus event processing task
static void vMasterTask(void *pvParameters)
{
    // Main Modbus stack processing cycle
    for (;;)
    {
        // Wait for poll events
        BaseType_t status = xEventGroupWaitBits(mbm_event_group,
                                                (BaseType_t)(MB_EVENT_STACK_STARTED),
                                                pdFALSE, // do not clear bits
                                                pdFALSE,
                                                portMAX_DELAY);
        // Check if stack started then poll for data
        if (status & MB_EVENT_STACK_STARTED)
        {
            (void)eMBMasterPoll(); // Allow stack to process data
            // Send response buffer if ready to be sent
            BOOL xSentState = xMBMasterPortSerialTxPoll();
            if (xSentState)
            {
                // Let state machine know that response was transmitted out
                (void)xMBMasterPortEventPost(EV_MASTER_FRAME_SENT);
            }
        }
    }
}

// UART receive event task
static void vUartTask(void *pvParameters)
{
    uart_event_t xEvent;
    USHORT usResult = 0;
    for (;;)
    {
        if (xQueueReceive(xMbUartQueue, (void *)&xEvent, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGD(TAG, "MB_uart[%d] event:", ucUartNumber);
            switch (xEvent.type)
            {
            // Event of UART receiving data
            case UART_DATA:
                ESP_LOGD(TAG, "Data event, len: %d.", xEvent.size);
                // This flag set in the event means that no more
                // data received during configured timeout and UART TOUT feature is triggered
#if defined(CONFIG_MODBUS_ZPL_IDF_V4_2)
                if (xEvent.timeout_flag)
                {
                    // Read received data and send it to modbus stack
                    usResult = usMBMasterPortSerialRxPoll(xEvent.size);
                    ESP_LOGD(TAG, "Timeout occured, processed: %d bytes", usResult);
                }
#elif defined(CONFIG_MODBUS_ZPL_IDF_V4_0)
                usResult = usMBMasterPortSerialRxPoll(xEvent.size);
                ESP_LOGD(TAG, "Timeout occured, processed: %d bytes", usResult);
#else
#error "Invalid IDF version for modbus master"
#endif /* #if defined(CONFIG_MODBUS_ZPL_IDF_V4_2) */
                break;
            // Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGD(TAG, "hw fifo overflow.");
                xQueueReset(xMbUartQueue);
                break;
            // Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGD(TAG, "ring buffer full.");
                xQueueReset(xMbUartQueue);
                uart_flush_input(ucUartNumber);
                break;
            // Event of UART RX break detected
            case UART_BREAK:
                ESP_LOGD(TAG, "uart rx break.");
                break;
            // Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGD(TAG, "uart parity error.");
                break;
            // Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGD(TAG, "uart frame error.");
                break;
            default:
                ESP_LOGD(TAG, "uart event type: %d.", xEvent.type);
                break;
            }
        }
    }
    vTaskDelete(NULL);
}

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity)
{
    esp_err_t xErr = ESP_OK;
    BaseType_t xStatus;
    MB_PORT_CHECK((eParity <= MB_PAR_EVEN), FALSE, "mb serial set parity failure.");
    // Set communication port number
    ucUartNumber = ucPORT;
    // Configure serial communication parameters
    UCHAR ucParity = UART_PARITY_DISABLE;
    UCHAR ucData = UART_DATA_8_BITS;
    switch (eParity)
    {
    case MB_PAR_NONE:
        ucParity = UART_PARITY_DISABLE;
        break;
    case MB_PAR_ODD:
        ucParity = UART_PARITY_ODD;
        break;
    case MB_PAR_EVEN:
        ucParity = UART_PARITY_EVEN;
        break;
    }
    switch (ucDataBits)
    {
    case 5:
        ucData = UART_DATA_5_BITS;
        break;
    case 6:
        ucData = UART_DATA_6_BITS;
        break;
    case 7:
        ucData = UART_DATA_7_BITS;
        break;
    case 8:
        ucData = UART_DATA_8_BITS;
        break;
    default:
        ucData = UART_DATA_8_BITS;
        break;
    }
    uart_config_t xUartConfig = {
        .baud_rate = ulBaudRate,
        .data_bits = ucData,
        .parity = ucParity,
        .stop_bits = UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 2,
#if defined(CONFIG_MODBUS_ZPL_IDF_V4_2)
        .source_clk = UART_SCLK_APB,
#endif /* #if defined(CONFIG_MODBUS_ZPL_IDF_V4_2) */
    };
    mbm_event_group = xEventGroupCreate();
    MB_PORT_CHECK((mbm_event_group != NULL),
                  FALSE, "mb config failure, xEventGroupCreate() failed.");
    xStatus = xTaskCreate((void *)&vMasterTask,
                          "mb master task",
                          CONFIG_FMB_CONTROLLER_STACK_SIZE,
                          NULL, // No parameters
                          (CONFIG_FMB_SERIAL_TASK_PRIO - 1),
                          &mbm_task_handle);
    if (xStatus != pdPASS)
    {
        vTaskDelete(mbm_task_handle);
        // Force exit from function with failure
        MB_PORT_CHECK(FALSE, FALSE,
                      "mb stack master task creation error. xTaskCreate() returned (0x%x).",
                      xStatus);
    }
    // Set UART Pins
#ifdef CONFIG_MB_UART_PHY_MODE_RS485
    xErr = uart_set_pin(CONFIG_MB_UART_PORT_NUM, CONFIG_MB_UART_TXD, CONFIG_MB_UART_RXD,
                        CONFIG_MB_UART_RTS, UART_PIN_NO_CHANGE);

#elif CONFIG_MB_UART_PHY_MODE_RS232
    xErr = uart_set_pin(CONFIG_MB_UART_PORT_NUM, CONFIG_MB_UART_TXD, CONFIG_MB_UART_RXD,
                        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

#else
#error "Wrong UART physical layer"
#endif

    MB_PORT_CHECK((xErr == ESP_OK),
                  FALSE, "mb config failure, uart_set_pin() return (0x%x).", xErr);
    // Set UART config
    xErr = uart_param_config(ucUartNumber, &xUartConfig);
    MB_PORT_CHECK((xErr == ESP_OK),
                  FALSE, "mb config failure, uart_param_config() returned (0x%x).", xErr);
    // Install UART driver, and get the queue.
    xErr = uart_driver_install(ucUartNumber, MB_SERIAL_BUF_SIZE, MB_SERIAL_BUF_SIZE,
                               MB_QUEUE_LENGTH, &xMbUartQueue, MB_PORT_SERIAL_ISR_FLAG);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "mb serial driver failure, uart_driver_install() returned (0x%x).", xErr);
    // Set timeout for TOUT interrupt (T3.5 modbus time)
    xErr = uart_set_rx_timeout(ucUartNumber, MB_SERIAL_TOUT);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "mb serial set rx timeout failure, uart_set_rx_timeout() returned (0x%x).", xErr);

    // Set always timeout flag to trigger timeout interrupt even after rx fifo full
#if defined(CONFIG_MODBUS_ZPL_IDF_V4_2)
    uart_set_always_rx_timeout(ucUartNumber, true);
#elif defined(CONFIG_MODBUS_ZPL_IDF_V4_0)
    /// TODO
#endif

#ifdef CONFIG_MB_UART_PHY_MODE_RS485
    xErr = uart_set_mode(CONFIG_MB_UART_PORT_NUM,
                         UART_MODE_RS485_HALF_DUPLEX);
#elif CONFIG_MB_UART_PHY_MODE_RS232
    xErr = uart_set_mode(CONFIG_MB_UART_PORT_NUM,
                         UART_MODE_UART);
#else
#error "Wrong UART physical layer"
#endif

    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "mb serial driver failure, uart_set_mode() returned (0x%x).", xErr);

    // Create a task to handle UART events
    xStatus = xTaskCreate(vUartTask, "uart_queue_task", MB_SERIAL_TASK_STACK_SIZE,
                          NULL, MB_SERIAL_TASK_PRIO, &xMbTaskHandle);
    if (xStatus != pdPASS)
    {
        vTaskDelete(xMbTaskHandle);
        // Force exit from function with failure
        MB_PORT_CHECK(FALSE, FALSE,
                      "mb stack serial task creation error. xTaskCreate() returned (0x%x).",
                      xStatus);
    }
    else
    {
        vTaskSuspend(xMbTaskHandle); // Suspend serial task while stack is not started
    }
    ESP_LOGD(MB_PORT_TAG, "%s Init serial.", __func__);

    return TRUE;
}

void vMBMasterPortSerialClose(void)
{
    (void)vTaskDelete(xMbTaskHandle);
    ESP_ERROR_CHECK(uart_driver_delete(ucUartNumber));
}

BOOL xMBMasterPortSerialPutByte(CHAR ucByte)
{
    // Send one byte to UART transmission buffer
    // This function is called by Modbus stack
    UCHAR ucLength = uart_write_bytes(ucUartNumber, &ucByte, 1);
    return (ucLength == 1);
}

// Get one byte from intermediate RX buffer
BOOL xMBMasterPortSerialGetByte(CHAR *pucByte)
{
    assert(pucByte != NULL);
    USHORT usLength = 0;
    if (ucMasterRcvBufTmpCnt < ucMasterRcvBufTmpIdx - 1)
    {
        *pucByte = ucMasterRcvBufTmp[ucMasterRcvBufTmpCnt];
        ucMasterRcvBufTmpCnt++;
        usLength = 1;
    }
    return (usLength == 1);
}