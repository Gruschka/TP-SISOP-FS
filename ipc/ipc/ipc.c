/*
 * ipc.c
 *
 *  Created on: Sep 28, 2017
 *      Author: utnso
 */
#include "serialization.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

int ipc_getNextOperationId(int socket) {
	char type;
	char buffer[sizeof(char)];

	if(recv(socket, buffer, sizeof(ipc_struct_header), MSG_PEEK) < 0){
		exit(1);
	}

	memcpy(&type, buffer, sizeof(char));

	return type;
}


// Reads from a socket and returns a deserialized struct
void *ipc_recvMessage(int sockfd, int type) {
	char *buffer;
	ipc_struct_header header;

	if (recv(sockfd, &header, sizeof(ipc_struct_header), MSG_WAITALL) <= 0) {
		exit(1);
	}

	buffer = malloc(header.length);
	if(recv(sockfd, buffer, header.length, MSG_WAITALL) < 0){
		exit(1);
	}

	void *deserialized = (*deserializationArray[type])(buffer);
	bool isValidType = header.type == type;
	free(buffer);
	return isValidType ? deserialized : NULL;
}

int ipc_sendMessage(int sockfd, int type, void *content) {
	ipc_struct_header header;
	int result, *size = malloc(sizeof(int));
	char *serialized = (*serializationArray[type])(content, size);
	char *buffer = malloc(sizeof(ipc_struct_header) + *size);

	header.type = type;
	header.length = *size;

	memmove(buffer, &header, sizeof(ipc_struct_header));
	memcpy(buffer + sizeof(ipc_struct_header), serialized, *size);

	result = send(sockfd, buffer, sizeof(ipc_struct_header) + *size, MSG_NOSIGNAL);

	free(serialized);
	free(size);
	free(buffer);
	return result;
}

int ipc_createAndConnect(int port, char *ipAddress) {
	int sockfd;
	struct linger lo = { 1, 0 };
	struct sockaddr_in socketInfo;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		exit(1);
	}
	fcntl(sockfd, F_SETFL, FD_CLOEXEC);

	setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &lo, sizeof(lo));

	socketInfo.sin_family = AF_INET;
	socketInfo.sin_addr.s_addr = inet_addr(ipAddress);
	socketInfo.sin_port = htons(port);

	connect(sockfd, (struct sockaddr*) &socketInfo, sizeof(socketInfo));
	return sockfd;
}


int ipc_createAndListen(int port, int optval) {
	int sockfd;
	struct sockaddr_in socketInfo;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		exit(1);
	}
	fcntl(sockfd, F_SETFL, FD_CLOEXEC);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	socketInfo.sin_family = AF_INET;
	socketInfo.sin_addr.s_addr = INADDR_ANY;
	socketInfo.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr*) &socketInfo, sizeof(socketInfo)) != 0) {
		exit(1);
	}
	listen(sockfd, 10);
	return sockfd;
}
