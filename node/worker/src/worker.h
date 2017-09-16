/*
 * worker.h
 *
 *  Created on: 9/9/2017
 *      Author: utnso
 */

#ifndef WORKER_H_
#define WORKER_H_

#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void logDebug(char *);
void loadConfiguration();
void signalHandler(int);
void connectionHandler(int client_fd);
void readABlock(uint32_t block, char *blockContent);


#endif /* WORKER_H_ */
