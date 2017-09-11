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
#include <unistd.h>
#include <commons/log.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros



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

void *serverThread_main() {
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
	}

	if (client_sock < 0) {
		log_error(logger, "Couldn't accept connection.");
		return NULL;
	}

	return 0;
}
