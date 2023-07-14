/*
 * (C) Copyright 2020
 * Zimplistic Private Limited
 * Sicris Rey Embay, sicris@zimplistic.com
 */

#include "mbconfig.h"

#ifndef _MB_ZPL_H
#define _MB_ZPL_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/
#define MB_SER_PDU_SIZE_MIN     4       /*!< Minimum size of a Modbus RTU frame. */

#if MB_SLAVE_ZPL_ENABLED > 0
eMBErrorCode    eMBZPLInit( UCHAR slaveAddress, UCHAR ucPort, ULONG ulBaudRate,
                            eMBParity eParity );
void            eMBZPLStart( void );
void            eMBZPLStop( void );
eMBErrorCode    eMBZPLReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength );
eMBErrorCode    eMBZPLSend( UCHAR slaveAddress, const UCHAR * pucFrame, USHORT usLength );
BOOL            xMBZPLReceiveFSM( void );
BOOL            xMBZPLTransmitFSM( void );
BOOL            xMBZPLTimerT15Expired( void );
BOOL            xMBZPLTimerT35Expired( void );
eMBErrorCode    xMBZPLStoreRxFrame( UCHAR * pucBuf, USHORT usLength);
#endif

#if MB_MASTER_ZPL_ENABLED > 0
eMBErrorCode    eMBMasterZPLInit( UCHAR ucPort, ULONG ulBaudRate,eMBParity eParity );
void            eMBMasterZPLStart( void );
void            eMBMasterZPLStop( void );
eMBErrorCode    eMBMasterZPLReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength );
eMBErrorCode    eMBMasterZPLSend( UCHAR slaveAddress, const UCHAR * pucFrame, USHORT usLength );
BOOL            xMBMasterZPLReceiveFSM( void );
BOOL            xMBMasterZPLTransmitFSM( void );
BOOL            xMBMasterZPLTimerExpired( void );
#endif

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif /* _MB_ZPL_H */
