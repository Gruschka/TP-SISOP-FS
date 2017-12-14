/*
 * worker.c
 *
 *  Created on: Sep 8, 2017
 *      Author: Agustin Coda
 *      Description: An exploited Worker
 */

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

#define WORKER_REQUEST_FILE_FROM_SLAVE 123456
#define OK 123
#define REGISTER_REQUEST 987
#define FILE_CLOSE_REQUEST 11234

t_log *logFileNodo;
worker_configuration configuration;

int worker_runCommand(char *command) {
	int pid = fork();
	if (pid == 0) {
		int result = execl("/bin/bash", "sh", "-c", command, NULL);
		exit(result);
	}

	int status;
	waitpid(pid, &status, 0);
	return status;
}

int main(int argc, char **argv) {
	char * logFile = "/home/utnso/logFile";
	logFileNodo = log_create(logFile, "WORKER", 1, LOG_LEVEL_DEBUG);

	loadConfiguration(argc > 1 ? argv[1] : NULL);

	// Inicializo IPC de Hernie
	serialization_initialize();

	if (signal(SIGUSR1, signalHandler) == SIG_ERR) {
		log_error(logFileNodo, "Couldn't register signal handler");
		return EXIT_FAILURE;
	}

	createServer();

	return EXIT_SUCCESS;
}

void loadConfiguration(char *configFile) {
	configuration = fetchConfiguration(configFile != NULL ? configFile : "../conf/nodeConf.txt");
}

void signalHandler(int signo) {
	if (signo == SIGUSR1) {
		logDebug("SIGUSR1 - Reloading configuration");
		loadConfiguration(NULL);
	}
}

void logDebug(char *message) {
	log_debug(logFileNodo, message);
}

void *createServer() {
	// Create socket
	int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc < 0) {
		log_error(logFileNodo, "Couldn't create socket.");
		exit(EXIT_FAILURE);
	}
	log_debug(logFileNodo, "Socket correctly created");

	int iSetOption = 1;
	setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char*)&iSetOption, sizeof(iSetOption));

	// Prepare the sockaddr_in structure
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(configuration.workerPort);

	// Bind
	if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
		log_error(logFileNodo, "Couldn't Bind.");
		printf("\n\n %d", errno);
		exit(EXIT_FAILURE);
	}

	// Listen
	listen(socket_desc, SOMAXCONN);

	// Accept and incoming connection
	log_debug(logFileNodo, "Waiting for incoming connections...");
	int c = sizeof(struct sockaddr_in);
	struct sockaddr_in client;

	while (1) {
		int client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		if (client_sock >= 0) {
			log_debug(logFileNodo, "Connection accepted");
			connectionHandler(client_sock);
		} else {
			log_error(logFileNodo, "Couldn't accept connection.");
		}
	}

	return NULL;
}

void connectionHandler(int client_sock){
	const int blockSize = 1024*1024;

	pid_t pid = fork();
	if (pid == 0) {
		//Recibo toda la informacion necesaria para ejecutar las tareas
		uint32_t operation;
		recv(client_sock,&operation,sizeof(uint32_t), 0);
		switch (operation) {
		case WORKER_START_TRANSFORM_REQUEST:{
			log_debug(logFileNodo, "Transformation Stage");

			ipc_struct_worker_start_transform_request request;

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

			char *template = "export LC_ALL=C; head -c %li %s | tail -c %d | %s | sort > %s";

			long int bytesToRead = (request.block * blockSize) + request.usedBytes;
			int templateSize = snprintf(NULL, 0, template, bytesToRead, configuration.binPath, request.usedBytes, scriptPath, request.tempFilePath);
			char *buffer = malloc(templateSize + 1);
			sprintf(buffer, template, bytesToRead, configuration.binPath, request.usedBytes, scriptPath, request.tempFilePath);
			buffer[templateSize] = '\0';

			log_debug(logFileNodo, "\n Transforming Block: %d \n", request.block);
			log_debug(logFileNodo, "\n %s \n", buffer);
			int checkCode = worker_runCommand(buffer);
			ipc_struct_worker_start_transform_response transform_response;
			if(checkCode!= 0){
				log_debug(logFileNodo, "FAIL \n");
				transform_response.succeeded = 0;
			}
			else{
				log_debug(logFileNodo, "SUCCESS \n");
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
			log_debug(logFileNodo, "Local Reduction Stage \n");

			ipc_struct_worker_start_local_reduce_request request;
			recv(client_sock, &(request.scriptContentLength), sizeof(uint32_t), 0);

			request.scriptContent = malloc(request.scriptContentLength + 1);
			recv(client_sock, request.scriptContent, (request.scriptContentLength + 1), 0);
			char chmode[] = "0777";
			int chmodNumber;
			chmodNumber = strtol(chmode, 0, 8);
			char * scriptPath = scriptTempFileName();
			log_debug(logFileNodo, "\n %s \n", scriptPath);
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
			int i =0;//Aca deberia recibir la tabla de archivos del Master y ponerla en una lista
			t_list * fileList = list_create();
			recv(client_sock, &(request.transformTempEntriesCount), sizeof(uint32_t), 0);
			for(i = 0; i < request.transformTempEntriesCount; i++ ){
				//log_debug(logger, "Estoy adentro del for y el entry es %d \n", request.transformTempEntriesCount);
				fileNode * fileToReduce = malloc(sizeof(fileNode));
				recv(client_sock, &(fileToReduce->filePathLength), sizeof(uint32_t), 0);
				fileToReduce->filePath = malloc(fileToReduce->filePathLength +1);
				recv(client_sock, fileToReduce->filePath, (fileToReduce->filePathLength + 1), 0);
				list_add(fileList, fileToReduce);
			}

			recv(client_sock, &(request.reduceTempPathLen), sizeof(uint32_t), 0);

			request.reduceTempPath = malloc(request.reduceTempPathLen +1);
			recv(client_sock, request.reduceTempPath, (request.reduceTempPathLen +1), 0);
			log_debug(logFileNodo, "tempFile: %s", request.reduceTempPath);
			char * pairingResult = scriptTempFileName();
			log_debug(logFileNodo, "El resultado del apareo se guarda aca : %s \n", pairingResult);
			pairFiles(fileList, pairingResult);

			char *template = "cat %s | %s > %s";
			int templateSize = snprintf(NULL, 0, template, pairingResult, scriptPath, request.reduceTempPath);
			char *buffer = malloc(templateSize + 1);
			sprintf(buffer, template, pairingResult, scriptPath, request.reduceTempPath);
			buffer[templateSize] = '\0';
			log_debug(logFileNodo, "\n %s \n", buffer);
			int checkCode = worker_runCommand(buffer);
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
			remove(scriptPath);
			free(scriptPath);
			free(pairingResult);
			break;
		}
		case WORKER_START_GLOBAL_REDUCTION_REQUEST:{
			log_debug(logFileNodo, "Global Reduction Stage");
			int i = 0;

			ipc_struct_worker_start_global_reduce_request request;
			recv(client_sock, &(request.scriptContentLength), sizeof(uint32_t), 0);
			request.scriptContent = malloc(request.scriptContentLength + 1);
			recv(client_sock, request.scriptContent, (request.scriptContentLength +1), 0);

			char chmode[] = "0777";
			int chmodNumber;
			chmodNumber = strtol(chmode, 0, 8);
			char * scriptPath = scriptTempFileName();
			log_debug(logFileNodo, "scriptPath: %s \n", scriptPath);
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
			recv(client_sock, &(request.workersEntriesCount), sizeof(uint32_t),0);
			ipc_struct_worker_start_global_reduce_response reduction_response;
			//Recepcion y conexion a los workers
			t_list * workerList = list_create();
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
				workerToRequest->sockfd = ipc_createAndConnect(workerToRequest->port, workerToRequest->workerIp);
				log_debug(logFileNodo, "workerID: %s sockFD: %d \n", workerToRequest->workerName, workerToRequest->sockfd);
				if(workerToRequest->sockfd == -1){
					log_debug(logFileNodo, "Failed connecting to a Worker. Aborting!");
					reduction_response.succeeded = 0;
					send(client_sock, &(reduction_response.succeeded), sizeof(uint32_t), 0);
					break;
				}
				list_add(workerList, workerToRequest);
			}
			recv(client_sock, &(request.globalTempPathLen), sizeof(uint32_t), 0);

			request.globalTempPath = malloc(request.globalTempPathLen + 1);
			recv(client_sock, request.globalTempPath, (request.globalTempPathLen + 1), 0);

			char * pairingResult = scriptTempFileName();
			log_debug(logFileNodo, "\n The Global Pairing Result is saved at : %s \n", pairingResult);

			pairGlobalFiles(workerList, pairingResult);

			char * template = "cat %s | %s > %s";
			log_debug(logFileNodo, "temp: %s, script: %s , result: %s \n", pairingResult, scriptPath, request.globalTempPath);
			int templateSize = snprintf(NULL, 0, template, pairingResult, scriptPath, request.globalTempPath);
			char *buffer = malloc(templateSize + 1);
			sprintf(buffer, template, pairingResult, scriptPath, request.globalTempPath);
			buffer[templateSize] = '\0';

			int checkCode = worker_runCommand(buffer);
			log_debug(logFileNodo, "%s \n", buffer);

			if(checkCode != 0){
				reduction_response.succeeded = 0;
			}
			else{
				reduction_response.succeeded = 1;
			}

			uint32_t response_operation = WORKER_START_GLOBAL_REDUCTION_RESPONSE;
			send(client_sock, &response_operation, sizeof(int), 0);
			send(client_sock, &(reduction_response), sizeof(uint32_t), 0);

			// Libero memoria que utilizo la lista de workers
			for (i = 0; i < request.workersEntriesCount; i++){
				fileGlobalNode * workerToFree;
				workerToFree = list_get(workerList, i);
				free(workerToFree->filePath);
				free(workerToFree->workerIp);
				free(workerToFree->workerName);
				free(workerToFree);
			}
			list_destroy(workerList);

			free(buffer); // Template de System
			free(request.globalTempPath);
			free(pairingResult);
			remove(scriptPath);
			free(scriptPath);
			break;
		}
		case WORKER_START_FINAL_STORAGE_REQUEST: {
			ipc_struct_worker_start_final_storage_request request;

			recv(client_sock, &(request.globalTempPathLen), sizeof(uint32_t), 0);

			request.globalTempPath = malloc(request.globalTempPathLen + 1);
			recv(client_sock, request.globalTempPath, request.globalTempPathLen + 1, 0);

			recv(client_sock, &(request.finalResultPathLen), sizeof(uint32_t), 0);

			request.finalResultPath = malloc(request.finalResultPathLen + 1);
			recv(client_sock, request.finalResultPath, request.finalResultPathLen + 1, 0);

			uint32_t sockFs = ipc_createAndConnect(configuration.filesystemPort, configuration.filesystemIP);

			//handshake con FS
			ipc_struct_worker_handshake_to_fs handshake;
			handshake.status = 1; // ezto siknifik q soi 1 werker are jajjajaja3
			ipc_sendMessage(sockFs,WORKER_HANDSHAKE_TO_FS,&handshake);

			ipc_struct_worker_handshake_to_fs *handshakeResponse = ipc_recvMessage(sockFs,WORKER_HANDSHAKE_TO_FS);
			if(!handshakeResponse->status){
				// el failed syistem me ah rechasad0
				// todo: bengarse
				free(handshakeResponse);
			}else{
				free(handshakeResponse);
			}

			// Enviar archivo al file system
			ipc_struct_worker_file_to_fs file;
			file.pathName = request.finalResultPath;
			file.content = worker_utils_readFile(request.globalTempPath);

			ipc_sendMessage(sockFs, WORKER_SEND_FILE_TO_FS, &file);

			uint32_t op = WORKER_START_FINAL_STORAGE_RESPONSE;
			send(client_sock, &op, sizeof(uint32_t), 0);

			uint32_t succeeded = 1;
			send(client_sock, &succeeded, sizeof(uint32_t), 0);

			free(file.content);
			free(file.pathName);
		} break;
		case WORKER_REQUEST_FILE_FROM_SLAVE:{
			log_debug(logFileNodo, "Slave Worker Stage \n");

			uint32_t temporalNameLength;
			recv(client_sock, &temporalNameLength, sizeof(int), 0);

			char *temporalName = malloc(temporalNameLength + 1);
			recv(client_sock, temporalName, temporalNameLength + 1, 0);

			log_debug(logFileNodo, "The File is: %s \n", temporalName);

			char *fileContent = worker_utils_readFile(temporalName);

			int fileSize = strlen(fileContent);
			send(client_sock, &fileSize, sizeof(int), 0);

			send(client_sock, fileContent, (fileSize + 1), 0);
			free(temporalName);
			break;
		}
		default: {
			log_error(logFileNodo, "Operation couldn't be identified");
			break;
		}
		}

		log_debug(logFileNodo, "Finishing fork with pid = %d.", getpid());
		close(client_sock);
		exit(EXIT_SUCCESS);
	} else if (pid > 0) {
		log_debug(logFileNodo, "Created fork with pid = %d.", pid);
		close(client_sock);
	} else {
		log_error(logFileNodo, "Fork creation failed.");
		close(client_sock);
	}

}

// Apareo de Archivos
void pairFiles(t_list *listToPair, char *resultFilePath){
	int maxLineSize = 1024 * 1024;

	int filesCount = list_size(listToPair);
	char *filesCursors[filesCount];
	FILE *files[filesCount];

	int i;
	for(i = 0; i < filesCount; i++){
		fileNode *fileToOpen = list_get(listToPair, i);
		files[i] = fopen(fileToOpen->filePath, "r");
		filesCursors[i] = malloc(maxLineSize);
		fgets(filesCursors[i], maxLineSize, files[i]);
	}

	FILE *resultFile = fopen(resultFilePath, "w");

	int pairedFilesCount = 0;
	int auxFileIndex = 0;
	while (pairedFilesCount < filesCount) {
		for (i = 0; i < filesCount; i++) {
			if (filesCursors[auxFileIndex] == NULL || (filesCursors[i] != NULL && strcmp(filesCursors[auxFileIndex], filesCursors[i]) > 0)) {
				auxFileIndex = i;
			}
		}

		if (filesCursors[auxFileIndex] != NULL) {
			fprintf(resultFile, "%s", filesCursors[auxFileIndex]);
			fflush(resultFile);
		}

		if (fgets(filesCursors[auxFileIndex], maxLineSize, files[auxFileIndex]) == NULL) {
			free(filesCursors[auxFileIndex]);
			filesCursors[auxFileIndex] = NULL;
			fclose(files[auxFileIndex]);
			pairedFilesCount++;
		}
	}

	fclose(resultFile);
}

void pairGlobalFiles(t_list *workerList, char *resultFilePath) {
	t_list *localFiles = list_create();

	int i;
	for (i = 0; i < workerList->elements_count; i++) {
		fileGlobalNode *worker = list_get(workerList, i);

		uint32_t operation_code = WORKER_REQUEST_FILE_FROM_SLAVE;
		send(worker->sockfd, &(operation_code), sizeof(uint32_t), 0);

		send(worker->sockfd, &(worker->filePathLength), sizeof(int), 0);
		send(worker->sockfd, worker->filePath, worker->filePathLength + 1,0);

		int fileSize;
		recv(worker->sockfd, &fileSize, sizeof(int), MSG_WAITALL);

		char *fileContent = malloc(fileSize + 1);
		recv(worker->sockfd, fileContent, fileSize + 1, MSG_WAITALL);

		char *filePath = scriptTempFileName();
		FILE *file = fopen(filePath, "w");
		fputs(fileContent, file);
		fclose(file);

		fileNode *localFile = malloc(sizeof(fileNode));
		localFile->filePath = malloc(strlen(filePath + 1));
		strcpy(localFile->filePath, filePath);
		list_add(localFiles, localFile);
	}

	pairFiles(localFiles, resultFilePath);
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
