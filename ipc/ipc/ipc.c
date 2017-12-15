/*
 * ipc.c
 *
 *  Created on: Sep 28, 2017
 *      Author: utnso
 */
#include "ipc.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>

#define MAXEVENTS 64


int ipc_getNextOperationId(int socket) {
	char type;
	char buffer[sizeof(char)];

	if(recv(socket, buffer, sizeof(ipc_struct_header), MSG_PEEK) < 0){
		exit(1);
	}

	memcpy(&type, buffer, sizeof(char));

	return type;
}


// Reads from a socket and returns a deserialized struct + and handshake
void *ipc_recvMessage(int sockfd, int type) {
	char *buffer;
	ipc_struct_header header;

	if (recv(sockfd, &header, sizeof(ipc_struct_header), MSG_WAITALL) <= 0) {
		exit(1);
	}

	buffer = malloc(header.length);
	int bytesReceived = recv(sockfd, buffer, header.length, MSG_WAITALL);
	if (bytesReceived < 0){
		exit(1);
	}
	if (bytesReceived != header.length) {
		printf("bytesReceived != header.length. %d != %d", bytesReceived, header.length);
		fflush(stdout);
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

	result = send(sockfd, buffer, sizeof(ipc_struct_header) + *size, 0);

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
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval));

	socketInfo.sin_family = AF_INET;
	socketInfo.sin_addr.s_addr = INADDR_ANY;
	socketInfo.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr*) &socketInfo, sizeof(socketInfo)) != 0) {
		exit(1);
	}
	listen(sockfd, 10);
	return sockfd;
}

int make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1) { return -1; }

  return 0;
}

int create_and_bind (char *port) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s, sfd;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */

  s = getaddrinfo (NULL, port, &hints, &result);
  if (s != 0)
    {
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
      return -1;
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
	  int iSetOption = 1;
      sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char*)&iSetOption,
              sizeof(iSetOption));
      if (sfd == -1)
        continue;

      s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
      if (s == 0)
        {
          /* We managed to bind successfully! */
          break;
        }

      close (sfd);
    }

  if (rp == NULL)
  {
      return -1;
    }

  freeaddrinfo (result);

  return sfd;
}

bool is_connected(int socket) {
	char buffer[1];
	return recv(socket, buffer, sizeof buffer, MSG_PEEK) > 0;
}

int ipc_createSelectServer(char *port, ConnectionEventHandler newConnectionHandler, IncomingDataEventHandler incomingDataHandler, DisconnectionEventHandler disconnectionHandler) {
	int listeningSocket;

	listeningSocket = create_and_bind (port);

	if (listeningSocket == -1) return -1;

	if (make_socket_non_blocking (listeningSocket) == -1) return -1;

	if (listen (listeningSocket, SOMAXCONN) == -1) { return -1; }

	int i, new_fd, max_fd = listeningSocket;
	fd_set master, read_fds;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(listeningSocket, &master);

	while (1) {
		read_fds = master;

		if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
			printf("error en el select");
			fflush(stdout);
			exit(1);
		}

		for(i = 0 ; i <= max_fd ; i++) {
			if (FD_ISSET(i, &read_fds)) // buscar los seteados
			{
				if (i == listeningSocket)
				{
					struct sockaddr_in clientaddr;
					socklen_t clientaddr_size = sizeof(clientaddr);
					new_fd = accept(listeningSocket, (struct sockaddr*) &clientaddr, &clientaddr_size); // si es el escucha se tiene un nuevofd
					char *ip = strdup(inet_ntoa(clientaddr.sin_addr));
					newConnectionHandler(new_fd, ip);
					FD_SET(new_fd, &master);

					if (new_fd > max_fd) // chequear si el nuevo fd es mas grande que el maximo
					{
						max_fd = new_fd;
					}
				}

				if(i != listeningSocket) // si no son los socket especiales es un mensaje...
				{
					if(is_connected(i))  	//si el socket sigue estando conectado
					{
						ipc_struct_header *header = malloc(sizeof(ipc_struct_header));
						int receivedBytes = recv(i, header, sizeof(ipc_struct_header), MSG_PEEK);

						if (receivedBytes == sizeof(ipc_struct_header)) {
							incomingDataHandler(i, *header);
							free(header);
						} else {
							printf("error en el read");
							fflush(stdout);
						}
					} else { //se desconecto
						close(i);
						disconnectionHandler(i, NULL);
						FD_CLR(i, &master);
					}
				}
			} // fin actividad en socket
		} // fin for
	}

	return EXIT_SUCCESS;
}
