/*
 * worker.c
 *
 *  Created on: Sep 8, 2017
 *      Author: Hernan Canzonetta
 */

#include "worker.h"
#include "configWorker.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <commons/log.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#define TRANSFORMACION 1
#define REDUCCION_LOCAL 2
#define REDUCCION_GLOBAL 3


t_log *logger;
worker_configuration configuration;


int main() {
	logger = log_create(tmpnam(NULL), "WORKER", 1, LOG_LEVEL_DEBUG);
		loadConfiguration();

		if (signal(SIGUSR1, signalHandler) == SIG_ERR) {
			log_error(logger, "Couldn't register signal handler");
			return EXIT_FAILURE;
		}




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
	setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
	if (socket_desc == -1) {
		log_error(logger, "No se pudo crear el socket.");
	}
	log_debug(logger, "Se creó el socket correctamente.");

	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(configuration.workerPort);

	// Bind
	if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
		log_error(logger, "Couldn't Bind.");
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
	uint32_t operation;
	char *script;
	uint32_t scriptLength;
	uint32_t block;
	char * buffer;
	while(1){

		//Recibo toda la informacion necesaria para ejecutar las tareas
		recv(client_sock,&operation,sizeof(uint32_t), 0);
		recv(client_sock, &scriptLength, sizeof(uint32_t), 0);
		script = malloc(scriptLength);
		recv(client_sock, script,sizeof(scriptLength), 0);
		recv(client_sock, &block, sizeof(uint32_t), 0);
		char * blockContent = malloc(1024);
		readABlock(block, blockContent);
		switch(operation){
			case TRANSFORMACION:{

				//buffer = "./transformacion.py {aca va lo que leiste}  | sort > {aca va el path al output}"
				system(buffer);
				break;
			}
			case REDUCCION_LOCAL:{

				break;
			}
			case REDUCCION_GLOBAL:{

				break;
			}
			default:
				log_error(logger, "Operation couldn't be identified");
		}

	}
}

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
