/*
 * worker.c
 *
 *  Created on: Sep 8, 2017
 *      Author: Agustin Coda
 *      Description: An exploited Worker
 */

// TODO: Recibir tabla de Workers Global Reduction desde Master
// TODO: Recibir tabla de Archivos en Local Reduction desde Master
// TODO: Testear worker con master
// TODO: Probar algoritmo de apareo global con otros workers para ver si anda bien los sends y recv






#include "worker.h"
#include "configWorker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <netinet/in.h>
#include <netdb.h>

#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <ipc/ipc.h>
#include <ipc/serialization.h>

#define SLAVE_WORKER 123456
#define OK 1
#define REGISTER_REQUEST 15
#define FILE_CLOSE_REQUEST 16

t_log *logger;
worker_configuration configuration;
t_list * fileList;
t_list * pruebasApareo;




int main() {
	char * logFile = tmpnam(NULL);
	logger = log_create(logFile, "WORKER", 1, LOG_LEVEL_DEBUG);
	loadConfiguration();
	if (signal(SIGUSR1, signalHandler) == SIG_ERR) {
			log_error(logger, "Couldn't register signal handler");
			return EXIT_FAILURE;
		}
	fileList = list_create();
	createServer();


	//Esto para test de apareo local
//	fileNode * testLocal1 = malloc(sizeof(fileNode));
//	fileNode * testLocal2 = malloc(sizeof(fileNode));
//	fileNode * testLocal3 = malloc(sizeof(fileNode));
//	fileNode * testLocal4 = malloc(sizeof(fileNode));
//	testLocal1->filePath = malloc (strlen("/home/utnso/LocalTest1"));
//	testLocal2->filePath = malloc (strlen("/home/utnso/LocalTest2"));
//	testLocal3->filePath = malloc (strlen("/home/utnso/LocalTest3"));
//	testLocal4->filePath = malloc (strlen("/home/utnso/LocalTest4"));
//	strcpy(testLocal1->filePath, "/home/utnso/LocalTest1");
//	strcpy(testLocal2->filePath, "/home/utnso/LocalTest2");
//	strcpy(testLocal3->filePath, "/home/utnso/LocalTest3");
//
//	strcpy(testLocal4->filePath, "/home/utnso/LocalTest4");
//	list_add(fileList, testLocal1);
//	list_add(fileList, testLocal2);
//	list_add(fileList, testLocal3);
//	list_add(fileList, testLocal4);
//	pairingFiles(fileList, "PairTest");

	return EXIT_SUCCESS;
}


void loadConfiguration() {
	configuration = fetchConfiguration("../conf/nodeConf.txt");
}

void signalHandler(int signo) {
	if (signo == SIGUSR1) {
		logDebug("SIGUSR1 - Reloading configuration");
		loadConfiguration();
	}
}

void logDebug(char *message) {
	log_debug(logger, message);
}

void *createServer() {
	int socket_desc, client_sock, c;
	struct sockaddr_in server, client;
	// Create socket
	int iSetOption = 1;
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char*)&iSetOption, sizeof(iSetOption));
	if (socket_desc == -1) {
		log_error(logger, "Couldn't create socket.");
	}
	log_debug(logger, "Socket correctly created");

	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(configuration.workerPort);

	// Bind
	if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
		log_error(logger, "Couldn't Bind.");
		printf("\n\n %d", errno);
		return NULL;
	}

	// Listen
	listen(socket_desc, 4);

	// Accept and incoming connection
	log_debug(logger, "Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);


	while ((client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t*) &c))) {
		log_debug(logger, "Connection accepted");
		connectionHandler(client_sock);

	}

	if (client_sock < 0) {
		log_error(logger, "Couldn't accept connection.");
		return NULL;
	}



	return 0;
}

void connectionHandler(int client_sock){
	pid_t pid;
	int status;

	const int blockSize = 1024*1024;

	if((pid=fork()) == 0){

		//Recibo toda la informacion necesaria para ejecutar las tareas
		uint32_t operation;
		recv(client_sock,&operation,sizeof(uint32_t), 0);
		switch(operation){
			case WORKER_START_TRANSFORM_REQUEST:{
				ipc_struct_worker_start_transform_request request;

				recv(client_sock, &(request.scriptContentSize), sizeof(uint32_t), 0);
				request.scriptContent = malloc(request.scriptContentSize);
				recv(client_sock, &(request.scriptContent), request.scriptContentSize, 0);

				recv(client_sock, &(request.block), sizeof(uint32_t), 0);

				recv(client_sock, &(request.usedBytes), sizeof(uint32_t), 0); //FIXME: (Fede) para qué se usa?

				recv(client_sock, &(request.tempFilePathLength), sizeof(uint32_t),0);
				request.tempFilePath = malloc(request.tempFilePathLength);
				recv(client_sock, request.tempFilePath, request.tempFilePathLength, 0);

				char *template = "head -c %li /home/utnso/data.bin | tail -c %d | %s | sort > %s";
				long int bytesToRead = (request.block * blockSize) + blockSize;
				int templateSize = snprintf(NULL, 0, template, bytesToRead, blockSize, request.scriptContent, request.tempFilePath);
				char *buffer = malloc(templateSize + 1);
				sprintf(buffer, template, bytesToRead, blockSize, request.scriptContent, request.tempFilePath);
				buffer[templateSize] = '\0';
				system(buffer);

				fileNode *file = malloc (sizeof(fileNode));
				file->filePath = malloc(request.tempFilePathLength);
				memcpy(file->filePath, request.tempFilePath, request.tempFilePathLength);
				list_add(fileList, file);

				free(buffer);
				free(file);
				free(request.scriptContent);
				free(request.tempFilePath);

				int checkCode = OK;
				send(client_sock, &checkCode, sizeof(int), 0);
				break;
			}
			case WORKER_START_LOCAL_REDUCTION_REQUEST:{
				t_list * filesList;
				fileNode * fileToReduce = malloc(sizeof(fileNode));
				//Aca deberia recibir la tabla de archivos del Master y ponerla en una lista
				list_add(fileList, fileToReduce);

				uint32_t scriptLength;
				recv(client_sock, &scriptLength, sizeof(uint32_t), 0);

				char *script = malloc(scriptLength);
				recv(client_sock, script, sizeof(scriptLength), 0);

				uint32_t temporalNameLength;
				recv(client_sock, &temporalNameLength, sizeof(uint32_t), 0);

				char *temporalName = malloc(temporalNameLength);
				recv(client_sock, temporalName, temporalNameLength, 0);

				pairingFiles(filesList, temporalName);

				char *template = "%s %s > %s";
				int templateSize = snprintf(NULL, 0, template, temporalName, script, temporalName);
				char *buffer = malloc(templateSize + 1);
				sprintf(buffer, template, temporalName, script, temporalName);
				buffer[templateSize] = '\0';
				system(buffer);

				free(buffer);
				free(script);
				free(temporalName);
				free(fileToReduce);

				int checkCode = OK;
				send(client_sock, &checkCode, sizeof(int), 0);
				break;
			}
			case WORKER_START_GLOBAL_REDUCTION_REQUEST:{
				fileGlobalNode * workerToRequest = malloc(sizeof(fileGlobalNode));
				t_list * workerList;
				int workerListSize, i = 0;

				//Aca deberia recibir la tabla de archivos del Master y ponerla en una lista

				list_add(workerList, workerToRequest);

				//Conexion a los workers
				workerListSize = list_size(workerList);
				for(i=0; i < workerListSize; i++){
					workerToRequest = list_get(workerList, i);
					workerToRequest->sockfd = connectToWorker(workerToRequest);

				}

				uint32_t scriptLength;
				recv(client_sock, &scriptLength, sizeof(uint32_t), 0);

				char *script = malloc(scriptLength);
				recv(client_sock, script, sizeof(scriptLength), 0);

				uint32_t temporalNameLength;
				recv(client_sock, &temporalNameLength, sizeof(uint32_t), 0);

				char *temporalName = malloc(temporalNameLength);
				recv(client_sock, temporalName, temporalNameLength, 0);

				pairingGlobalFiles(workerList, temporalName);

				char * template = "%s %s > %s";
				int templateSize = snprintf(NULL, 0, template, temporalName, script, temporalName);
				char *buffer = malloc(templateSize + 1);
				sprintf(buffer, template, temporalName, script, temporalName);
				buffer[templateSize] = '\0';
				system(buffer);

				free(workerToRequest);
				free(buffer);
				free(script);
				free(temporalName);

				int checkCode = OK;
				send(client_sock, &checkCode, sizeof(int), 0);
				break;
			}
			case SLAVE_WORKER:{
				int closeCode, registerSize = 0;
				int clientCode = 0;
				char * registerToSend = malloc(256);

				uint32_t scriptLength;
				recv(client_sock, &scriptLength, sizeof(uint32_t), 0);

				char *script = malloc(scriptLength);
				recv(client_sock, script, sizeof(scriptLength), 0);

				uint32_t temporalNameLength;
				recv(client_sock, &temporalNameLength, sizeof(int), 0);

				char *temporalName = malloc(temporalNameLength);
				recv(client_sock, temporalName, temporalNameLength, 0);

				FILE * fileToOpen;
				fileToOpen = fopen(temporalName, "r");
				if(fileToOpen == NULL){
					log_error(logger, "Couldn't open file");
					close(client_sock);
					break;
				}
				while(closeCode != FILE_CLOSE_REQUEST){
					recv(client_sock, &clientCode, sizeof(int), 0);
					if(clientCode == REGISTER_REQUEST){
						if(fgets(registerToSend, 256, fileToOpen) == NULL){
							strcpy(registerToSend, "NULL");
							send(client_sock, &registerSize, sizeof(int), 0);
							send(client_sock, registerToSend, registerSize, 0);
						}
						else{
							registerSize = strlen(registerToSend);
							send(client_sock, &registerSize, sizeof(int), 0);
							send(client_sock, registerToSend, registerSize, 0);
						}

					}
					recv(client_sock, &closeCode, sizeof(int), 0);
				}

				close(client_sock);
				free(registerToSend);
				fclose(fileToOpen);
				free(temporalName);
				break;
			}
			default:
				log_error(logger, "Operation couldn't be identified");
		}
	}
	else
	{
		close(client_sock);
		waitpid(pid, &status, 0);
	}

}


// Apareo de Archivos
void pairingFiles(t_list *listToPair, char* resultName){
	int i, lower, registerPosition = 0;
	int eofCounter = 0;
	char *lowerString = malloc(256);

	fileNode *fileToOpen;
	FILE *pairingResultFile;
	pairingResultFile = fopen(resultName, "w");
	int listSize = list_size(listToPair);
	char * fileRegister [listSize];
	FILE* filesArray[listSize];
	for(i=0; i < listSize; i++){
		fileToOpen = list_get(listToPair, i);
		filesArray[i]= fopen(fileToOpen->filePath, "r");
		fileRegister[i] = malloc(256);
		fgets(fileRegister[i], 256, filesArray[i]);
	}
	strcpy(lowerString, fileRegister[0]);
	FILE *registerFromFile = filesArray[0];
	i = 0;
	while(eofCounter < listSize){
		if(fileRegister[i] != NULL){
			lower = strcmp(lowerString, fileRegister[i]);
			if(lower > 0){
				strcpy(lowerString, fileRegister[i]);
				registerFromFile = filesArray[i];
				registerPosition = i;
			}
		}
		if(i == (listSize - 1)){
			fputs(lowerString, pairingResultFile);

			if (fgets(fileRegister[registerPosition], 256, registerFromFile) == NULL){
				eofCounter ++;
//				memset(fileRegister[registerPosition], 'z', 256);
				fileRegister[registerPosition] = NULL;
				memset(lowerString, 'z', 256);

			}
			else{
				strcpy(lowerString, fileRegister[registerPosition]);
			}
			i = 0;
		}
		else{
			i++;
		}
	}
	for(i=0; i < listSize; i++){
		fclose(filesArray[i]);
	}
	fclose(pairingResultFile);


}

void pairingGlobalFiles(t_list *listToPair, char* resultName){
	int i, lower, eofCounter, registerLength, registerPosition = 0;
	int requestCode = REGISTER_REQUEST;
	int closeCode = FILE_CLOSE_REQUEST;
	char * lowerString = malloc(256);

	FILE * pairingResultFile;
	pairingResultFile = fopen(resultName, "w");
	int listSize = list_size(listToPair);
	char * fileRegister [listSize];
	fileGlobalNode * fileToOpen;
	for(i=0; i<listSize; i++){
		fileToOpen = list_get(listToPair, i);
		fileRegister[i] = malloc(256);
		send(fileToOpen->sockfd, &requestCode, sizeof(int), 0);
		recv(fileToOpen->sockfd, &registerLength, sizeof(int), 0);
		recv(fileToOpen->sockfd, fileRegister[i], registerLength, 0);
	}
	fileGlobalNode * workerToRequest;
	i = 0;

	while(eofCounter < listSize){
		if(fileRegister[i] != NULL){
			lower = strcmp(lowerString, fileRegister[i]);
			if(lower > 0){
				strcpy(lowerString, fileRegister[i]);
				workerToRequest = list_get(listToPair, i);
				registerPosition = i;
			}
		}
		if(i == (listSize - 1)){
			fputs(lowerString, pairingResultFile);
			send(workerToRequest->sockfd, &requestCode, sizeof(int), 0);
			recv(workerToRequest->sockfd, &registerLength, sizeof(int), 0);
			recv(workerToRequest->sockfd, fileRegister[registerPosition], registerLength, 0);
			if (strcmp(fileRegister[registerPosition], "NULL") == 0){
				eofCounter ++;
				//				memset(fileRegister[registerPosition], 'z', 256);
				fileRegister[registerPosition] = NULL;
				memset(lowerString, 'z', 256);

			}
			else{
				strcpy(lowerString, fileRegister[registerPosition]);
			}
			i = 0;
		}
		else{
			i++;
		}
	}
		for(i=0; i < listSize; i++){
			fileToOpen = list_get(listToPair, i);
			send(fileToOpen->sockfd, &closeCode, sizeof(int), 0);
			free(fileRegister[i]);
		}
	fclose(pairingResultFile);
	free(lowerString);




}


int connectToWorker(fileGlobalNode * worker){
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	printf("\nWorker Ip: %s Port No: %d", worker->workerIp, worker->port);
	printf("\nConnecting to Worker\n");

	/* Create a socket point */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	server = gethostbyname(worker->workerIp);
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(worker->port);

	/* Now connect to the server */
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		exit(1);
	}
	return sockfd;
}



