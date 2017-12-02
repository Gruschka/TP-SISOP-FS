/*
 * worker.c
 *
 *  Created on: Sep 8, 2017
 *      Author: Agustin Coda
 *      Description: An exploited Worker
 */

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
#include <sys/stat.h>
#include <sys/sendfile.h>

#include <ipc/ipc.h>
#include <ipc/serialization.h>

#define SLAVE_WORKER 123456
#define OK 1
#define REGISTER_REQUEST 15
#define FILE_CLOSE_REQUEST 16

t_log *logger;
worker_configuration configuration;


int main() {
	char * logFile = "/home/utnso/logFile";
	logger = log_create(logFile, "WORKER", 1, LOG_LEVEL_DEBUG);
	loadConfiguration();
	if (signal(SIGUSR1, signalHandler) == SIG_ERR) {
			log_error(logger, "Couldn't register signal handler");
			return EXIT_FAILURE;
		}
	createServer();

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
	listen(socket_desc, SOMAXCONN);

	// Accept and incoming connection
	log_debug(logger, "Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);


	while ((client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t*) &c))) {
		log_debug(logger, "Connection accepted");
		connectionHandler(client_sock);
	}
//	connectionHandler(5);

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

//		operation = WORKER_START_TRANSFORM_REQUEST;

		recv(client_sock,&operation,sizeof(uint32_t), 0);
		switch(operation){
			case WORKER_START_TRANSFORM_REQUEST:{
				log_debug(logger, "Transformation Stage");

				ipc_struct_worker_start_transform_request request;

//				//Valores Hardcoded
//				request.block = 3;
//				request.tempFilePath = "/home/utnso/resultadoHardcoded";
//				request.usedBytes = 10000;

				recv(client_sock, &(request.scriptContentLength), sizeof(uint32_t), 0);
				request.scriptContent = malloc(request.scriptContentLength + 1);
				recv(client_sock, request.scriptContent, (request.scriptContentLength + 1) * sizeof(char), 0);
				recv(client_sock, &(request.block), sizeof(uint32_t), 0);
				recv(client_sock, &(request.usedBytes), sizeof(uint32_t), 0);
				recv(client_sock, &(request.tempFilePathLength), sizeof(uint32_t),0);
				request.tempFilePath = malloc(request.tempFilePathLength + 1);
				recv(client_sock, request.tempFilePath, ((request.tempFilePathLength * sizeof(char)) + 1), 0);

				char chmode[] = "0777";
			    int chmodNumber;
			    chmodNumber = strtol(chmode, 0, 8);
				char * scriptPath = scriptTempFileName();
				FILE *scriptFile = fopen(scriptPath, "w");
				if (scriptFile == NULL) {
					exit(-1); //fixme: ola q ace
				}
				fprintf(scriptFile, strdup(request.scriptContent), "");
				fclose(scriptFile);
				if (chmod(scriptPath, chmodNumber)){
					perror("Permissions couldn't be given");
					break;
				}

				char *template = "head -c %li %s | tail -c %d | %s | sort > %s";

				long int bytesToRead = (request.block * blockSize) + request.usedBytes;
				int templateSize = snprintf(NULL, 0, template, bytesToRead, configuration.binPath, request.usedBytes, scriptPath, request.tempFilePath);
				char *buffer = malloc(templateSize + 1);
				sprintf(buffer, template, bytesToRead, configuration.binPath, request.usedBytes, scriptPath, request.tempFilePath);
				buffer[templateSize] = '\0';

				int checkCode = 0;
				log_debug(logger, "\n Transforming Block: %d \n", request.block);
				log_debug(logger, "\n %s \n", buffer);
				checkCode = system(buffer);

				ipc_struct_worker_start_transform_response transform_response;
				if(checkCode!= 0){
					transform_response.succeeded = 0;
				}
				else{
					transform_response.succeeded = 1;
				}


				free(buffer);
				free(request.scriptContent);
				free(request.tempFilePath);
				uint32_t response_operation = WORKER_START_TRANSFORM_RESPONSE;
				send(client_sock, &response_operation, sizeof(uint32_t), 0);
				send(client_sock, &(transform_response.succeeded), sizeof(int), 0);
				remove(scriptPath);
				free(scriptPath);
				break;
			}
			case WORKER_START_LOCAL_REDUCTION_REQUEST:{

				log_debug(logger, "Local Reduction Stage \n");


				ipc_struct_worker_start_local_reduce_request request;
				recv(client_sock, &(request.scriptContentLength), sizeof(uint32_t), 0);

				request.scriptContent = malloc(request.scriptContentLength + 1);
				recv(client_sock, request.scriptContent, (request.scriptContentLength + 1), 0);
				char chmode[] = "0777";
			    int chmodNumber;
			    chmodNumber = strtol(chmode, 0, 8);
				char * scriptPath = scriptTempFileName();
				log_debug(logger, "\n %s \n", scriptPath);
				FILE *scriptFile = fopen(scriptPath, "w");
				if (scriptFile == NULL) {
					exit(-1); //fixme: ola q ace
				}
				fprintf(scriptFile, strdup(request.scriptContent), "");
//				fwrite(request.scriptContent, sizeof(char), request.scriptContentLength, scriptFile);
				fclose(scriptFile);
				if (chmod(scriptPath, chmodNumber)){
					perror("Permissions couldn't be given");
					break;
				}
				int i =0;//Aca deberia recibir la tabla de archivos del Master y ponerla en una lista
				t_list * fileList = list_create();
				recv(client_sock, &(request.transformTempEntriesCount), sizeof(uint32_t), 0);
				for(i = 0; i < request.transformTempEntriesCount; i++ ){
					log_debug(logger, "Estoy adentro del for y el entry es %d \n", request.transformTempEntriesCount);
					fileNode * fileToReduce = malloc(sizeof(fileNode));
					recv(client_sock, &(fileToReduce->filePathLength), sizeof(uint32_t), 0);
					fileToReduce->filePath = malloc(fileToReduce->filePathLength +1);
					recv(client_sock, fileToReduce->filePath, (fileToReduce->filePathLength + 1), 0);
					log_debug(logger, "El path es %s ", fileToReduce->filePath);
					list_add(fileList, fileToReduce);
				}

				recv(client_sock, &(request.reduceTempPathLen), sizeof(uint32_t), 0);

				request.reduceTempPath = malloc(request.reduceTempPathLen +1);
				recv(client_sock, request.reduceTempPath, (request.reduceTempPathLen +1), 0);
				char * reduceTempFilePath = malloc(request.reduceTempPathLen +1);
				memcpy(reduceTempFilePath, request.reduceTempPath, request.reduceTempPathLen);
				reduceTempFilePath[request.reduceTempPathLen] = '\0';
				log_debug(logger, "tempFile: %s", reduceTempFilePath);
				char * pairingResult = scriptTempFileName();
				log_debug(logger, "\n El resultado del apareo se guarda aca : %s \n", pairingResult);
				pairingFiles(fileList, pairingResult);

				char *template = "cat %s | %s > %s";
				int templateSize = snprintf(NULL, 0, template, pairingResult, scriptPath, request.reduceTempPath);
				char *buffer = malloc(templateSize + 1);
				sprintf(buffer, template, pairingResult, scriptPath, request.reduceTempPath);
				buffer[templateSize] = '\0';
				log_debug(logger, "\n %s \n", buffer);
				int checkCode = system(buffer);
				ipc_struct_worker_start_local_reduce_response reduction_response;
				if(checkCode != 0){
					reduction_response.succeeded = 0;
				}
				else{
					reduction_response.succeeded = 1;
				}
				free(buffer);
				free(request.scriptContent);
				free(request.reduceTempPath);
				for (i=0; i < request.transformTempEntriesCount; i++){
					fileNode * fileToReduce;
					fileToReduce = list_get(fileList, i);
					free(fileToReduce->filePath);
					free(fileToReduce);
				}
				list_destroy(fileList);
				uint32_t response_operation = WORKER_START_LOCAL_REDUCTION_RESPONSE;
				send(client_sock, &response_operation, sizeof(uint32_t), 0);
				send(client_sock, &(reduction_response.succeeded), sizeof(int), 0);
				free(reduceTempFilePath);
				//remove(pairingResult);
				remove(scriptPath);
				free(scriptPath);
				break;
			}
			case WORKER_START_GLOBAL_REDUCTION_REQUEST:{
				log_debug(logger, "Global Reduction Stage");
				t_list * workerList = list_create();
				int i = 0;
				uint32_t sockFs;
				ipc_struct_worker_start_global_reduce_request request;
				recv(client_sock, &(request.scriptContentLength), sizeof(uint32_t), 0);

				request.scriptContent = malloc(request.scriptContentLength + 1);
				recv(client_sock, request.scriptContent, (request.scriptContentLength +1), 0);

				char chmode[] = "0777";
			    int chmodNumber;
			    chmodNumber = strtol(chmode, 0, 8);
				char * scriptPath = scriptTempFileName();
				FILE *scriptFile = fopen(scriptPath, "w");
				if (scriptFile == NULL) {
					exit(-1); //fixme: ola q ace
				}
				fprintf(scriptFile, strdup(request.scriptContent), "");
//				fwrite(request.scriptContent, sizeof(char), request.scriptContentLength, scriptFile);
				fclose(scriptFile);
				if (chmod(scriptPath, chmodNumber)){
					perror("Permissions couldn't be given");
					break;
				}
				recv(client_sock, &(request.workersEntriesCount), sizeof(uint32_t),0);
				ipc_struct_worker_start_global_reduce_response reduction_response;
				//Recepcion y conexion a los workers
				for(i=0; i < request.workersEntriesCount; i++){
					fileGlobalNode * workerToRequest = malloc(sizeof(fileGlobalNode));
					recv(client_sock, &(workerToRequest->workerNameLength), sizeof(uint32_t),0);
					workerToRequest->workerName = malloc(workerToRequest->workerNameLength + 1);
					recv(client_sock, workerToRequest->workerName, (workerToRequest->workerNameLength + 1), 0);
					workerToRequest->workerName[workerToRequest->workerNameLength] = '\0';
					recv(client_sock, &(workerToRequest->workerIpLen), sizeof(uint32_t),0);
					workerToRequest->workerIp = malloc(workerToRequest->workerIpLen + 1);
					recv(client_sock, workerToRequest->workerIp, (workerToRequest->workerIpLen + 1), 0);
					recv(client_sock, &(workerToRequest->port), sizeof(uint32_t), 0);
					recv(client_sock, &(workerToRequest->filePathLength), sizeof(uint32_t), 0);
					workerToRequest->filePath = malloc(workerToRequest->filePathLength + 1);
					recv(client_sock, workerToRequest->filePath, (workerToRequest->filePathLength + 1), 0);
					workerToRequest->filePath[workerToRequest->filePathLength] = '\0';
					workerToRequest->sockfd = connectToWorker(workerToRequest->workerIp, workerToRequest->port);
					if(workerToRequest->sockfd == -1){
						reduction_response.succeeded = 0;
						send(client_sock, &(reduction_response.succeeded), sizeof(uint32_t), 0);
						break;
					}
					list_add(workerList, workerToRequest);
					send(workerToRequest->sockfd, &(workerToRequest->filePathLength), sizeof(int),0);
					workerToRequest->filePath = malloc(workerToRequest->filePathLength + 1);
					send(workerToRequest->sockfd, workerToRequest->filePath, workerToRequest->filePathLength,0);

				}
				recv(client_sock, &(request.globalTempPathLen), sizeof(uint32_t), 0);

				request.globalTempPath = malloc(request.globalTempPathLen + 1);
				recv(client_sock, request.globalTempPath, (request.globalTempPathLen + 1), 0);
				char * pairingResultName = malloc(request.globalTempPathLen +1);
				memcpy(pairingResultName, request.globalTempPath, request.globalTempPathLen);
				pairingResultName[request.globalTempPathLen] = '\0';
				pairingGlobalFiles(workerList, pairingResultName);

				char * template = "cat %s | %s > %s";
				int templateSize = snprintf(NULL, 0, template, request.globalTempPath, scriptPath, request.globalTempPath);
				char *buffer = malloc(templateSize + 1);
				sprintf(buffer, template, request.globalTempPath, scriptPath, request.globalTempPath);
				buffer[templateSize] = '\0';

				int checkCode = system(buffer);

				if(checkCode != 0){
					reduction_response.succeeded = 0;
				}
				else{
					reduction_response.succeeded = 1;
				}

				uint32_t response_operation = WORKER_START_GLOBAL_REDUCTION_RESPONSE;
				send(client_sock, &response_operation, sizeof(int), 0);
				send(client_sock, &(reduction_response), sizeof(uint32_t), 0);
				for (i = 0; i < request.workersEntriesCount; i++){
					fileGlobalNode * workerToFree;
					workerToFree = list_get(workerList, i);
					free(workerToFree->filePath);
					free(workerToFree->workerIp);
					free(workerToFree->workerName);
					free(workerToFree);
				}
				list_destroy(workerList);
				//Enviar archivo al Filesystem
				uint32_t fileFinalNameLength = 0;
				recv(client_sock, &fileFinalNameLength, sizeof(uint32_t), 0);
				char * fileFinalName = malloc(fileFinalNameLength +1);
				recv(client_sock, fileFinalName, fileFinalNameLength, 0);
				sockFs = connectToFileSystem();
				char *fileContent = worker_utils_readFile(fileFinalName);
				ipc_struct_worker_file_to_yama *sendFile = malloc(sizeof(ipc_struct_worker_file_to_yama));
				sendFile->file = fileContent;
				sendFile->pathName = fileFinalName;
				ipc_sendMessage(sockFs, WORKER_SEND_FILE_TO_YAMA, fileContent);
				free(buffer);
				free(fileContent);
				free(sendFile);
				free(request.globalTempPath);
				free(pairingResultName);
				remove(scriptPath);
				free(scriptPath);
				break;
			}
			case SLAVE_WORKER:{
				log_debug(logger, "Slave Worker Stage");
				int closeCode, registerSize = 0;
				int clientCode = 0;
				char * registerToSend = malloc(256);
//
//				uint32_t scriptLength;
//				recv(client_sock, &scriptLength, sizeof(uint32_t), 0);
//
//				char *script = malloc(scriptLength);
//				recv(client_sock, script, sizeof(scriptLength), 0);
//
				uint32_t temporalNameLength;
				recv(client_sock, &temporalNameLength, sizeof(int), 0);

				char *temporalName = malloc(temporalNameLength + 1);
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
							registerSize = strlen(registerToSend);
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
	int maxLineSize = 1024 * 1024;

	int filesCount = list_size(listToPair);
	char *filesCursors[filesCount];
	FILE *filesArray[filesCount];
	int i;
	for(i = 0; i < filesCount; i++){
		fileNode *fileToOpen = list_get(listToPair, i);
		filesArray[i] = fopen(fileToOpen->filePath, "r");
		filesCursors[i] = malloc(maxLineSize);
		fgets(filesCursors[i], maxLineSize, filesArray[i]);
	}

	char *lowerString = malloc(maxLineSize);
	FILE *pairingResultFile = fopen(resultName, "w");

	int pairedFilesCount = 0;
	while (pairedFilesCount < filesCount) {
		memset(lowerString, 255, maxLineSize);

		int fileIndex;

		for (i = 0; i < filesCount; i++) {
			if (filesCursors[i] != NULL && strcmp(lowerString, filesCursors[i]) > 0) {
				strcpy(lowerString, filesCursors[i]);
				fileIndex = i;
			}
		}

		fputs(lowerString, pairingResultFile);

		if (fgets(filesCursors[fileIndex], maxLineSize, filesArray[fileIndex]) == NULL) {
			filesCursors[fileIndex] = NULL;
			pairedFilesCount++;
		}
	}

	for (i = 0; i < filesCount; i++) {
		free(filesCursors[i]);
		fclose(filesArray[i]);
	}

	fclose(pairingResultFile);
	free(lowerString);
}

void pairingGlobalFiles(t_list *listToPair, char* resultName){
	int i, lower, eofCounter, registerLength, registerPosition = 0;
	int maxLineSize = 1024 * 1024;
	int requestCode = REGISTER_REQUEST;
	int closeCode = FILE_CLOSE_REQUEST;
	char * lowerString = malloc(maxLineSize);

	FILE * pairingResultFile;
	pairingResultFile = fopen(resultName, "w");
	int listSize = list_size(listToPair);
	char * fileRegister [listSize];
	fileGlobalNode * fileToOpen;
	for(i=0; i<listSize; i++){
		fileToOpen = list_get(listToPair, i);
		fileRegister[i] = malloc(maxLineSize);
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
				fileRegister[registerPosition] = NULL;
				memset(lowerString, 255, maxLineSize);

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


int connectToWorker(char *workerIp, int port){
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	printf("\nWorker Ip: %s Port No: %d", workerIp, port);
	printf("\nConnecting to Worker\n");

	/* Create a socket point */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	server = gethostbyname(workerIp);
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	/* Now connect to the server */
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		sockfd = -1;
		return sockfd;
	}
	return sockfd;
}

int connectToFileSystem(){
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	printf("\nFileSystem Ip: %s Port No: %d", configuration.filesystemIP, configuration.filesystemPort);
	printf("\nConnecting to FileSystem\n");

	/* Create a socket point */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	server = gethostbyname(configuration.filesystemIP);
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(configuration.filesystemPort);

	/* Now connect to the server */
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		exit(1);
	}
	return sockfd;
}

char *worker_utils_readFile(char *path) {
	FILE *fp;
	long lSize;
	char *buffer;

	fp = fopen (path, "rb" );
	if( !fp ) perror(path),exit(1);

	fseek( fp , 0L , SEEK_END);
	lSize = ftell( fp );
	rewind( fp );

	/* allocate memory for entire content */
	buffer = calloc( 1, lSize+1 );
	if( !buffer ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);

	/* copy the file into the buffer // AND HANDSHAKE */
	if( 1!=fread( buffer , lSize, 1 , fp) )
	  fclose(fp),free(buffer),fputs("entire read fails",stderr),exit(1);

	fclose(fp);

	return buffer;
}

char *scriptTempFileName() {
	static char template[] = "/tmp/XXXXXX";
	char *name = malloc(strlen(template) + 1);
	strcpy(name, template);
	mktemp(name);

	return name;
}

