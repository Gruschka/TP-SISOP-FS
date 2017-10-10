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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/wait.h>
#define TRANSFORMATION 1
#define LOCAL_REDUCTION 2
#define GLOBAL_REDUCTION 3


t_log *logger;
worker_configuration configuration;
t_list * fileList;

int main() {
	logger = log_create(tmpnam(NULL), "WORKER", 1, LOG_LEVEL_DEBUG);
		loadConfiguration();

		if (signal(SIGUSR1, signalHandler) == SIG_ERR) {
			log_error(logger, "Couldn't register signal handler");
			return EXIT_FAILURE;
		}
		fileList = list_create();
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
	uint32_t operation;
	char *script;
	uint32_t scriptLength;
	uint32_t block;
	long int bytesToRead;
	const int blockSize = 1024*1024;
	char * buffer;
	char * temporalName;
	uint32_t temporalNameLength;
	if((pid=fork()) == 0){

		//Recibo toda la informacion necesaria para ejecutar las tareas
		recv(client_sock,&operation,sizeof(uint32_t), 0);
		recv(client_sock, &scriptLength, sizeof(uint32_t), 0);
		script = malloc(scriptLength);
		recv(client_sock, script, sizeof(scriptLength), 0);
		switch(operation){
			case TRANSFORMATION:{
				fileNode * file = malloc (sizeof(fileNode));
				recv(client_sock, &block, sizeof(uint32_t), 0);
				bytesToRead = (block * blockSize) + blockSize;
				recv(client_sock, &temporalNameLength, sizeof(uint32_t),0);
				temporalName = malloc(temporalNameLength);
				recv(client_sock, temporalName, temporalNameLength, 0);
				file->filePath = malloc(temporalNameLength);
				memcpy(file->filePath, temporalName, temporalNameLength);
				char * template = "head -c %li /home/utnso/data.bin | tail -c %d | %s | sort > %s";
				int templateSize = snprintf(NULL, 0, template, bytesToRead, blockSize, script, temporalName);
				buffer = malloc(templateSize + 1);
				sprintf(buffer, template, bytesToRead, blockSize, script, temporalName);
				buffer[templateSize] = '\0';
				system(buffer);
				list_add(fileList, file);
				free(buffer);
				free(temporalName);
				free(file);
				free(script);
				break;
			}
			case LOCAL_REDUCTION:{

				break;
			}
			case GLOBAL_REDUCTION:{

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

// De momento no me sirve esto
/*
void readABlock (uint32_t block, char * blockContent){
	int blockLocation = block * 1024;
	FILE *file;

	//Open File
	file = fopen(configuration.binPath, "rb");
	if (!file) {
		log_error(logger, "Couldn't open file: %s", configuration.binPath);
	}

	fseek(file, blockLocation, SEEK_SET);

	fread(&blockContent, 1024, 1, file);


}
*/
// Apareo de Archivos
void pairingFiles(){
	int n = list_size(fileList);
}
