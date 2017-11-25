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
t_list * pruebasApareo;



int main() {
	char * logFile = tmpnam(NULL);
	logger = log_create(logFile, "WORKER", 1, LOG_LEVEL_DEBUG);
	loadConfiguration();
	if (signal(SIGUSR1, signalHandler) == SIG_ERR) {
			log_error(logger, "Couldn't register signal handler");
			return EXIT_FAILURE;
		}
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
				log_debug(logger, "Transformation Stage");
				ipc_struct_worker_start_transform_request request;

				recv(client_sock, &(request.scriptContentLength), sizeof(uint32_t), 0);
				request.scriptContent = malloc(request.scriptContentLength + 1);
				recv(client_sock, request.scriptContent, (request.scriptContentLength + 1) * sizeof(char), 0);

				char chmode[] = "0777";
			    int chmodNumber;
			    chmodNumber = strtol(chmode, 0, 8);
				char * scriptPathFormat = "/home/utnso/transformationScript%d";
				char *scriptPath = malloc(100);
				sprintf(scriptPath, scriptPathFormat, client_sock);
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

				recv(client_sock, &(request.block), sizeof(uint32_t), 0);

				recv(client_sock, &(request.usedBytes), sizeof(uint32_t), 0);
				recv(client_sock, &(request.tempFilePathLength), sizeof(uint32_t),0);
				request.tempFilePath = malloc(request.tempFilePathLength + 1);
				recv(client_sock, request.tempFilePath, ((request.tempFilePathLength * sizeof(char)) + 1), 0);

//				char *template = "head -c %li %s | tail -c %d | %s | sort > %s";
				char *template = "echo \"hello world\" | %s";

//				long int bytesToRead = (request.block * blockSize) + request.usedBytes;
				int templateSize = snprintf(NULL, 0, template, scriptPath);

//				int templateSize = snprintf(NULL, 0, template, bytesToRead, configuration.binPath, request.usedBytes, request.scriptContent, request.tempFilePath);
				char *buffer = malloc(templateSize + 1);
//				sprintf(buffer, template, bytesToRead, configuration.binPath, request.usedBytes, request.scriptContent, request.tempFilePath);
				sprintf(buffer, template, scriptPath);
				buffer[templateSize] = '\0';
				int checkCode = 0;

				checkCode = system(buffer);
				ipc_struct_worker_start_transform_response transform_response;
				if(checkCode == 127 || checkCode == -1){
					transform_response.succeeded = 0;
				}
				else{
					transform_response.succeeded = 1;
				}

//				fileNode *file = malloc (sizeof(fileNode));
//				file->filePath = malloc(request.tempFilePathLength);
//				memcpy(file->filePath, request.tempFilePath, request.tempFilePathLength);

				free(buffer);
//				free(file);
				free(request.scriptContent);
				free(request.tempFilePath);
				uint32_t response_operation = WORKER_START_TRANSFORM_RESPONSE;
				send(client_sock, &response_operation, sizeof(uint32_t), 0);
				send(client_sock, &(transform_response.succeeded), sizeof(int), 0);
				break;
			}
			case WORKER_START_LOCAL_REDUCTION_REQUEST:{
				log_debug(logger, "Local Reduction Stage");


				ipc_struct_worker_start_local_reduce_request request;
				recv(client_sock, &(request.transformTempEntriesCount), sizeof(uint32_t), 0);
				recv(client_sock, &(request.scriptContentLength), sizeof(uint32_t), 0);

				request.scriptContent = malloc(request.scriptContentLength + 1);
				recv(client_sock, request.scriptContent, (request.scriptContentLength + 1), 0);

				char chmode[] = "0777";
			    int chmodNumber;
			    chmodNumber = strtol(chmode, 0, 8);
				char * scriptPathFormat = "/home/utnso/localReductionScript%d";
				char *scriptPath = malloc(100);
				sprintf(scriptPath, scriptPathFormat, client_sock);
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
				t_list * fileList;
				for(i = 0; i < request.transformTempEntriesCount; i++ ){
					fileNode * fileToReduce = malloc(sizeof(fileNode));
					recv(client_sock, &(fileToReduce->filePathLength), sizeof(uint32_t), 0);
					fileToReduce->filePath = malloc(fileToReduce->filePathLength +1);
					recv(client_sock, fileToReduce->filePath, (fileToReduce->filePathLength + 1), 0);
					list_add(fileList, fileToReduce);
				}

				recv(client_sock, &(request.reduceTempPathLen), sizeof(uint32_t), 0);

				request.reduceTempPath = malloc(request.reduceTempPathLen +1);
				recv(client_sock, request.reduceTempPath, (request.reduceTempPathLen +1), 0);
				char * reduceTempFilePath = malloc(request.reduceTempPathLen +1);
				memcpy(reduceTempFilePath, request.reduceTempPath, request.reduceTempPathLen);
				reduceTempFilePath[request.reduceTempPathLen] = '\0';
				pairingFiles(fileList, reduceTempFilePath);

				char *template = "cat %s | %s > %s";
				int templateSize = snprintf(NULL, 0, template, request.reduceTempPath, scriptPath, request.reduceTempPath);
				char *buffer = malloc(templateSize + 1);
				sprintf(buffer, template, request.reduceTempPath, request.scriptContent, request.reduceTempPath);
				buffer[templateSize] = '\0';
				int checkCode = system(buffer);
				ipc_struct_worker_start_local_reduce_response reduction_response;
				if(checkCode == 127 || checkCode == -1){
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
				break;
			}
			case WORKER_START_GLOBAL_REDUCTION_REQUEST:{
				log_debug(logger, "Global Reduction Stage");
				t_list * workerList;
				int i = 0;
				uint32_t sockFs;
				ipc_struct_worker_start_global_reduce_request request;




				recv(client_sock, &(request.scriptContentLength), sizeof(uint32_t), 0);

				request.scriptContent = malloc(request.scriptContentLength + 1);
				recv(client_sock, request.scriptContent, (request.scriptContentLength +1), 0);

				char chmode[] = "0777";
			    int chmodNumber;
			    chmodNumber = strtol(chmode, 0, 8);
				char * scriptPathFormat = "/home/utnso/GlobalReductionScript%d";
				char *scriptPath = malloc(100);
				sprintf(scriptPath, scriptPathFormat, client_sock);
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

				if(checkCode == 127 || checkCode == -1){
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
				int fileResultSize = finalFileSize(fileFinalName);
				if(fileResultSize == -1){
					perror("File not found");
					reduction_response.succeeded = 0;
					send(client_sock, &(reduction_response.succeeded), sizeof(uint32_t), 0);
					break;
				}

				send(sockFs, &fileResultSize, sizeof(uint32_t), 0);
				request.globalTempPath[request.globalTempPathLen] = '\0';
				FILE * finalFile = fopen(request.globalTempPath, "r");
				int finalFileFd = fileno(finalFile);
				int bytesSent = sendfile(sockFs, finalFileFd, NULL, fileResultSize);
				if (bytesSent == fileResultSize){
					log_debug(logger, "File sent successfully");
				}
				else{
					perror("File couldn't be sent correctly");
				}
				free(fileFinalName);
				fclose(finalFile);
				free(buffer);
				free(scriptPath);
				free(request.globalTempPath);
				free(pairingResultName);
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

int finalFileSize(char *filePath) {
    struct stat st;

    if (stat(filePath, &st) == 0)
        return st.st_size;

    return -1;
}
