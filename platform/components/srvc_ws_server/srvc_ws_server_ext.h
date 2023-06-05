/**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**  @file       : srvc_ws_server_ext.h
**  @author     : Nguyen Ngoc Tung (ngoctung.dhbk@gmail.com)
**  @date       : 2022 Apr 9
**  @brief      : Header file containing configuration of Srvc_WS_Server module
**  @namespace  : WSS
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @addtogroup  Srvc_WS_Server
** @{
*/

#ifndef __SRVC_WS_SERVER_EXT_H__
#define __SRVC_WS_SERVER_EXT_H__

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
** @brief   This table defines channel instances of the Websocket Server and their configuration.
** @details
**
** This module only provides one Websocket server. The server supports multiple communication channels which are
** uniquely identified by URI string. Each instance in this table encapsulates a communication channel of the Websocket
** server and has the following properties:
**
** - Instance_ID            : Alias of a channel instance. This alias is used in x_WSS_Get_Inst() to get handle of the
**                            corresponding URI instance.
**
** - URI                    : URI (Uniform Resource Identifier) of the channel instance. For example, if URI is "/a/b/c"
**                            then the URL to access the channel over the network is "ws://<device_ip>/a/b/c"
**
** - Max_Clients            : Maximum number of clients that can connect to the channel at a time
**
*/
#define WSS_INST_TABLE(X)                                                        \
                                                                                 \
/*-----------------------------------------------------------------------------*/\
/*  Instance_ID             Configuration                                      */\
/*-----------------------------------------------------------------------------*/\
                                                                                 \
/*  Channel monitoring slave board's status                                    */\
X(  WSS_SLAVE_STATUS,       /* URI          */  "/slave/status"                 ,\
                            /* Max_Clients  */  3                               )\
                                                                                 \
/*  Channel of slave board's realtime log messages                             */\
X(  WSS_SLAVE_RTLOG,        /* URI          */  "/slave/rtlog"                  ,\
                            /* Max_Clients  */  3                               )\
                                                                                 \
/*-----------------------------------------------------------------------------*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#endif /* __SRVC_WS_SERVER_EXT_H__ */

/**
** @}
** @}
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
