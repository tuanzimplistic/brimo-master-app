/*
 * (C) Copyright 2021
 * Zimplistic Private Limited
 * Toan Dang, toan.dang@zimplistic.com
 */
#ifndef _MBZPL_REQ_M_H
#define _MBZPL_REQ_M_H

#include "mb_m.h"
#include "request/mbzpl_req01_m.h"
#include "request/mbzpl_req02_m.h"

/*! \ingroup mbzpl_req_m
 * \brief Slave address
 *
 */
#define SLAVE_ADDR          (0x01)

/*! \defgroup mbzpl_req_m ZPL Modbus Master
 * \code #include "mbzpl_req_m.h" \endcode
 *
 * This module defines the interface for the application. It contains
 * the basic functions and types required to send commands to slave board using
 * Modbus Master protocol stack.
 * A typical application will want to call MAL_REQ_init() first. The main loop will be
 * created to send "Get State" command periodically to get state of sub modules.
 * The time interval between pooling depends on the timeout configuration of each modules.
 *
 */

/*! \ingroup mbzpl_req_m
 * \brief Initialize the Modbus Master module.
 *
 * This functions initializes the modbus master by calling the function
 * eMBMasterInit() and registers corresponding callback functions using function mbzpl_register_all() for responded modbus buffer.
 *
 ** \return If no error occurs the function returns ESP_OK.
 * Otherwise ESP_FAIL.
 *
 */
uint8_t MAL_REQ_init(void);

/*! \ingroup mbzpl_req_m
 * \brief Send the modbus package.
 *
 * This functions sends the modbus package to the slave.
 * Please note that this function should be called after the modbus master
 * has been initialized by the function MAL_REQ_init().
 *
 * \param ucSndAddr The slave address.
 * \param lTimeOut The timeout to wait until the modbus frame is idle.
 * \param usLength The length of the package which will be sent.
 * \param ucBufPtr Pointer to the package which will be sent.
 *
 * \return If no error occurs the function returns eMBMasterReqErrCode::MB_MRE_NO_ERR.
 * Otherwise one of the following error codes is returned:
 *    - eMBMasterReqErrCode::MB_MRE_ILL_ARG If the slave address is invalid.
 *    - eMBMasterReqErrCode::MB_MRE_MASTER_BUSY If the modbus master is busy after the lTimeOut is expired.
 *
 */
eMBMasterReqErrCode mbzpl_MasterSendReq(UCHAR ucSndAddr, LONG lTimeOut, USHORT usLength, UCHAR *ucBufPtr);

#endif /*_MBZPL_REQ_M_H*/
