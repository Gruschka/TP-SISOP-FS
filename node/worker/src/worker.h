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
#include <stdio.h>
#include <commons/collections/list.h>

typedef struct file {
	char * filePath;
	uint32_t filePathLength;
}fileNode;

typedef struct fileGlobalNode {
	char * filePath;
	char * workerIp;
	int port;
	uint32_t sockfd;
	int filePathLength;
}fileGlobalNode;

void logDebug(char *);
void loadConfiguration();
void signalHandler(int);
void connectionHandler(int client_fd);
void pairingGlobalFiles(t_list *listToPair, char* resultName);
void pairingFiles();
void *createServer();
int connectToWorker(fileGlobalNode * worker);
int connectToFileSystem();

#endif /* WORKER_H_ */
