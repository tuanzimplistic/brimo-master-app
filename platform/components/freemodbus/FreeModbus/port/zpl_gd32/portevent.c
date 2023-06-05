/*
 * (C) Copyright 2020
 * Zimplistic Private Limited
 * Sicris Rey Embay, sicris@zimplistic.com
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "port.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "mb.h"
#include "mbport.h"

/* ----------------------- Variables ----------------------------------------*/
static StaticTask_t xMBEventHdlrTCB;
static StackType_t xMBEventHdlrStackSto[CONFIG_MODBUS_HANDLER_STACK_SIZE];
static TaskHandle_t xMBEventTaskHandle = NULL;

static StaticQueue_t xQueueMBEvent;
#define QUEUE_LENGTH    5
#define ITEM_SIZE       sizeof( eMBEventType )
uint8_t ucQueueStorageArea[ QUEUE_LENGTH * ITEM_SIZE ];
QueueHandle_t xMBEventQueue = NULL;

static void _MBEventTask(void * pArg)
{
    (void)pArg;
    while(1) {
        eMBPoll();
    }
}

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortEventInit( void )
{
    /* Create Event Task Handler */
    if(NULL == xMBEventTaskHandle) {
        xMBEventTaskHandle = xTaskCreateStatic(
                _MBEventTask,
                "ModbusEvt",
                CONFIG_MODBUS_HANDLER_STACK_SIZE,
                (void *)0,
                CONFIG_MODBUS_HANDLER_TASK_PRIORITY,
                xMBEventHdlrStackSto,
                &xMBEventHdlrTCB
                );
    }

    if(NULL == xMBEventQueue) {
        xMBEventQueue = xQueueCreateStatic( QUEUE_LENGTH,
                                     ITEM_SIZE,
                                     ucQueueStorageArea,
                                     &xQueueMBEvent );
    }

    return TRUE;
}

BOOL
xMBPortEventPost( eMBEventType eEvent )
{
    if(pdTRUE == xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken  = pdFALSE;
        xQueueSendFromISR(xMBEventQueue, &eEvent, &xHigherPriorityTaskWoken);
        if(xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        if(pdTRUE != xQueueSend(xMBEventQueue, &eEvent, 2)) {
            return FALSE;
        }

    }
    return TRUE;
}

BOOL
xMBPortEventGet( eMBEventType * eEvent )
{
    eMBEventType event;
    if(pdPASS == xQueueReceive(xMBEventQueue, &(event), portMAX_DELAY)) {
        switch(event) {
            case EV_READY: {
                *eEvent = EV_READY;
                break;
            }
            case EV_FRAME_RECEIVED: {
                *eEvent = EV_FRAME_RECEIVED;
                break;
            }
            case EV_EXECUTE: {
                *eEvent = EV_EXECUTE;
                break;
            }
            case EV_FRAME_SENT: {
                *eEvent = EV_FRAME_SENT;
                break;
            }
            default:
                break;
        }
    }
    return TRUE;
}
