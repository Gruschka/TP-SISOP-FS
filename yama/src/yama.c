/*
 * yama.c
 *
 *  Created on: Sep 8, 2017
 *      Author: Hernan Canzonetta
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <commons/log.h>

#include "yama.h"
#include "configuration.h"

yama_configuration configuration;
t_log *logger;

int main(int argc, char** argv) {
	logger = log_create(tmpnam(NULL), "YAMA", 1, LOG_LEVEL_DEBUG);
	loadConfiguration();

	if (signal(SIGUSR1, signalHandler) == SIG_ERR) {
		log_error(logger, "Couldn't register signal handler");
		return EXIT_FAILURE;
	}

	while(1)
		sleep(1);
	return EXIT_SUCCESS;
}

void loadConfiguration() {
	configuration = fetchConfiguration("conf/yama.conf");
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
