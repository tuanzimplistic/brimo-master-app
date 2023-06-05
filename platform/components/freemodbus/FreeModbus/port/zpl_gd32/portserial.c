/*
 * (C) Copyright 2020
 * Zimplistic Private Limited
 * Sicris Rey Embay, sicris@zimplistic.com
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"
#include "BSP_uart.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbzpl.h"

/* ----------------------- Static variables ---------------------------------*/
static StaticTask_t xMBWorkerTCB;
static StackType_t xMBWorkerStackSto[CONFIG_MODBUS_WORKER_STACK_SIZE];
static TaskHandle_t xMBWorkerTaskHandle = NULL;

volatile UCHAR ucRxFrame[CONFIG_MODBUS_ZPL_SLAVE_MAX_FRAME_SIZE];

/* ----------------------- Defines ------------------------------------------*/
#if defined(CONFIG_MODBUS_ZPL_UART_ONE)
#define MB_SERIAL   BSP_UART_ONE
#elif defined(CONFIG_MODBUS_ZPL_UART_TWO)
#define MB_SERIAL   BSP_UART_TWO
#endif
/* ----------------------- static functions ---------------------------------*/
static void _MBWorkerTask(void * pArg)
{
    uint32_t u32NotificationValue;
    uint32_t len = 0;
    (void)pArg;

    while(!bsp_uart_ready(MB_SERIAL)) {
        vTaskDelay(1);
    }

    /* Register for Task Notification */
    configASSERT(BSP_UART_OK == bsp_uart_register_receive_notify(
            MB_SERIAL, xMBWorkerTaskHandle));

    while(1) {
        if(pdPASS == xTaskNotifyWait(pdFALSE, 0xFFFFFFFF,
                &u32NotificationValue, portMAX_DELAY)) {
            if(u32NotificationValue & BSP_UART_RX_FRAME_FLAG) {
                /* Full Frame Received */
                len = CONFIG_MODBUS_ZPL_SLAVE_MAX_FRAME_SIZE;
                configASSERT(BSP_UART_OK == bsp_uart_receive(MB_SERIAL, (uint8_t *)ucRxFrame, &len));
                if(len > 0) {
                    xMBZPLStoreRxFrame(ucRxFrame, len);
                    xMBPortEventPost(EV_FRAME_RECEIVED);
                    /* T3.5 is already integrated with Rx RTO */
                    pxMBPortCBTimerExpired();
                }
            }
            if(u32NotificationValue & BSP_UART_TX_DONE_FLAG) {
                /* Done Transmission */
                xMBPortEventPost(EV_FRAME_SENT);
            }
        }
    }
}

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits,
        eMBParity eParity)
{
    (void)ucPORT;
    (void)ulBaudRate;
    (void)ucDataBits;
    (void)eParity;
    if(BSP_UART_OK != bsp_uart_init()) {
        return FALSE;
    }

    /* Create worker task */
    if(NULL == xMBWorkerTaskHandle) {
        xMBWorkerTaskHandle = xTaskCreateStatic(
                _MBWorkerTask,
                "Modbus",
                CONFIG_MODBUS_WORKER_STACK_SIZE,
                (void *)0,
                CONFIG_MODBUS_WORKER_TASK_PRIORITY,
                xMBWorkerStackSto,
                &xMBWorkerTCB
                );
    }

    return TRUE;
}

void vMBPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
    /// TODO
}

void vMBPortClose(void)
{
    /// TODO
}

BOOL xMBPortSerialPutByte(CHAR ucByte)
{
    /// TODO
    return TRUE;
}

BOOL xMBPortSerialPut( CHAR * pucBuf, USHORT usLen)
{
    if((CHAR *)0 == pucBuf) {
        return FALSE;
    }

    if(usLen == 0) {
        /* Nothing to send */
        return TRUE;
    }

    if(BSP_UART_OK != bsp_uart_send(MB_SERIAL, (uint8_t *)pucBuf, usLen)) {
        return FALSE;
    }

    return TRUE;
}


BOOL xMBPortSerialGetByte(CHAR * pucByte)
{
    /// TODO
    return TRUE;
}
