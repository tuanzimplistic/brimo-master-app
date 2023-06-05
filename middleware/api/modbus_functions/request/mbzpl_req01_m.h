/*
 * (C) Copyright 2020
 * Zimplistic Private Limited
 * Sicris Rey Embay, sicris@zimplistic.com
 */

#ifndef _MB_ZPL_MASTER_REQ01_H
#define _MB_ZPL_MASTER_REQ01_H

#include "port.h"

#define REQ01_CODE_OFFSET           (0x00)
#define REQ01_CONTEXT_OFFSET        (0x01)
#define REQ01_MAJ_VER_OFFSET        (0x02)
#define REQ01_MIN_VER_OFFSET        (0x03)
#define REQ01_PATCH_VER_OFFSET      (0x04)
#define REQ01_DIRTY_VER_OFFSET      (0x05)
#define REQ01_COMMIT_HASH_OFFSET    (0x06)

#define COMMIT_HASH_STR_LEN         (41)    /* Git hash is 40 digit string + Null termination */
#define REQ01_LEN                   (6 + COMMIT_HASH_STR_LEN)

#define SLAVE_BOOT_CONTEXT          (0x01)
#define SLAVE_APPL_CONTEXT          (0x02)

#if defined(CONFIG_MODBUS_ZPL_MASTER)

/* Request 0x01 is used to get version information of slave firmware */
eMBMasterReqErrCode mbzpl_MasterSendReq01(UCHAR ucSndAddr, LONG lTimeOut);
eMBException eMBZplProcessRequest01( UCHAR * pucFrame, USHORT * usLen );

/* Accessor of Req01 state variables */
uint8_t Req01_GetSlaveContext(void);
uint8_t Req01_GetMajorVersion(void);
uint8_t Req01_GetMinorVersion(void);
uint8_t Req01_GetPatchVersion(void);
uint8_t Req01_isDirtyVersion(void);
uint8_t * Req01_GetCommitHash(void);

#endif /* #if defined(CONFIG_MODBUS_ZPL_MASTER) */
#endif /* _MB_ZPL_MASTER_REQ01_H */
