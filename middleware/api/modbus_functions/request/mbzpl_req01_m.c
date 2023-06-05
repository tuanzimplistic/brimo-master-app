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
#include "mbzpl_req01_m.h"
#include "esp_log.h"
#include "string.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_ZPL_REQ01            (0x01)
#define MB_ZPL_REQ01_LEN        (1)

static const char * TAG = "mbzpl_req01";

static uint8_t SlaveContext;
static uint8_t MajorVersion;
static uint8_t MinorVersion;
static uint8_t PatchVersion;
static uint8_t isDirtyVersion;
static uint8_t commitHash[COMMIT_HASH_STR_LEN];

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
eMBMasterReqErrCode mbzpl_MasterSendReq01(UCHAR ucSndAddr, LONG lTimeOut)
{
    UCHAR *ucMBFrame;
    BOOL ret;
    eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;
    if( ucSndAddr > MB_MASTER_TOTAL_SLAVE_NUM) {
        eErrStatus = MB_MRE_ILL_ARG;
        ESP_LOGE(TAG, "mbzpl_MasterSendReq01: Invalid slave address 0x%02X", ucSndAddr);
        return eErrStatus;
    }
    if( xMBMasterRunResTake(lTimeOut) == FALSE) {
        eErrStatus = MB_MRE_MASTER_BUSY;
        ESP_LOGE(TAG, "mbzpl_MasterSendReq01: xMBMasterRunResTake() failed.");
        return eErrStatus;
    }

    vMBMasterGetPDUSndBuf(&ucMBFrame);
    vMBMasterSetDestAddress(ucSndAddr);
    ucMBFrame[0] = MB_ZPL_REQ01;
    vMBMasterSetPDUSndLength(MB_ZPL_REQ01_LEN);
    ret = xMBMasterPortEventPost( EV_MASTER_FRAME_TRANSMIT );
    if(TRUE != ret) {
        ESP_LOGE(TAG, "mbzpl_MasterSendReq01: xMBMasterPortEventPost Failed.");
    }
    eErrStatus = eMBMasterWaitRequestFinish( );
    if(MB_MRE_NO_ERR != eErrStatus) {
        ESP_LOGE(TAG, "mbzpl_MasterSendReq01: eMBMasterWaitRequestFinish() returned 0x%02X", eErrStatus);
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

eMBException eMBZplProcessRequest01( UCHAR * pucFrame, USHORT * usLen )
{
    eMBException    eStatus = MB_EX_NONE;

    /* Parse Frame according to Zimplistic Message Specification */
    if(pucFrame[REQ01_CODE_OFFSET] != MB_ZPL_REQ01) {
        eStatus = MB_EX_ILLEGAL_FUNCTION;
        return eStatus;
    }
    SlaveContext = pucFrame[REQ01_CONTEXT_OFFSET];
    MajorVersion = pucFrame[REQ01_MAJ_VER_OFFSET];
    MinorVersion = pucFrame[REQ01_MIN_VER_OFFSET];
    PatchVersion = pucFrame[REQ01_PATCH_VER_OFFSET];
    isDirtyVersion = pucFrame[REQ01_DIRTY_VER_OFFSET];
    strncpy((char *)commitHash, (char *)&pucFrame[REQ01_COMMIT_HASH_OFFSET], (COMMIT_HASH_STR_LEN-1));

    return eStatus;
}

uint8_t Req01_GetSlaveContext(void)
{
    return SlaveContext;
}

uint8_t Req01_GetMajorVersion(void)
{
    return MajorVersion;
}

uint8_t Req01_GetMinorVersion(void)
{
    return MinorVersion;
}

uint8_t Req01_GetPatchVersion(void)
{
    return PatchVersion;
}

uint8_t Req01_isDirtyVersion(void)
{
    return isDirtyVersion;
}

uint8_t * Req01_GetCommitHash(void)
{
    return commitHash;
}

#endif /* #if defined(CONFIG_MODBUS_ZPL_MASTER) */
