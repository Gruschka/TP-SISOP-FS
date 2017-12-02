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
      setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption,
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

int ipc_createEpollServer(char *port, EpollConnectionEventHandler newConnectionHandler, EpollIncomingDataEventHandler incomingDataHandler, EpollDisconnectionEventHandler disconnectionHandler) {
  int sfd, s;
  int efd;
  struct epoll_event event;
  struct epoll_event *events;

  sfd = create_and_bind (port);
  if (sfd == -1) return -1;

  s = make_socket_non_blocking (sfd);
  if (s == -1) return -1;

  s = listen (sfd, SOMAXCONN);
  if (s == -1) { return -1; }

  efd = epoll_create1 (0);
  if (efd == -1)
    {
      perror ("epoll_create");
      abort ();
    }

  event.data.fd = sfd;
  event.events = EPOLLIN | EPOLLET;
  s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
  if (s == -1)
    {
      perror ("epoll_ctl");
      abort ();
    }

  /* Buffer where events are returned */
  events = calloc (MAXEVENTS, sizeof event);

  /* The event loop */
  while (1)
    {
      int n, i;

      n = epoll_wait (efd, events, MAXEVENTS, -1);
      for (i = 0; i < n; i++)
	{
	  if ((events[i].events & EPOLLERR) ||
              (events[i].events & EPOLLHUP) ||
              (!(events[i].events & EPOLLIN)))
	    {
              /* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
	      fprintf (stderr, "epoll error\n");
	      close (events[i].data.fd);
	      continue;
	    }

	  else if (sfd == events[i].data.fd)
	    {
              /* We have a notification on the listening socket, which
                 means one or more incoming connections. */
              while (1)
                {
                  struct sockaddr in_addr;
                  socklen_t in_len;
                  int infd;
                  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                  in_len = sizeof in_addr;
                  infd = accept (sfd, &in_addr, &in_len);
                  if (infd == -1)
                    {
                      if ((errno == EAGAIN) ||
                          (errno == EWOULDBLOCK))
                        {
                          /* We have processed all incoming
                             connections. */
                          break;
                        }
                      else
                        {
                          perror ("accept");
                          break;
                        }
                    }

                  s = getnameinfo (&in_addr, in_len,
                                   hbuf, sizeof hbuf,
                                   sbuf, sizeof sbuf,
                                   NI_NUMERICHOST | NI_NUMERICSERV);

                  /* Make the incoming socket non-blocking and add it to the
                     list of fds to monitor. */
                  s = make_socket_non_blocking (infd);
                  if (s == -1)
                    abort ();

                  event.data.fd = infd;
                  event.events = EPOLLIN | EPOLLET;
                  s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
                  newConnectionHandler(infd);
                  if (s == -1)
                    {
                      perror ("epoll_ctl");
                      abort ();
                    }
                }
              continue;
            }
          else
            {
              /* We have data on the fd waiting to be read. Read and
                 display it. We must read whatever data is available
                 completely, as we are running in edge-triggered mode
                 and won't get a notification again for the same
                 data. */
              void *buffer = malloc(sizeof(ipc_struct_header));
              ssize_t count;
              count = recv(events[i].data.fd, buffer, sizeof(ipc_struct_header), MSG_PEEK);

              if (count == sizeof(ipc_struct_header)) {
            	  ipc_struct_header *header = malloc(sizeof(ipc_struct_header));
            	  memcpy(header, buffer, sizeof(ipc_struct_header));
            	  incomingDataHandler(events[i].data.fd, *header);
            	  free(header);
              } else if (count == 0) {
            	  disconnectionHandler(events[i].data.fd);
              } else {
            	  // Manejar error
            	  printf("manejar");
              }

              free(buffer);
            }
        }
    }

  free (events);

  close (sfd);

  return EXIT_SUCCESS;
}
