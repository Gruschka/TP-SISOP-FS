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
	ipc_struct_start_transform_reduce_response_entry *first = malloc(sizeof(ipc_struct_start_transform_reduce_response_entry));
	first->blockID = 1;
	first->workerIP = "127.0.0.1";
	first->workerPort = 1111;
	first->nodeID = 1;
	first->tempPath = "/tmp/tuvieja";
	first->usedBytes = 100;

	ipc_struct_start_transform_reduce_response_entry *second = malloc(sizeof(ipc_struct_start_transform_reduce_response_entry));
	second->blockID = 2;
	second->workerIP = "127.0.0.2";
	second->workerPort = 2222;
	second->nodeID = 2;
	second->tempPath = "/tmp/tuviejo";
	second->usedBytes = 200;

	ipc_struct_start_transform_reduce_response *testResponse = malloc(sizeof(ipc_struct_start_transform_reduce_response));
	testResponse->entriesCount = 2;
	ipc_struct_start_transform_reduce_response_entry entries[2] = { *first, *second } ;
	testResponse->entries = entries;

	SerializationFunction serializationFn = *serializationArray[YAMA_START_TRANSFORM_REDUCE_RESPONSE];
	DeserializationFunction deserializationFn = *deserializationArray[YAMA_START_TRANSFORM_REDUCE_RESPONSE];

	int serializedSize;
	char *serialized = serializationFn((void *)testResponse, &serializedSize);

	log_debug(logger, "serializedSize: %d", serializedSize);
	ipc_struct_start_transform_reduce_response *deserialized = deserializationFn((void *)serialized);

	log_debug(logger, "deserialized: count: %d, size: %d", deserialized->entriesCount, deserialized->entriesSize);
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

	//int fsFd = ipc_createAndConnect(configuration.filesystemPort, configuration.filesytemIP);
	int fsFd = ipc_createAndConnect(8081, "10.0.1.52");
	ipc_struct_fs_get_file_info_request *request = malloc(sizeof(ipc_struct_fs_get_file_info_request));
	request->filePath = "/users/guille/capo.txt";

	ipc_sendMessage(fsFd, YAMA_START_TRANSFORM_REDUCE_REQUEST, request);

	ipc_struct_fs_get_file_info_response *response = ipc_recvMessage(fsFd, YAMA_START_TRANSFORM_REDUCE_RESPONSE);
}

void *server_mainThread() {
	log_debug(logger, "Hello from the other side    xD");
	int sockfd = ipc_createAndListen(8888, 0);

	struct sockaddr_in cliaddr;
	int clilen = sizeof(cliaddr);
	int newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);
	log_debug(logger, "New socket accepted. fd: %d", newsockfd);
	while (true) {
		ipc_struct_start_transform_reduce_request *request = ipc_recvMessage(newsockfd, YAMA_START_TRANSFORM_REDUCE_REQUEST);

		log_debug(logger, "request: path: %s", request->filePath);

		ipc_struct_start_transform_reduce_response_entry *first = malloc(sizeof(ipc_struct_start_transform_reduce_response_entry));
		first->blockID = 1;
		first->workerIP = "127.0.0.1";
		first->workerPort = 1111;
		first->nodeID = 1;
		first->tempPath = "/tmp/tuvieja";
		first->usedBytes = 100;

		ipc_struct_start_transform_reduce_response_entry *second = malloc(sizeof(ipc_struct_start_transform_reduce_response_entry));
		second->blockID = 2;
		second->workerIP = "127.0.0.2";
		second->workerPort = 2222;
		second->nodeID = 2;
		second->tempPath = "/tmp/tuviejo";
		second->usedBytes = 200;

		ipc_struct_start_transform_reduce_response *testResponse = malloc(sizeof(ipc_struct_start_transform_reduce_response));
		testResponse->entriesCount = 2;
		ipc_struct_start_transform_reduce_response_entry entries[2] = { *first, *second } ;
		testResponse->entries = entries;

		ipc_sendMessage(newsockfd, YAMA_START_TRANSFORM_REDUCE_RESPONSE, testResponse);
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
