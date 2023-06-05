/*
 * (C) Copyright 2020
 * Zimplistic Private Limited
 * Sicris Rey Embay, sicris@zimplistic.com
 */
/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbzpl.h"
#include "mbframe.h"

//#include "mbcrc.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_SER_PDU_SIZE_MIN     4       /*!< Minimum size of a Modbus ZPL frame. */
#define MB_SER_PDU_SIZE_MAX     CONFIG_MODBUS_ZPL_SLAVE_MAX_FRAME_SIZE  /*!< Maximum size of a Modbus ZPL frame. */
#define MB_SER_PDU_SIZE_CRC     2       /*!< Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RX_INIT,              /*!< Receiver is in initial state. */
    STATE_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_RX_RCV,               /*!< Frame is beeing received. */
    STATE_RX_ERROR              /*!< If the frame is invalid. */
} eMBRcvState;

typedef enum
{
    STATE_TX_IDLE,              /*!< Transmitter is in idle state. */
    STATE_TX_XMIT               /*!< Transmitter is in transfer state. */
} eMBSndState;

/* ----------------------- Static variables ---------------------------------*/
static volatile eMBSndState eSndState;
static volatile eMBRcvState eRcvState;

volatile UCHAR  ucZPLBuf[MB_SER_PDU_SIZE_MAX];

static volatile UCHAR *pucSndBufferCur;
static volatile USHORT usSndBufferCount;

static volatile USHORT usRcvBufferPos;

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBZPLInit( UCHAR ucSlaveAddress, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    /* Modbus RTU uses 8 Databits. */
    if( xMBPortSerialInit( ucPort, ulBaudRate, 8, eParity ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    } else {
        /// TODO: Initialize timers.  However, IMHO no need
        /// T3.5 is handled by UART Rx Timeout Interrupt
    }
    return eStatus;
}

void
eMBZPLStart( void )
{

}

void
eMBZPLStop( void )
{
}

eMBErrorCode
eMBZPLReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    /* Length and CRC check */
    if( ( usRcvBufferPos >= MB_SER_PDU_SIZE_MIN)
        && ( usMBCRC16( ( UCHAR * ) ucZPLBuf, usRcvBufferPos) == 0)) {
        *pucRcvAddress = ucZPLBuf[MB_SER_PDU_ADDR_OFF];
        *pusLength = ( USHORT )( usRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC);
        *pucFrame = ( UCHAR *) & ucZPLBuf[MB_SER_PDU_PDU_OFF];
    } else {
        eStatus = MB_EIO;
    }

    return eStatus;
}

eMBErrorCode
eMBZPLSend( UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          usCRC16;

    if( eRcvState == STATE_RX_IDLE) {
        /* First byte before the Modbus-PDU is the slave address. */
        pucSndBufferCur = ( UCHAR * ) pucFrame - 1;
        usSndBufferCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pucSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
        usSndBufferCount += usLength;

        /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
        usCRC16 = usMBCRC16( ( UCHAR * ) pucSndBufferCur, usSndBufferCount );
        ucZPLBuf[usSndBufferCount++] = ( UCHAR )( usCRC16 & 0xFF );
        ucZPLBuf[usSndBufferCount++] = ( UCHAR )( usCRC16 >> 8 );

        if(TRUE != xMBPortSerialPut(ucZPLBuf, usSndBufferCount)) {
            eStatus = MB_EIO;
        }
    } else {
        eStatus = MB_EIO;
    }
    return eStatus;
}

BOOL
xMBZPLReceiveFSM( void )
{
    BOOL            xTaskNeedSwitch = FALSE;
    return xTaskNeedSwitch;
}

BOOL
xMBZPLTransmitFSM( void )
{
    BOOL            xNeedPoll = FALSE;

    return xNeedPoll;
}

BOOL
xMBZPLTimerT35Expired( void )
{
    BOOL            xNeedPoll = FALSE;

    eRcvState = STATE_RX_IDLE;
    return xNeedPoll;
}

eMBErrorCode
xMBZPLStoreRxFrame( UCHAR * pucBuf, const USHORT usLength)
{
    USHORT i;
    if(((UCHAR *)0 == pucBuf) || (usLength > MB_SER_PDU_SIZE_MAX)) {
        return MB_EINVAL;
    }

    memcpy(ucZPLBuf, pucBuf, usLength);
    usRcvBufferPos = usLength;

    return MB_ENOERR;
}
