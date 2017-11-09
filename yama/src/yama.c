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
#include <pthread.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <ipc/ipc.h>
#include <ipc/serialization.h>
#include <arpa/inet.h>
#include "yama.h"
#include "configuration.h"

pthread_mutex_t stateTable_mutex;

yama_configuration configuration;
t_log *logger;
t_list *stateTable;
pthread_t serverThread;

void yst_addEntry(yama_state_table_entry *entry) {
	pthread_mutex_lock(&stateTable_mutex);
	list_add(stateTable, entry);
	pthread_mutex_unlock(&stateTable_mutex);
}

yama_state_table_entry *yst_getEntry(uint32_t jobID, uint32_t masterID, uint32_t nodeID) {
	yama_state_table_entry *result = NULL;

	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currEntry = list_get(stateTable, i);
		if (currEntry->jobID == jobID && currEntry->masterID == masterID && currEntry->nodeID == nodeID) return currEntry;
	}

	return NULL;
}

void testSerialization() {
	ipc_struct_start_transformation_response_entry *first = malloc(sizeof(ipc_struct_start_transformation_response_entry));
	first->blockID = 1;
	first->connectionString = "127.0.0.1:27015";
	first->nodeID = 1;
	first->tempPath = "/tmp/tuvieja";
	first->usedBytes = 100;

	ipc_struct_start_transformation_response_entry *second = malloc(sizeof(ipc_struct_start_transformation_response_entry));
	second->blockID = 2;
	second->connectionString = "127.0.0.2:27015";
	second->nodeID = 2;
	second->tempPath = "/tmp/tuviejo";
	second->usedBytes = 200;

	ipc_struct_start_transformation_response *testResponse = malloc(sizeof(ipc_struct_start_transformation_response));
	testResponse->entriesCount = 2;
	ipc_struct_start_transformation_response_entry entries[2] = { *first, *second } ;
	testResponse->entries = entries;

	SerializationFunction serializationFn = *serializationArray[YAMA_START_TRANSFORMATION_RESPONSE];
	int serializedSize;
	char *serialized = serializationFn((void *)testResponse, &serializedSize);
	log_debug(logger, "serializedSize: %d", serializedSize);
}

void test() {
	yama_state_table_entry *first = malloc(sizeof(yama_state_table_entry));
	yama_state_table_entry *second = malloc(sizeof(yama_state_table_entry));

	first->blockNumber = 1;
	first->jobID = 1;
	first->masterID = 1;
	first->nodeID = 1;
	first->stage = IN_PROCESS;
	first->tempPath = "/tmp/1";

	yst_addEntry(first);

	second->blockNumber = 2;
	second->jobID = 2;
	second->masterID = 2;
	second->nodeID = 2;
	second->stage = IN_PROCESS;
	second->tempPath = "/tmp/2";

	yst_addEntry(second);

	pthread_mutex_lock(&stateTable_mutex);
	yama_state_table_entry *found = yst_getEntry(1, 1, 1);
	pthread_mutex_unlock(&stateTable_mutex);

	log_debug(logger, "Found path: %s", found->tempPath);
	testSerialization();
}

void *server_mainThread() {
	printf("Hello from the other side    xD");
	int sockfd = ipc_createAndListen(8888, 0);

	struct sockaddr_in cliaddr;
	int clilen = sizeof(cliaddr);
	int newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);
	printf("New socket accepted. fd: %d", newsockfd);
	fflush(stdout);
	while (true) {
		ipc_struct_test_message *testMessage = ipc_recvMessage(newsockfd, TEST_MESSAGE);

		printf("testMessage: %s. %c", testMessage->bleh, testMessage->blah);
	}
	return NULL;
}

void initialize() {
	serialization_initialize();

	stateTable = list_create();
	pthread_mutex_init(&stateTable_mutex, NULL);
}

int main(int argc, char** argv) {
	char *logFilePath = tmpnam(NULL);
	logger = log_create(logFilePath, "YAMA", 1, LOG_LEVEL_DEBUG);
	log_debug(logger, "Log file: %s", logFilePath);
	loadConfiguration();
	initialize();

	if (signal(SIGUSR1, signalHandler) == SIG_ERR) {
		log_error(logger, "Couldn't register signal handler");
		return EXIT_FAILURE;
	}

	pthread_create(&serverThread, NULL, server_mainThread, NULL);

	test();

	pthread_join(serverThread, NULL);
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
