/*
 * worker.c
 *
 *  Created on: Sep 8, 2017
 *      Author: Hernan Canzonetta
 */

#include "worker.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <commons/log.h>


t_log *logger;

int main() {
	logger = log_create(tmpnam(NULL), "WORKER", 1, LOG_LEVEL_DEBUG);
		loadConfiguration();

		if (signal(SIGUSR1, signalHandler) == SIG_ERR) {
			log_error(logger, "Couldn't register signal handler");
			return EXIT_FAILURE;
		}


	return EXIT_SUCCESS;
}
