/*
 * (C) Copyright 2020
 * Zimplistic Private Limited
 * Sicris Rey Embay, sicris@zimplistic.com
 */

/* ----------------------- System includes ----------------------------------*/

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"
#if defined(CONFIG_MODBUS_ZPL_MASTER)
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_m.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbconfig.h"
#include "mbzpl_req02_m.h"
#include "esp_log.h"
#include "string.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_ZPL_REQ02            (0x02)
#define MB_ZPL_REQ02_LEN        (1)

static const char * TAG = "mbzpl_req02";

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
eMBMasterReqErrCode mbzpl_MasterSendReq02(UCHAR ucSndAddr, LONG lTimeOut)
{
    UCHAR *ucMBFrame;
    BOOL ret;
    eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;
    if( ucSndAddr > MB_MASTER_TOTAL_SLAVE_NUM) {
        eErrStatus = MB_MRE_ILL_ARG;
        ESP_LOGE(TAG, "mbzpl_MasterSendReq02: Invalid slave address 0x%02X", ucSndAddr);
        return eErrStatus;
    }
    if( xMBMasterRunResTake(lTimeOut) == FALSE) {
        eErrStatus = MB_MRE_MASTER_BUSY;
        ESP_LOGE(TAG, "mbzpl_MasterSendReq02: xMBMasterRunResTake() failed.");
        return eErrStatus;
    }

    vMBMasterGetPDUSndBuf(&ucMBFrame);
    vMBMasterSetDestAddress(ucSndAddr);
    ucMBFrame[0] = MB_ZPL_REQ02;
    vMBMasterSetPDUSndLength(MB_ZPL_REQ02_LEN);
    ret = xMBMasterPortEventPost( EV_MASTER_FRAME_TRANSMIT );
    if(TRUE != ret) {
        ESP_LOGE(TAG, "mbzpl_MasterSendReq02: xMBMasterPortEventPost Failed.");
    }
    eErrStatus = eMBMasterWaitRequestFinish( );
    if(MB_MRE_NO_ERR != eErrStatus) {
        ESP_LOGE(TAG, "mbzpl_MasterSendReq02: eMBMasterWaitRequestFinish() returned 0x%02X", eErrStatus);
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

#endif /* #if defined(CONFIG_MODBUS_ZPL_MASTER) */
