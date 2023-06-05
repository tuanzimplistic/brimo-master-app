/*
 * (C) Copyright 2021
 * Zimplistic Private Limited
 * Toan Dang, toan.dang@zimplistic.com
 */

/* ----------------------- System includes ----------------------------------*/

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"
#if defined(CONFIG_MODBUS_ZPL_MASTER)
/* ----------------------- Modbus includes ----------------------------------*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mb_m.h"
#include "mbconfig.h"
#include "esp_log.h"
#include "mbzpl_req_m.h"
#include "srvc_micropy.h"

/*! \ingroup mbzpl_req_m
 * \brief Maximum length of Modbus responded buffer.
 */
#define MAL_GET_STATE_RES_MAX_LENGTH        (48)

/*! \ingroup mbzpl_req_m
 * \brief Fuction code offset in Modbus frame.
 */
#define REQ_MAL_CODE_OFFSET                 (0x00)

/*! \ingroup mbzpl_req_m
 * \brief Subcode offset in Modbus frame.
 */
#define REQ_MAL_SUBCODE_OFFSET              (0x01)

#define MB_ZPL_REQ01                        (0x01)
#define MB_ZPL_REQ02                        (0x02)
#define MB_ZPL_REQ15                        (0x15)
#define MB_ZPL_REQ16                        (0x16)
#define MB_ZPL_REQ17                        (0x17)

/*! \ingroup mbzpl_req_m
 * \brief Heater Function code
 *
 */
#define MB_ZPL_REQ20                        (0x20)

/*! \ingroup mbzpl_req_m
 * \brief Dispenser Function code
 *
 */
#define MB_ZPL_REQ21                        (0x21)

/*! \ingroup mbzpl_req_m
 * \brief VT Function code
 *
 */
#define MB_ZPL_REQ22                        (0x22)

/*! \ingroup mbzpl_req_m
 * \brief WP Function code
 *
 */
#define MB_ZPL_REQ23                        (0x23)

/*! \ingroup mbzpl_req_m
 * \brief KN Function code
 *
 */
#define MB_ZPL_REQ24                        (0x24)

/*! \ingroup mbzpl_req_m
 * \brief KR Function code
 *
 */
#define MB_ZPL_REQ25                        (0x25)

/*! \ingroup mbzpl_req_m
 * \brief GPIO Function code
 *
 */
#define MB_ZPL_REQ2F                        (0x2F)

/** @brief  Maximum size in bytes of the message received from C environment */
#define MP_MAX_C_MSG_LEN                    128

/* ----------------------- Static variables ------------------------------------------*/
static const char * TAG = "mbzpl_req";
static TaskHandle_t taskHandle;
static bool isInit = false;

/* ----------------------- Static functions ------------------------------------------*/
eMBException eMBZplRequest01( UCHAR * pucFrame, USHORT * usLen );
eMBException eMBZplRequest02( UCHAR * pucFrame, USHORT * usLen );
eMBException eMBZplRequest15( UCHAR * pucFrame, USHORT * usLen );
eMBException eMBZplRequest16( UCHAR * pucFrame, USHORT * usLen );
eMBException eMBZplRequest17( UCHAR * pucFrame, USHORT * usLen );
eMBException eMBZplRequest20( UCHAR * pucFrame, USHORT * usLen );
eMBException eMBZplRequest21( UCHAR * pucFrame, USHORT * usLen );
eMBException eMBZplRequest22( UCHAR * pucFrame, USHORT * usLen );
eMBException eMBZplRequest23( UCHAR * pucFrame, USHORT * usLen );
eMBException eMBZplRequest24( UCHAR * pucFrame, USHORT * usLen );
eMBException eMBZplRequest25( UCHAR * pucFrame, USHORT * usLen );
eMBException eMBZplRequest2F( UCHAR * pucFrame, USHORT * usLen );
BOOL mbzpl_register_all(void);

static xMBFunctionHandler const MB_ZPL_FUNC_TABLE[] = {
        { MB_ZPL_REQ01, eMBZplRequest01},
        { MB_ZPL_REQ02, eMBZplRequest02},
        { MB_ZPL_REQ15, eMBZplRequest15},
        { MB_ZPL_REQ16, eMBZplRequest16},
        { MB_ZPL_REQ17, eMBZplRequest17},
        { MB_ZPL_REQ20, eMBZplRequest20},
        { MB_ZPL_REQ21, eMBZplRequest21},
        { MB_ZPL_REQ22, eMBZplRequest22},
        { MB_ZPL_REQ23, eMBZplRequest23},
        { MB_ZPL_REQ24, eMBZplRequest24},
        { MB_ZPL_REQ25, eMBZplRequest25},
        { MB_ZPL_REQ2F, eMBZplRequest2F}
};
/* ----------------------- Start implementation -----------------------------*/
eMBMasterReqErrCode mbzpl_MasterSendReq(UCHAR ucSndAddr, LONG lTimeOut, USHORT usLength, UCHAR *ucBufPtr)
{
    UCHAR *ucMBFrame;
    BOOL ret;
    eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;
    if( ucSndAddr > MB_MASTER_TOTAL_SLAVE_NUM) {
        eErrStatus = MB_MRE_ILL_ARG;
        ESP_LOGE(TAG, "mbzpl_MasterSendReq: Invalid slave address 0x%02X", ucSndAddr);
        return eErrStatus;
    }
    if( xMBMasterRunResTake(lTimeOut) == FALSE) {
        eErrStatus = MB_MRE_MASTER_BUSY;
        ESP_LOGE(TAG, "mbzpl_MasterSendReq: xMBMasterRunResTake() failed.");
        return eErrStatus;
    }

    vMBMasterGetPDUSndBuf(&ucMBFrame);
    vMBMasterSetDestAddress(ucSndAddr);
    vMBMasterSetPDUSndLength(usLength);
    /*Copy buffer data*/
    memcpy(ucMBFrame, ucBufPtr, usLength);
    ret = xMBMasterPortEventPost( EV_MASTER_FRAME_TRANSMIT );
    if(TRUE != ret) {
        ESP_LOGE(TAG, "mbzpl_MasterSendReq: xMBMasterPortEventPost Failed.");
    }
    eErrStatus = eMBMasterWaitRequestFinish( );
    if(MB_MRE_NO_ERR != eErrStatus) {
        ESP_LOGE(TAG, "mbzpl_MasterSendReq: eMBMasterWaitRequestFinish() returned 0x%02X", eErrStatus);
    }

    switch(eErrStatus) {
        case MB_MRE_NO_ERR:
            break;

        case MB_MRE_NO_REG:
            ESP_LOGE(TAG, "Invalid register request");
            break;

        case MB_MRE_TIMEDOUT:
            ESP_LOGE(TAG, "Slave did not send response");
            break;

        case MB_MRE_EXE_FUN:
        case MB_MRE_REV_DATA:
            ESP_LOGE(TAG, "Invalid response from slave");
            break;

        case MB_MRE_MASTER_BUSY:
            ESP_LOGE(TAG, "Master is busy (previous request is pending)");
            break;

        default:
            ESP_LOGE(TAG, "Incorrect return code (%x) ", eErrStatus);
            break;
    }
    return (eErrStatus);
}

static void _MAL_ReqTask(void * parameters)
{
    eMBMasterReqErrCode mb_ret ;
    static uint8_t au8_msg[MP_MAX_C_MSG_LEN];

    while (1)
    {
        /* Wait until a message from MicroPython arrives */
        uint16_t u16_len = sizeof(au8_msg);
        if ((s8_MP_Que_Receive_From_MP (au8_msg, &u16_len) == MP_OK) && (u16_len != 0))
        {
            mb_ret = mbzpl_MasterSendReq (SLAVE_ADDR, 100, u16_len, au8_msg);
            if (mb_ret != MB_MRE_NO_ERR)
            {
                ESP_LOGE(TAG, "mbzpl_MasterSendReq err %d %d", u16_len, au8_msg[0]);
            }
        }
    }
}

BOOL mbzpl_register_all(void)
{
    uint32_t tableEntryCnt = sizeof(MB_ZPL_FUNC_TABLE) / sizeof(MB_ZPL_FUNC_TABLE[0]);
    uint32_t i = 0;
    eMBErrorCode xErr;

    for (i = 0; i < tableEntryCnt; i++) {
#if defined(CONFIG_MODBUS_ZPL_MASTER)
        xErr = eMBMasterRegisterCB(MB_ZPL_FUNC_TABLE[i].ucFunctionCode, MB_ZPL_FUNC_TABLE[i].pxHandler);
        if(MB_ENOERR != xErr) {
            ESP_LOGE(TAG, "mbzpl_register_all: eMBMasterRegisterCB() error 0x%02X", xErr);
            return FALSE;
        }
#elif defined(CONFIG_MODBUS_ZPL_SLAVE)
        xErr = eMBRegisterCB(MB_ZPL_FUNC_TABLE[i].ucFunctionCode, MB_ZPL_FUNC_TABLE[i].pxHandler);
        if(MB_ENOERR != xErr) {
            return FALSE;
        }
#else
#error "Invalid Modbus configuration"
#endif
    }

    return TRUE;
}

uint8_t MAL_REQ_init(void)
{
    eMBErrorCode status = MB_ENOERR;
    if (isInit == true) {
        return ESP_FAIL; /*Do nothing*/
    }
    isInit = true;

    if(TRUE != mbzpl_register_all()) {
        ESP_LOGE(TAG, "mbzpl_register_all error");
        return ESP_FAIL;
    }
    status = eMBMasterInit(MB_ZPL, (UCHAR)CONFIG_MB_UART_PORT_NUM,
                                (ULONG)CONFIG_MB_UART_BAUD_RATE, MB_PAR_NONE);
    if(MB_ENOERR != status) {
        ESP_LOGE(TAG, "eMBMasterInit error 0x%02X", status);
        return ESP_FAIL;
    }
    status = eMBMasterEnable();
    if(MB_ENOERR != status) {
        ESP_LOGE(TAG, "eMBMasterEnable error 0x%02X", status);
        return ESP_FAIL;
    }
    xMBMasterPortEnable(TRUE);

    /* Create Worker Task */
#if (CONFIG_MAL_MB_TASK_CORE == 0)
    if(pdPASS != xTaskCreatePinnedToCore(
                        _MAL_ReqTask,
                        TAG,
                        CONFIG_MAL_MB_TASK_STACK,
                        (void *)0,
                        CONFIG_MAL_MB_TASK_PRIO,
                        &taskHandle,
                        PRO_CPU_NUM)) {
        ESP_LOGE(TAG, "Failed to create worker Task.");
    }
#elif (CONFIG_MAL_MB_TASK_CORE == 1)
    if(pdPASS != xTaskCreatePinnedToCore(
                        _MAL_ReqTask,
                        TAG,
                        CONFIG_MAL_MB_TASK_STACK,
                        (void *)0,
                        CONFIG_MAL_MB_TASK_PRIO,
                        &taskHandle,
                        APP_CPU_NUM)) {
        ESP_LOGE(TAG, "Failed to create worker Task.");
    }
#else
    if(pdPASS != xTaskCreatePinnedToCore(
                            _MAL_ReqTask,
                            TAG,
                            CONFIG_MAL_MB_TASK_STACK,
                            (void *)0,
                            CONFIG_MAL_MB_TASK_PRIO,
                            &taskHandle,
                            tskNO_AFFINITY)) {
            ESP_LOGE(TAG, "Failed to create worker Task.");
        }
#endif
    return ESP_OK;
}

eMBException eMBZplRequest( UCHAR * pucFrame, USHORT * usLen )
{
    eMBException    eStatus = MB_EX_NONE;

    uint8_t reqcode = pucFrame[REQ_MAL_CODE_OFFSET];
    uint8_t subcode = pucFrame[REQ_MAL_SUBCODE_OFFSET];
    
    (void)reqcode;
    (void)subcode;

    s8_MP_Que_Send_To_MP(pucFrame, *usLen);

    return eStatus;
}

eMBException eMBZplRequest01( UCHAR * pucFrame, USHORT * usLen )
{
    eMBZplProcessRequest01(pucFrame, usLen);
    return eMBZplRequest(pucFrame, usLen);
}

eMBException eMBZplRequest02( UCHAR * pucFrame, USHORT * usLen )
{
    /* Do nothing */
    return MB_EX_NONE;
}

eMBException eMBZplRequest15( UCHAR * pucFrame, USHORT * usLen )
{
    return eMBZplRequest(pucFrame, usLen);
}

eMBException eMBZplRequest16( UCHAR * pucFrame, USHORT * usLen )
{
    return eMBZplRequest(pucFrame, usLen);
}

eMBException eMBZplRequest17( UCHAR * pucFrame, USHORT * usLen )
{
    return eMBZplRequest(pucFrame, usLen);
}

eMBException eMBZplRequest20( UCHAR * pucFrame, USHORT * usLen )
{
    return eMBZplRequest(pucFrame, usLen);
}

eMBException eMBZplRequest21( UCHAR * pucFrame, USHORT * usLen )
{
    return eMBZplRequest(pucFrame, usLen);
}

eMBException eMBZplRequest22( UCHAR * pucFrame, USHORT * usLen )
{
    return eMBZplRequest(pucFrame, usLen);
}

eMBException eMBZplRequest23( UCHAR * pucFrame, USHORT * usLen )
{
    return eMBZplRequest(pucFrame, usLen);
}

eMBException eMBZplRequest24( UCHAR * pucFrame, USHORT * usLen )
{
    return eMBZplRequest(pucFrame, usLen);
}

eMBException eMBZplRequest25( UCHAR * pucFrame, USHORT * usLen )
{
    return eMBZplRequest(pucFrame, usLen);
}

eMBException eMBZplRequest2F( UCHAR * pucFrame, USHORT * usLen )
{
    return eMBZplRequest(pucFrame, usLen);
}

#endif /* #if defined(CONFIG_MODBUS_ZPL_MASTER) */
