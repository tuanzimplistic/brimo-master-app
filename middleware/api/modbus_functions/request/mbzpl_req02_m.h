/*
 * (C) Copyright 2020
 * Zimplistic Private Limited
 * Sicris Rey Embay, sicris@zimplistic.com
 */

#ifndef _MB_ZPL_MASTER_REQ02_H
#define _MB_ZPL_MASTER_REQ02_H

#include "port.h"

#if defined(CONFIG_MODBUS_ZPL_MASTER)

/* Request 0x02 is used to request slave board to enter Bootloader mode */
eMBMasterReqErrCode mbzpl_MasterSendReq02(UCHAR ucSndAddr, LONG lTimeOut);

#endif /* #if defined(CONFIG_MODBUS_ZPL_MASTER) */
#endif /* _MB_ZPL_MASTER_REQ02_H */
