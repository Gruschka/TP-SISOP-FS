/*
 * ipc.h
 *
 *  Created on: Sep 28, 2017
 *      Author: utnso
 */

#ifndef IPC_H_
#define IPC_H_

#include "serialization.h"

typedef void (*ConnectionEventHandler)(int fd, char *ip);
typedef ConnectionEventHandler DisconnectionEventHandler;
typedef void (*IncomingDataEventHandler)(int fd, ipc_struct_header header);

int ipc_getNextOperationId(int socket);
void *ipc_recvMessage(int sockfd, int type);
int ipc_sendMessage(int sockfd, int type, void *content);
int ipc_createAndConnect(int port, char *ipAddress);
int ipc_createAndListen(int port, int optval);
int ipc_createSelectServer(char *port, ConnectionEventHandler newConnectionHandler, IncomingDataEventHandler incomingDataHandler, DisconnectionEventHandler disconnectionHandler);

#endif /* IPC_H_ */
