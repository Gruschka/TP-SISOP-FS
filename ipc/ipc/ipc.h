/*
 * ipc.h
 *
 *  Created on: Sep 28, 2017
 *      Author: utnso
 */

#ifndef IPC_H_
#define IPC_H_

int ipc_getNextOperationId(int socket);
void *ipc_recvMessage(int sockfd, int type);
int ipc_sendMessage(int sockfd, int type, void *content);
int ipc_createAndConnect(int port, char *ipAddress);
int ipc_createAndListen(int port, int optval);

#endif /* IPC_H_ */
