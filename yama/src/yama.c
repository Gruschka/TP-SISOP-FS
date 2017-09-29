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
#include <commons/collections/list.h>

#include "yama.h"
#include "configuration.h"

yama_configuration configuration;
t_log *logger;
t_list *stateTable;

void test() {
	yama_state_table_entry *entry = malloc(sizeof(yama_state_table_entry));

	entry->blockNumber = 8;
	entry->jobID = 1;
	entry->masterID = 1;
	entry->nodeID = 1;
	entry->stage = IN_PROCESS;
	entry->tempPath = "/tmp/q1w2e3";

	list_add(stateTable, entry);
}

int main(int argc, char** argv) {
	char *logFilePath = tmpnam(NULL);
	logger = log_create(logFilePath, "YAMA", 1, LOG_LEVEL_DEBUG);
	log_debug(logger, "Log file: %s", logFilePath);
	loadConfiguration();

	if (signal(SIGUSR1, signalHandler) == SIG_ERR) {
		log_error(logger, "Couldn't register signal handler");
		return EXIT_FAILURE;
	}

	stateTable = list_create();

	test();
	return EXIT_SUCCESS;
}


void loadConfiguration() {
	configuration = fetchConfiguration("conf/yama.conf");
}

void signalHandler(int signo) {
	if (signo == SIGUSR1) {
		log_debug(logger, "SIGUSR1 - Reloading configuration");
		loadConfiguration();
	}
}
