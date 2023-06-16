/*
 * (C) Copyright 2020
 * Zimplistic Private Limited
 * Sicris Rey Embay, sicris@zimplistic.com
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/

#include "mb_m.h"
#include "mbzpl.h"
#include "mbframe.h"

#include "mbcrc.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_ZPL_SER_PDU_SIZE_MIN 4

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_M_RX_INIT,              /*!< Receiver is in initial state. */
    STATE_M_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_M_RX_RCV,               /*!< Frame is beeing received. */
    STATE_M_RX_ERROR,             /*!< If the frame is invalid. */
} eMBMasterRcvState;

typedef enum
{
    STATE_M_TX_IDLE,              /*!< Transmitter is in idle state. */
    STATE_M_TX_XMIT,              /*!< Transmitter is in transfer state. */
    STATE_M_TX_XFWR,              /*!< Transmitter is in transfer finish and wait receive state. */
} eMBMasterSndState;

#if defined(CONFIG_MODBUS_ZPL_MASTER)
/*------------------------ Shared variables ---------------------------------*/
extern volatile UCHAR   ucMasterRcvBuf[];
extern volatile UCHAR   ucMasterSndBuf[];

/* ----------------------- Static variables ---------------------------------*/
static volatile eMBMasterSndState   eSndState;
static volatile eMBMasterRcvState   eRcvState;

static volatile UCHAR   *pucMasterSndBufferCur;
static volatile USHORT  usMasterSndBufferCount;
static volatile USHORT  usMasterRcvBufferPos;

static volatile UCHAR *ucMasterZPLRcvBuf = ucMasterRcvBuf;
static volatile UCHAR *ucMasterZPLSndBuf = ucMasterSndBuf;

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBMasterZPLInit(UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    ULONG           usTimerT35_50us;

    ENTER_CRITICAL_SECTION(  );

    if( xMBMasterPortSerialInit( ucPort, ulBaudRate, 8, eParity ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    }
    else
    {
        /* If baudrate > 19200 then we should use the fixed timer values
         * t35 = 1750us. Otherwise t35 must be 3.5 times the character time.
         */
        if( ulBaudRate > 19200 )
        {
            usTimerT35_50us = 35;       /* 1800us. */
        }
        else
        {
            /* The timer reload value for a character is given by:
             *
             * ChTimeValue = Ticks_per_1s / ( Baudrate / 11 )
             *             = 11 * Ticks_per_1s / Baudrate
             *             = 220000 / Baudrate
             * The reload for t3.5 is 1.5 times this value and similary
             * for t3.5.
             */
            usTimerT35_50us = ( 7UL * 220000UL ) / ( 2UL * ulBaudRate );
        }
        if( xMBMasterPortTimersInit( ( USHORT ) usTimerT35_50us ) != TRUE )
        {
            eStatus = MB_EPORTERR;
        }
    }
    EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

void
eMBMasterZPLStart( void )
{
    ENTER_CRITICAL_SECTION(  );
    /* Initially the receiver is in the state STATE_M_RX_INIT. we start
     * the timer and if no character is received within t3.5 we change
     * to STATE_M_RX_IDLE. This makes sure that we delay startup of the
     * modbus protocol stack until the bus is free.
     */
    eRcvState = STATE_M_RX_IDLE; //STATE_M_RX_INIT (We start processing immediately)
    vMBMasterPortSerialEnable( TRUE, FALSE );
    vMBMasterPortTimersT35Enable(  );

    EXIT_CRITICAL_SECTION(  );
}

void
eMBMasterZPLStop( void )
{
    ENTER_CRITICAL_SECTION(  );
    vMBMasterPortSerialEnable( FALSE, FALSE );
    vMBMasterPortTimersDisable(  );
    EXIT_CRITICAL_SECTION(  );
}

eMBErrorCode
eMBMasterZPLReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    ENTER_CRITICAL_SECTION(  );
    assert( usMasterRcvBufferPos < MB_SER_PDU_SIZE_MAX );
    
    /////////////debug
    if(usMasterRcvBufferPos >= MB_ZPL_SER_PDU_SIZE_MIN ){
        USHORT len = ( USHORT )( usMasterRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );
        for(int i=0; i<len; i++){
            ESP_LOGE("modbus", "data[%d] = %d", i, (int)(( UCHAR * ) ucMasterZPLRcvBuf)[i]);
        }
    }
    /////////////debug

    /* Length and CRC check */
    if( ( usMasterRcvBufferPos >= MB_ZPL_SER_PDU_SIZE_MIN )
        && ( usMBCRC16( ( UCHAR * ) ucMasterZPLRcvBuf, usMasterRcvBufferPos ) == 0 ) )
    {
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = ucMasterZPLRcvBuf[MB_SER_PDU_ADDR_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = ( USHORT )( usMasterRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );

        /* Return the start of the Modbus PDU to the caller. */
        *pucFrame = ( UCHAR * ) & ucMasterZPLRcvBuf[MB_SER_PDU_PDU_OFF];
    }
    else
    {
        eStatus = MB_EIO;
    }

    EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBMasterZPLSend( UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          usCRC16;

    if ( ucSlaveAddress > MB_MASTER_TOTAL_SLAVE_NUM ) return MB_EINVAL;

    ENTER_CRITICAL_SECTION(  );

    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if( eRcvState == STATE_M_RX_IDLE )
    {
        /* First byte before the Modbus-PDU is the slave address. */
        pucMasterSndBufferCur = ( UCHAR * ) pucFrame - 1;
        usMasterSndBufferCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pucMasterSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
        usMasterSndBufferCount += usLength;

        /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
        usCRC16 = usMBCRC16( ( UCHAR * ) pucMasterSndBufferCur, usMasterSndBufferCount );
        ucMasterZPLSndBuf[usMasterSndBufferCount++] = ( UCHAR )( usCRC16 & 0xFF );
        ucMasterZPLSndBuf[usMasterSndBufferCount++] = ( UCHAR )( usCRC16 >> 8 );

        /* Activate the transmitter. */
        eSndState = STATE_M_TX_XMIT;
        // The place to enable RS485 driver
        vMBMasterPortSerialEnable( FALSE, TRUE );
    }
    else
    {
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

BOOL
xMBMasterZPLReceiveFSM( UCHAR ucByte )
{
    assert(( eSndState == STATE_M_TX_IDLE ) || ( eSndState == STATE_M_TX_XFWR ));

    switch ( eRcvState )
    {
        /* If we have received a character in the init state we have to
         * wait until the frame is finished.
         */
    case STATE_M_RX_INIT:
        vMBMasterPortTimersT35Enable( );
        break;

        /* In the error state we wait until all characters in the
         * damaged frame are transmitted.
         */
    case STATE_M_RX_ERROR:
        vMBMasterPortTimersT35Enable( );
        break;

        /* In the idle state we wait for a new character. If a character
         * is received the t1.5 and t3.5 timers are started and the
         * receiver is in the state STATE_M_RX_RCV and disable early
         * the timer of respond timeout .
         */
    case STATE_M_RX_IDLE:
        /* In time of respond timeout,the receiver receive a frame.
         * Disable timer of respond timeout and change the transmiter state to idle.
         */
        vMBMasterPortTimersDisable( );
        eSndState = STATE_M_TX_IDLE;

        usMasterRcvBufferPos = 0;
        ucMasterZPLRcvBuf[usMasterRcvBufferPos++] = ucByte;
        eRcvState = STATE_M_RX_RCV;

        /* Enable t3.5 timers. */
        vMBMasterPortTimersT35Enable( );
        break;

        /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
    case STATE_M_RX_RCV:
        if( usMasterRcvBufferPos < MB_SER_PDU_SIZE_MAX )
        {
            ucMasterZPLRcvBuf[usMasterRcvBufferPos++] = ucByte;
        }
        else
        {
            eRcvState = STATE_M_RX_ERROR;
        }
        vMBMasterPortTimersT35Enable( );
        break;
    }
    return TRUE;
}

BOOL
xMBMasterZPLTransmitFSM( void )
{
    BOOL xNeedPoll = TRUE;
    BOOL xFrameIsBroadcast = FALSE;

    assert( eRcvState == STATE_M_RX_IDLE );

    switch ( eSndState )
    {
        /* We should not get a transmitter event if the transmitter is in
         * idle state.  */
    case STATE_M_TX_XFWR:
        xNeedPoll = FALSE;
        break;

    case STATE_M_TX_IDLE:
        break;

    case STATE_M_TX_XMIT:
        /* check if we are finished. */
        if( usMasterSndBufferCount != 0 )
        {
            xMBMasterPortSerialPutByte( ( CHAR )*pucMasterSndBufferCur );
            pucMasterSndBufferCur++;  /* next byte in sendbuffer. */
            usMasterSndBufferCount--;
        }
        else
        {
            xFrameIsBroadcast = ( ucMasterZPLSndBuf[MB_SER_PDU_ADDR_OFF] == MB_ADDRESS_BROADCAST ) ? TRUE : FALSE;
            vMBMasterRequestSetType( xFrameIsBroadcast );
            eSndState = STATE_M_TX_XFWR;
            /* If the frame is broadcast ,master will enable timer of convert delay,
             * else master will enable timer of respond timeout. */
            if ( xFrameIsBroadcast == TRUE )
            {
                vMBMasterPortTimersConvertDelayEnable( );
            }
            else
            {
                vMBMasterPortTimersRespondTimeoutEnable( );
            }
        }
        break;
    }

    return xNeedPoll;
}

BOOL MB_PORT_ISR_ATTR xMBMasterZPLTimerExpired(void)
{
    BOOL xNeedPoll = FALSE;

    switch (eRcvState)
    {
        /* Timer t35 expired. Startup phase is finished. */
    case STATE_M_RX_INIT:
        xNeedPoll = xMBMasterPortEventPost(EV_MASTER_READY);
        break;

        /* A frame was received and t35 expired. Notify the listener that
         * a new frame was received. */
    case STATE_M_RX_RCV:
        xNeedPoll = xMBMasterPortEventPost(EV_MASTER_FRAME_RECEIVED);
        break;

        /* An error occured while receiving the frame. */
    case STATE_M_RX_ERROR:
        vMBMasterSetErrorType(EV_ERROR_RECEIVE_DATA);
        xNeedPoll = xMBMasterPortEventPost(EV_MASTER_ERROR_PROCESS);
        break;

        /* Function called in an illegal state. */
    default:
        assert(eRcvState == STATE_M_RX_IDLE);
        break;
    }
    eRcvState = STATE_M_RX_IDLE;

    switch (eSndState)
    {
        /* A frame was send finish and convert delay or respond timeout expired.
         * If the frame is broadcast,The master will idle,and if the frame is not
         * broadcast. Notify the listener process error.*/
    case STATE_M_TX_XFWR:
        if ( xMBMasterRequestIsBroadcast( ) == FALSE ) {
            vMBMasterSetErrorType(EV_ERROR_RESPOND_TIMEOUT);
            xNeedPoll = xMBMasterPortEventPost(EV_MASTER_ERROR_PROCESS);
        }
        break;
        /* Function called in an illegal state. */
    default:
        assert( ( eSndState == STATE_M_TX_XMIT ) || ( eSndState == STATE_M_TX_IDLE ));
        break;
    }
    eSndState = STATE_M_TX_IDLE;

    vMBMasterPortTimersDisable( );
    /* If timer mode is convert delay, the master event then turns EV_MASTER_EXECUTE status. */
    if (xMBMasterGetCurTimerMode() == MB_TMODE_CONVERT_DELAY) {
        xNeedPoll = xMBMasterPortEventPost(EV_MASTER_EXECUTE);
    }

    return xNeedPoll;
}


#endif /* #if defined(CONFIG_MODBUS_ZPL_MASTER) */


