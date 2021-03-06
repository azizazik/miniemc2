/********************************************************************
* Description: sockets.c
*   socket utilites
*
* Copyright(c) 2001, Joris Robijn
*          (c) 2003, Rene Wagner
* Adapted for EMC by: Eric H. Johnson
* License: GPL Version 2
* System: Linux
*
* Copyright (c) 2007 All rights reserved.
*
* Last change:
* $Revision: 1.1 $
* $Author: eric-johnson $
* $Date: 2007/02/20 01:55:48 $
********************************************************************/

#ifdef __cplusplus
 extern "C" {
#endif 

#ifndef SOCKETS_H
#define SOCKETS_H

#include <stdlib.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef LCDPORT
# define LCDPORT 13666
#endif

#ifndef SHUT_RDWR
# define SHUT_RDWR 2
#endif

// Client functions...
extern int sockConnect(char *host, unsigned short int port);
extern int sockClose(int fd);
// Send/receive lines of text
extern int sockPrintf(int fd, const char *format, .../*args*/);
extern int sockSendString(int fd, char *string);
// Recv gives only one line per call...
extern int sockRecvString(int fd, char *dest, size_t maxlen);
// Send/receive raw data
extern int sockSend(int fd, void *src, size_t size);
extern int sockRecv(int fd, void *dest, size_t maxlen);

/* Return error message string for the socket function */
extern char *sockGetError(void);
extern int sockSendError(int fd, char* message);
extern int sockPrintfError(int fd, const char *format, .../*args*/);

#ifdef __cplusplus
}
#endif

#endif
