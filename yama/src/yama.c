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
#include <commons/collections/dictionary.h>
#include <ipc/ipc.h>
#include <ipc/serialization.h>
#include <arpa/inet.h>
#include <string.h>
#include "yama.h"
#include "configuration.h"
#include "scheduling.h"

//TODO: Setear id de master como el fd de su conexion
//TODO: Cerrar scheduling
//TODO: Revisar leaks
//TODO: Deshardcodear puerto

pthread_mutex_t stateTable_mutex;
pthread_mutex_t nodesList_mutex;

uint32_t scheduling_baseAvailability = 1;
yama_configuration configuration;
t_log *logger;
t_list *stateTable;
t_list *nodesList;
t_dictionary *workersDict;
t_dictionary *mastersDict;
pthread_t serverThread;
uint32_t lastJobID = 1;
int fsFd;

int stringsAreEqual(char *str1, char *str2);

void dumpEntry(yama_state_table_entry *entry, uint32_t i) {
	log_debug(logger, "%d -> %d | %d | %s | %d | %d | %s | %d", i, entry->jobID, entry->masterID, entry->nodeID, entry->blockNumber, entry->stage, entry->tempPath, entry->status);
}

t_list *getEntriesMatching(uint32_t jobID, char *nodeID, yama_job_stage stage) {
	t_list *result = list_create();

	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (currentEntry->jobID == jobID &&
				currentEntry->stage == stage &&
				stringsAreEqual(currentEntry->nodeID, nodeID) &&
				currentEntry->stage == stage)
			list_add(result, currentEntry);
	}

	return result;
}

int jobIsFinished(uint32_t jobID, char *nodeID, yama_job_stage stage) {
	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (currentEntry->jobID == jobID &&
				currentEntry->stage == stage &&
				stringsAreEqual(currentEntry->nodeID, nodeID) &&
				currentEntry->status != FINISHED_OK) // si hay algun archivo de ese job q no termino, doy false
			return 0;
	}

	return 1;
}

uint32_t getJobIDForTempPath(char *tempPath) {
	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (stringsAreEqual(tempPath, currentEntry->tempPath)) {
			return currentEntry->jobID;
		}
	}

	return -1;
}

yama_state_table_entry *getEntry(char *nodeID, char *tempPath) {
	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (stringsAreEqual(currentEntry->nodeID, nodeID) && stringsAreEqual(currentEntry->tempPath, tempPath)) {
			return currentEntry;
		}
	}

	return NULL;
}

char *tempFileName() {
	static char template[] = "/tmp/XXXXXX";
	char *name = malloc(strlen(template) + 1);
	strcpy(name, template);
	mktemp(name);

	return name;
}

int stringsAreEqual(char *str1, char *str2) {
	return strcmp(str1, str2) == 0;
}

void yst_addEntry(yama_state_table_entry *entry) {
	pthread_mutex_lock(&stateTable_mutex);
	list_add(stateTable, entry);
	pthread_mutex_unlock(&stateTable_mutex);
}


yama_state_table_entry *yst_getEntry(uint32_t jobID, uint32_t masterID, char *nodeID) {
	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currEntry = list_get(stateTable, i);
		if (currEntry->jobID == jobID && currEntry->masterID == masterID && stringsAreEqual(currEntry->nodeID, nodeID)) return currEntry;
	}

	return NULL;
}

void testStateTable() {
	yama_state_table_entry *first = malloc(sizeof(yama_state_table_entry));
		yama_state_table_entry *second = malloc(sizeof(yama_state_table_entry));

		first->blockNumber = 1;
		first->jobID = 1;
		first->masterID = 1;
		first->nodeID = "NodeA";
		first->stage = IN_PROCESS;
		first->tempPath = "/tmp/1";

		yst_addEntry(first);

		second->blockNumber = 2;
		second->jobID = 2;
		second->masterID = 2;
		second->nodeID = "NodeB";
		second->stage = IN_PROCESS;
		second->tempPath = "/tmp/2";

		yst_addEntry(second);

		pthread_mutex_lock(&stateTable_mutex);
		yama_state_table_entry *found = yst_getEntry(1, 1, "NodeA");
		pthread_mutex_unlock(&stateTable_mutex);

		log_debug(logger, "Found path: %s", found->tempPath);
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

ipc_struct_fs_get_file_info_response *requestInfoToFilesystem(char *filePath) {
	ipc_struct_fs_get_file_info_request *request = malloc(sizeof(ipc_struct_fs_get_file_info_request));
	request->filePath = filePath;
	ipc_sendMessage(fsFd, FS_GET_FILE_INFO_REQUEST, request);
	ipc_struct_fs_get_file_info_response *response = ipc_recvMessage(fsFd, FS_GET_FILE_INFO_RESPONSE);
	printf("File %s has %d blocks", filePath, response->entriesCount);

	int i;
	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_fs_get_file_info_response_entry *entry = response->entries + i;
		WorkerInfo *firstWorkerInfo = malloc(sizeof(WorkerInfo));
		firstWorkerInfo->id = strdup(entry->firstCopyNodeID);
		firstWorkerInfo->ip = strdup(entry->firstCopyNodeIP);
		firstWorkerInfo->port = entry->firstCopyNodePort;
		//TODO: ver como serializar/deserializar cuando no esta alguna copia
		if (!dictionary_has_key(workersDict, entry->firstCopyNodeID)) {
			Worker *worker = malloc(sizeof(Worker));
			worker->availability = 0;
			worker->currentLoad = 0;
			worker->historicalLoad = 0;
			worker->name = strdup(entry->firstCopyNodeID);
			scheduling_addWorker(worker);
			dictionary_put(workersDict, entry->firstCopyNodeID, firstWorkerInfo);
		}

		WorkerInfo *secondWorkerInfo = malloc(sizeof(WorkerInfo));
		secondWorkerInfo->id = strdup(entry->secondCopyNodeID);
		secondWorkerInfo->ip = strdup(entry->secondCopyNodeIP);
		secondWorkerInfo->port = entry->secondCopyNodePort;
		if (!dictionary_has_key(workersDict, entry->secondCopyNodeID)) {
			Worker *worker = malloc(sizeof(Worker));
			worker->availability = 0;
			worker->currentLoad = 0;
			worker->historicalLoad = 0;
			worker->name = strdup(entry->secondCopyNodeID);
			scheduling_addWorker(worker);
			dictionary_put(workersDict, entry->secondCopyNodeID, secondWorkerInfo);
		}
	}
	return response;
}

void testFSConnection() {
	ipc_struct_fs_get_file_info_response *response = requestInfoToFilesystem("/mnt/FS/metadata/archivos/1/ejemplo.txt");
	log_debug(logger, "entriesCount: %d", response->entriesCount);
}

ipc_struct_fs_get_file_info_response_entry *testScheduling_createEntry(char *node1, uint32_t block1, char *node2, uint32_t block2) {
	ipc_struct_fs_get_file_info_response_entry *entry = malloc(sizeof(ipc_struct_fs_get_file_info_response_entry));
	entry->blockSize = 50;
	entry->firstCopyBlockID = block1;
	entry->secondCopyBlockID = block2;
	entry->firstCopyNodeID = strdup(node1);
	entry->secondCopyNodeID = strdup(node2);
	entry->firstCopyNodeIP = "127.0.0.1";
	entry->secondCopyNodeIP = "127.0.0.2";
	entry->firstCopyNodePort = 5001;
	entry->secondCopyNodePort = 5002;
	return entry;
}

ipc_struct_start_transform_reduce_response *getStartTransformationResponse(ExecutionPlan *executionPlan) {
	ipc_struct_start_transform_reduce_response *response = malloc(sizeof(ipc_struct_start_transform_reduce_response));

	response->entriesCount = executionPlan->entriesCount;
	response->entries = malloc(sizeof(ipc_struct_start_transform_reduce_response_entry) * response->entriesCount);

	int i;
	for (i = 0; i < response->entriesCount; i++) {
		ExecutionPlanEntry *epEntry = executionPlan->entries + i;
		ipc_struct_start_transform_reduce_response_entry *responseEntry = response->entries + i;

		responseEntry->blockID = epEntry->blockID;
		responseEntry->usedBytes = epEntry->usedBytes;
		responseEntry->nodeID = epEntry->workerID;
		responseEntry->tempPath = tempFileName();

		WorkerInfo *workerInfo = dictionary_get(workersDict, epEntry->workerID);
		responseEntry->workerIP = strdup(workerInfo->ip);
		responseEntry->workerPort = workerInfo->port;

		log_debug(logger, "[ExecutionPlan] i: %d. blockID: %d. workerID: %s", i, epEntry->blockID, epEntry->workerID);
	}

	return response;
}

void trackTransformationResponseInStateTable(ipc_struct_start_transform_reduce_response *response) {
	int idx;
	for (idx = 0; idx < response->entriesCount; idx++) {
		yama_state_table_entry *entry = malloc(sizeof(yama_state_table_entry));
		ipc_struct_start_transform_reduce_response_entry *responseEntry = response->entries + idx;

		entry->jobID = lastJobID + 1;
		entry->blockNumber = responseEntry->blockID;
		entry->masterID = 1;
		entry->nodeID = strdup(responseEntry->nodeID);
		entry->stage = TRANSFORMATION;
		entry->status = IN_PROCESS;
		entry->tempPath = strdup(responseEntry->tempPath);

		yst_addEntry(entry);
	}
	lastJobID++;
}

void testScheduling(scheduling_algorithm algorithm) {
	scheduling_currentAlgorithm = algorithm;
	ipc_struct_fs_get_file_info_response *testResponse = malloc(sizeof(ipc_struct_fs_get_file_info_response));
	ipc_struct_fs_get_file_info_response_entry *entry1 = testScheduling_createEntry("NodeA", 1, "NodeB", 1);
	ipc_struct_fs_get_file_info_response_entry *entry2 = testScheduling_createEntry("NodeB", 3, "NodeC", 2);
	ipc_struct_fs_get_file_info_response_entry *entry3 = testScheduling_createEntry("NodeD", 1, "NodeE", 1);

	ipc_struct_fs_get_file_info_response_entry entries[3] = { *entry1, *entry2, *entry3 } ;
	testResponse->entries = entries;

	testResponse->entriesCount = 3;
	int i;
	for (i = 0; i < testResponse->entriesCount; i++) {
		ipc_struct_fs_get_file_info_response_entry *entry = testResponse->entries + i;
		WorkerInfo *workerInfo = malloc(sizeof(WorkerInfo));
		workerInfo->id = strdup(entry->firstCopyNodeID);
		workerInfo->ip = strdup(entry->firstCopyNodeIP);
		workerInfo->port = entry->firstCopyNodePort;
		dictionary_put(workersDict, entry->firstCopyNodeID, workerInfo);
	}

	Worker *workerA = malloc(sizeof(Worker));
	workerA->name = "NodeA";
	workerA->currentLoad = 0;
	workerA->historicalLoad = 0;
	Worker *workerB = malloc(sizeof(Worker));
	workerB->name = "NodeB";
	workerB->currentLoad = 0;
	workerB->historicalLoad = 0;
	Worker *workerC = malloc(sizeof(Worker));
	workerC->name = "NodeC";
	workerC->currentLoad = 0;
	workerC->historicalLoad = 0;
	Worker *workerD = malloc(sizeof(Worker));
	workerD->name = "NodeD";
	workerD->currentLoad = 0;
	workerD->historicalLoad = 0;
	Worker *workerE = malloc(sizeof(Worker));
	workerE->name = "NodeE";
	workerE->currentLoad = 0;
	workerE->historicalLoad = 0;

	scheduling_addWorker(workerA);
	scheduling_addWorker(workerB);
	scheduling_addWorker(workerC);
	scheduling_addWorker(workerD);
	scheduling_addWorker(workerE);
	ExecutionPlan *executionPlan = getExecutionPlan(testResponse);
	log_debug(logger, "Execution plan: %d", executionPlan->entriesCount);

	ipc_struct_start_transform_reduce_response *response = getStartTransformationResponse(executionPlan);
	printf(response->entriesSize);
}

void test() {
//	testStateTable();
//	testSerialization();
//	testFSConnection();
//	testScheduling(W_CLOCK);
}

void newConnectionHandler(int fd) {
	log_debug(logger, "New master connection accepted. FD: %d", fd);
}

void disconnectionHandler(int fd) {
	log_debug(logger, "Master disconnected. FD: %d", fd);
}

void incomingDataHandler(int fd, ipc_struct_header header) {
	switch (header.type) {
	case YAMA_START_TRANSFORM_REDUCE_REQUEST: {
		ipc_struct_start_transform_reduce_request *request = ipc_recvMessage(fd, YAMA_START_TRANSFORM_REDUCE_REQUEST);
		log_debug(logger, "request: path: %s", request->filePath);

		ipc_struct_fs_get_file_info_response *fileInfo = requestInfoToFilesystem(request->filePath);

		ExecutionPlan *executionPlan = getExecutionPlan(fileInfo);
		ipc_struct_start_transform_reduce_response *response = getStartTransformationResponse(executionPlan);
		trackTransformationResponseInStateTable(response);
		ipc_sendMessage(fd, YAMA_START_TRANSFORM_REDUCE_RESPONSE, response);

		break;
	}
	case YAMA_NOTIFY_TRANSFORM_FINISH: {
		ipc_struct_yama_notify_stage_finish *transformFinish = ipc_recvMessage(fd, YAMA_NOTIFY_TRANSFORM_FINISH);
		log_debug(logger, "[YAMA_NOTIFY_TRANSFORM_FINISH] nodeID: %s. tempPath: %s. succeeded: %d", transformFinish->nodeID, transformFinish->tempPath, transformFinish->succeeded);

		//succeeded me lo paso por los huevos
		// bueno, esta bien
		// TODO: replanificar

		// lo marco como finalizado
		yama_state_table_entry *ystEntry = getEntry(transformFinish->nodeID, transformFinish->tempPath);
		ystEntry->status = FINISHED_OK;

		uint32_t jobID = getJobIDForTempPath(transformFinish->tempPath);

		// si terminaron todas las transformaciones para ese nodo disparo la siguiente etapa
		if (jobIsFinished(jobID, ystEntry->nodeID, TRANSFORMATION)) {
			t_list *entriesToReduce = getEntriesMatching(jobID, ystEntry->nodeID, TRANSFORMATION);
			log_debug(logger, "Terminaron todas las transformaciones (%d) para el job %d, nodeID: %s", list_size(entriesToReduce), jobID, ystEntry->nodeID);
			ipc_struct_master_continueWithLocalReductionRequest *request = malloc(sizeof(ipc_struct_master_continueWithLocalReductionRequest));
			request->entriesCount = list_size(entriesToReduce);
			request->entries = malloc(sizeof(ipc_struct_master_continueWithLocalReductionRequestEntry) * request->entriesCount);

			int i;
			for (i = 0; i < request->entriesCount; i++) {
				yama_state_table_entry *entry = list_get(entriesToReduce, i);
				ipc_struct_master_continueWithLocalReductionRequestEntry *currentEntry = request->entries + i;
				dumpEntry(entry, i);
				currentEntry->localReduceTempPath = tempFileName();
				currentEntry->transformTempPath = strdup(entry->tempPath);
				WorkerInfo *workerInfo = dictionary_get(workersDict, entry->nodeID);
				currentEntry->workerIP = strdup(workerInfo->ip);
				currentEntry->workerPort = workerInfo->port;
				currentEntry->nodeID = strdup(entry->nodeID);
			}

			ipc_sendMessage(fd, MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST, request);
			list_destroy(entriesToReduce);
			free(request);
		}

		break;
	}
	case YAMA_NOTIFY_LOCAL_REDUCTION_FINISH: {
		ipc_struct_yama_notify_stage_finish *localReductionFinish = ipc_recvMessage(fd, YAMA_NOTIFY_LOCAL_REDUCTION_FINISH);
		log_debug(logger, "[YAMA_NOTIFY_LOCAL_REDUCTION_FINISH] nodeID: %s. tempPath: %s. succeeded: %d", localReductionFinish->nodeID, localReductionFinish->tempPath, localReductionFinish->succeeded);
		break;
	}
	case YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH: {
		ipc_struct_yama_notify_stage_finish *globalReductionFinish = ipc_recvMessage(fd, YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH);
		log_debug(logger, "[YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH] nodeID: %s. tempPath: %s. succeeded: %d", globalReductionFinish->nodeID, globalReductionFinish->tempPath, globalReductionFinish->succeeded);

		break;
	}
	case YAMA_NOTIFY_FINAL_STORAGE_FINISH: {
		ipc_struct_yama_notify_stage_finish *finalStorageFinish = ipc_recvMessage(fd, YAMA_NOTIFY_FINAL_STORAGE_FINISH);
		log_debug(logger, "[YAMA_NOTIFY_FINAL_STORAGE_FINISH] nodeID: %s. tempPath: %s. succeeded: %d", finalStorageFinish->nodeID, finalStorageFinish->tempPath, finalStorageFinish->succeeded);

		break;
	}

	default:
		break;
	}
}

void *server_mainThread() {
	log_debug(logger, "Waiting for masters");
	ipc_createEpollServer("8888", newConnectionHandler, incomingDataHandler, disconnectionHandler);
	return NULL;
}

void initialize() {
	serialization_initialize();
	scheduling_initialize();

	stateTable = list_create();
	nodesList = list_create();
	workersDict = dictionary_create();
	mastersDict = dictionary_create();
	fsFd = ipc_createAndConnect(configuration.filesystemPort, configuration.filesytemIP);
	log_debug(logger, "Connected to FS");
	pthread_mutex_init(&stateTable_mutex, NULL);
	pthread_mutex_init(&nodesList_mutex, NULL);
}

int main(int argc, char** argv) {
	char *logFilePath = tempFileName();
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

	scheduling_baseAvailability = configuration.baseAvailability;
}

void signalHandler(int signo) {
	if (signo == SIGUSR1) {
		log_debug(logger, "SIGUSR1 - Reloading configuration");
		loadConfiguration();
	}
}
