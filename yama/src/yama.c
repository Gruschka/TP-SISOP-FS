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
#include <signal.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <string.h>
#include "yama.h"
#include "configuration.h"
#include "scheduling.h"

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
uint32_t lastJobID = 0;
int fsFd;

void destroyStateTable() {
	int i = 0;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *entry = list_get(stateTable, i);
		free(entry->nodeID);
		free(entry->tempPath);
		free(entry->fileName);
		free(entry);
	}
	list_destroy(stateTable);
}

int stringsAreEqual(char *str1, char *str2);

void dumpEntry(yama_state_table_entry *entry, uint32_t i) {
	log_debug(logger, "%d -> %d | %d | %s | %d | %d | %s | %d", i, entry->jobID, entry->masterID, entry->nodeID, entry->blockNumber, entry->stage, entry->tempPath, entry->status);
}


t_list *getEntriesMatchingJobID(uint32_t jobID) {
	t_list *result = list_create();
	t_dictionary *dictionary = dictionary_create();

	pthread_mutex_lock(&stateTable_mutex);

	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (currentEntry->jobID == jobID && !dictionary_has_key(dictionary, currentEntry->nodeID)) {
			dictionary_put(dictionary, currentEntry->nodeID, 1);
			list_add(result, currentEntry);
		}
	}
	pthread_mutex_unlock(&stateTable_mutex);

	//TODO: ver que no este liberanod cualqueir cosa
	dictionary_destroy(dictionary);
	return result;
}

t_list *getEntriesMatching(uint32_t jobID, char *nodeID, yama_job_stage stage) {
	t_list *result = list_create();
	pthread_mutex_lock(&stateTable_mutex);

	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (currentEntry->jobID == jobID &&
				currentEntry->stage == stage &&
				stringsAreEqual(currentEntry->nodeID, nodeID) &&
				currentEntry->stage == stage)
			list_add(result, currentEntry);
	}
	pthread_mutex_unlock(&stateTable_mutex);

	return result;
}

int localReductionIsFinished(uint32_t jobID) {
	pthread_mutex_lock(&stateTable_mutex);
	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (currentEntry->jobID == jobID && currentEntry->stage == LOCAL_REDUCTION) {
			if (currentEntry->status != FINISHED_OK) {
				pthread_mutex_unlock(&stateTable_mutex);
				return 0;
			}
		}
	}

	pthread_mutex_unlock(&stateTable_mutex);
	return 1;
}

int jobIsFinished(uint32_t jobID, char *nodeID, yama_job_stage stage) {
	pthread_mutex_lock(&stateTable_mutex);
	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (currentEntry->jobID == jobID && currentEntry->stage == stage && stringsAreEqual(currentEntry->nodeID, nodeID)){
			if(currentEntry->status != FINISHED_OK) {
				pthread_mutex_unlock(&stateTable_mutex);
				return 0;
			}
		}
	}

	pthread_mutex_unlock(&stateTable_mutex);
	return 1;
}

uint32_t getJobIDForTempPath(char *tempPath) {
	pthread_mutex_lock(&stateTable_mutex);
	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (stringsAreEqual(tempPath, currentEntry->tempPath)) {
			pthread_mutex_unlock(&stateTable_mutex);
			return currentEntry->jobID;
		}
	}

	pthread_mutex_unlock(&stateTable_mutex);
	return -1;
}

void setState(uint32_t jobID, yama_job_stage stage, yama_job_status status) {
	pthread_mutex_lock(&stateTable_mutex);
	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (currentEntry->jobID == jobID) {
			currentEntry->status = status;
			currentEntry->stage = stage;
		}
	}
	pthread_mutex_unlock(&stateTable_mutex);
}

void markLocalReductionAsFinished(char *tempPath, char *nodeID) {
	pthread_mutex_lock(&stateTable_mutex);
	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (stringsAreEqual(currentEntry->tempPath, tempPath) && stringsAreEqual(currentEntry->nodeID, nodeID)) {
			currentEntry->status = FINISHED_OK;
		}
	}
	pthread_mutex_unlock(&stateTable_mutex);
}

yama_state_table_entry *getEntry(char *nodeID, char *tempPath) {
	pthread_mutex_lock(&stateTable_mutex);
	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currentEntry = list_get(stateTable, i);

		if (stringsAreEqual(currentEntry->nodeID, nodeID) && stringsAreEqual(currentEntry->tempPath, tempPath)) {
			pthread_mutex_unlock(&stateTable_mutex);
			return currentEntry;
		}
	}
	pthread_mutex_unlock(&stateTable_mutex);

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
	if ((str1 && !str2) || (!str1 && str2)) {
		log_debug(logger, "stringsAreEqual received null");
		return 0;
	}
	if (!str1 && !str2) {
		log_debug(logger, "stringsAreEqual received two nulls");
		return 0;
	}

	return strcmp(str1, str2) == 0;
}

void yst_addEntry(yama_state_table_entry *entry) {
	pthread_mutex_lock(&stateTable_mutex);
	list_add(stateTable, entry);
	pthread_mutex_unlock(&stateTable_mutex);
}


yama_state_table_entry *yst_getEntry(uint32_t jobID, uint32_t masterID, char *nodeID) {
	pthread_mutex_lock(&stateTable_mutex);
	int i;
	for (i = 0; i < list_size(stateTable); i++) {
		yama_state_table_entry *currEntry = list_get(stateTable, i);
		if (currEntry->jobID == jobID && currEntry->masterID == masterID && stringsAreEqual(currEntry->nodeID, nodeID)) {
			pthread_mutex_unlock(&stateTable_mutex);
			return currEntry;
		}
	}

	return NULL;

}

ipc_struct_fs_get_file_info_response *requestInfoToFilesystem(char *filePath) {
	ipc_struct_fs_get_file_info_request *request = malloc(sizeof(ipc_struct_fs_get_file_info_request));
	request->filePath = strdup(filePath);
	ipc_sendMessage(fsFd, FS_GET_FILE_INFO_REQUEST, request);
	ipc_struct_fs_get_file_info_response *response = ipc_recvMessage(fsFd, FS_GET_FILE_INFO_RESPONSE);
	log_debug(logger, "[FS_GET_FILE_INFO_RESPONSE] File %s has %d blocks", filePath, response->entriesCount);
	free(request->filePath);
	free(request);

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
		} else {
			free(firstWorkerInfo->id);
			free(firstWorkerInfo->ip);
			free(firstWorkerInfo);
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
		} else {
			free(secondWorkerInfo->id);
			free(secondWorkerInfo->ip);
			free(secondWorkerInfo);
		}
	}
	return response;
}

void dumpExecutionPlan(ExecutionPlan *executionPlan) {
	log_debug(logger, "ExecutionPlan:");
	log_debug(logger, "i | blockID | workerID");

	int i;
	for (i = 0; i < executionPlan->entriesCount; i++) {
		ExecutionPlanEntry *entry = executionPlan->entries + i;
		log_debug(logger, "%d |   %d   |   %s  ", i, entry->blockID, entry->workerID);
	}

	log_debug(logger, "-------------------");
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
		responseEntry->nodeID = strdup(epEntry->workerID);
		responseEntry->tempPath = tempFileName();

		WorkerInfo *workerInfo = dictionary_get(workersDict, epEntry->workerID);
		responseEntry->workerIP = strdup(workerInfo->ip);
		responseEntry->workerPort = workerInfo->port;
	}

	return response;
}

void trackTransformationResponseInStateTable(ipc_struct_start_transform_reduce_response *response, int fd, char *fileName) {
	int idx;
	for (idx = 0; idx < response->entriesCount; idx++) {
		yama_state_table_entry *entry = malloc(sizeof(yama_state_table_entry));
		ipc_struct_start_transform_reduce_response_entry *responseEntry = response->entries + idx;

		entry->jobID = lastJobID + 1;
		entry->blockNumber = responseEntry->blockID;
		entry->masterID = fd;
		entry->nodeID = strdup(responseEntry->nodeID);
		entry->stage = TRANSFORMATION;
		entry->status = IN_PROCESS;
		entry->tempPath = strdup(responseEntry->tempPath);
		entry->fileName = strdup(fileName);

		yst_addEntry(entry);
	}
	lastJobID++;
}

void newConnectionHandler(int fd, char *ipAddress) {
	log_debug(logger, "New master connection accepted. FD: %d. IP: %s", fd, ipAddress);
	free(ipAddress);
}

void disconnectionHandler(int fd, char *ipAddress) {
	log_debug(logger, "Master disconnected. FD: %d", fd);
}

void freeStartTransformReduceResponse(ipc_struct_start_transform_reduce_response *response) {
	int i = 0;
	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_start_transform_reduce_response_entry *currentEntry = response->entries + i;
		free(currentEntry->nodeID);
		free(currentEntry->tempPath);
		free(currentEntry->workerIP);
	}
	free(response->entries);
	free(response);
}

void freeExecutionPlan(ExecutionPlan *executionPlan) {
	int i = 0;
	for (i = 0; i < executionPlan->entriesCount; i++) {
		ExecutionPlanEntry *currentEntry = executionPlan->entries + i;
		free(currentEntry->workerID);
	}
	free(executionPlan->entries);
	free(executionPlan);
}

void freeFileInfoResponse(ipc_struct_fs_get_file_info_response *response) {
	int i = 0;
	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_fs_get_file_info_response_entry *currentEntry = response->entries + i;
		free(currentEntry->firstCopyNodeID);
		free(currentEntry->firstCopyNodeIP);
		free(currentEntry->secondCopyNodeID);
		free(currentEntry->secondCopyNodeIP);
	}
	free(response->entries);
	free(response);
}

void freeContinueWithGlobalReductionRequest(ipc_struct_master_continueWithGlobalReductionRequest *request) {
	int i = 0;
	for (i = 0; i < request->entriesCount; i++) {
		ipc_struct_master_continueWithGlobalReductionRequestEntry *currentEntry = request->entries + i;
		free(currentEntry->globalReduceTempPath);
		free(currentEntry->localReduceTempPath);
		free(currentEntry->nodeID);
		free(currentEntry->workerIP);
	}
	free(request->entries);
	free(request);
}

void freeContinueWithLocalReductionRequest(ipc_struct_master_continueWithLocalReductionRequest *request) {
	int i = 0;
	for (i = 0; i < request->entriesCount; i++) {
		ipc_struct_master_continueWithLocalReductionRequestEntry *currentEntry = request->entries + i;
		free(currentEntry->localReduceTempPath);
		free(currentEntry->transformTempPath);
		free(currentEntry->nodeID);
		free(currentEntry->workerIP);
	}
	free(request->entries);
	free(request);
}

void incomingDataHandler(int fd, ipc_struct_header header) {
	switch (header.type) {
	case YAMA_START_TRANSFORM_REDUCE_REQUEST: {
		ipc_struct_start_transform_reduce_request *request = ipc_recvMessage(fd, YAMA_START_TRANSFORM_REDUCE_REQUEST);
		log_debug(logger, "request: path: %s", request->filePath);

		ipc_struct_fs_get_file_info_response *fileInfo = requestInfoToFilesystem(request->filePath);

		ExecutionPlan *executionPlan = getExecutionPlan(fileInfo);
		ipc_struct_start_transform_reduce_response *response = getStartTransformationResponse(executionPlan);
		dumpExecutionPlan(executionPlan);
		updateWorkload(executionPlan);
		trackTransformationResponseInStateTable(response, fd, request->filePath);
		ipc_sendMessage(fd, YAMA_START_TRANSFORM_REDUCE_RESPONSE, response);

		free(request->filePath);
		free(request);
		freeStartTransformReduceResponse(response);
		freeExecutionPlan(executionPlan);
		freeFileInfoResponse(fileInfo);
		break;
	}
	case YAMA_NOTIFY_TRANSFORM_FINISH: {
		ipc_struct_yama_notify_stage_finish *transformFinish = ipc_recvMessage(fd, YAMA_NOTIFY_TRANSFORM_FINISH);
		log_debug(logger, "[YAMA_NOTIFY_TRANSFORM_FINISH] nodeID: %s. tempPath: %s. succeeded: %d.", transformFinish->nodeID, transformFinish->tempPath, transformFinish->succeeded);

		if (!transformFinish->succeeded) {
//			yama_state_table_entry *ystEntry = getEntry(transformFinish->nodeID, transformFinish->tempPath);
//			ystEntry->status = ERROR;
//
//			ipc_struct_fs_get_file_info_response *fileInfo = requestInfoToFilesystem(ystEntry->fileName);
//			ExecutionPlan *executionPlan = getExecutionPlan(fileInfo);
//			ipc_struct_start_transform_reduce_response *response = getStartTransformationResponse(executionPlan);
//			trackTransformationResponseInStateTable(response, fd, ystEntry->fileName);
//			ipc_sendMessage(fd, YAMA_START_TRANSFORM_REDUCE_RESPONSE, response);


			break;
		}

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
			char *localReduceTempPath = tempFileName();
			for (i = 0; i < request->entriesCount; i++) {
				yama_state_table_entry *entry = list_get(entriesToReduce, i);
				ipc_struct_master_continueWithLocalReductionRequestEntry *currentEntry = request->entries + i;
				dumpEntry(entry, i);
				currentEntry->localReduceTempPath = strdup(localReduceTempPath);
				currentEntry->transformTempPath = strdup(entry->tempPath);
				WorkerInfo *workerInfo = dictionary_get(workersDict, entry->nodeID);
				currentEntry->workerIP = strdup(workerInfo->ip);
				currentEntry->workerPort = workerInfo->port;
				currentEntry->nodeID = strdup(entry->nodeID);

				entry->stage = LOCAL_REDUCTION;
				entry->status = IN_PROCESS;
				free(entry->tempPath);
				entry->tempPath = strdup(currentEntry->localReduceTempPath);
			}

			ipc_sendMessage(fd, MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST, request);
			list_destroy(entriesToReduce);
			freeContinueWithLocalReductionRequest(request);
			free(localReduceTempPath);
		}

		free(transformFinish->nodeID);
		free(transformFinish->tempPath);
		free(transformFinish);

		break;
	}
	case YAMA_NOTIFY_LOCAL_REDUCTION_FINISH: {
		ipc_struct_yama_notify_stage_finish *localReductionFinish = ipc_recvMessage(fd, YAMA_NOTIFY_LOCAL_REDUCTION_FINISH);
		log_debug(logger, "[YAMA_NOTIFY_LOCAL_REDUCTION_FINISH] nodeID: %s. tempPath: %s. succeeded: %d", localReductionFinish->nodeID, localReductionFinish->tempPath, localReductionFinish->succeeded);

		uint32_t jobID = getJobIDForTempPath(localReductionFinish->tempPath);

		// lo marco como finalizado
		// todo ver el succeeded
//		yama_state_table_entry *ystEntry = getEntry(localReductionFinish->nodeID, localReductionFinish->tempPath);
		markLocalReductionAsFinished(localReductionFinish->tempPath, localReductionFinish->nodeID);

		if (localReductionIsFinished(jobID)) {
			// elegir el nodo menos cargado
			t_list *entriesToReduce = getEntriesMatchingJobID(jobID);
			log_debug(logger, "Terminaron todas las reducciones locales (cantidad: %d) para el job %d", list_size(entriesToReduce), jobID);
			ipc_struct_master_continueWithGlobalReductionRequest *request = malloc(sizeof(ipc_struct_master_continueWithGlobalReductionRequest));
			request->entriesCount = list_size(entriesToReduce);
			request->entries = calloc(request->entriesCount, sizeof(ipc_struct_master_continueWithGlobalReductionRequestEntry));

			char *globalReduceTempPath = tempFileName();
			int i;
			for (i = 0; i < request->entriesCount; i++) {
				yama_state_table_entry *tableEntry = list_get(entriesToReduce, i);
				ipc_struct_master_continueWithGlobalReductionRequestEntry *requestEntry = request->entries + i;

				requestEntry->localReduceTempPath = strdup(tableEntry->tempPath);
				requestEntry->globalReduceTempPath = strdup(globalReduceTempPath);

				WorkerInfo *workerInfo = dictionary_get(workersDict, tableEntry->nodeID);
				requestEntry->workerIP = strdup(workerInfo->ip);
				requestEntry->workerPort = workerInfo->port;
				requestEntry->nodeID = strdup(tableEntry->nodeID);
				requestEntry->isWorkerInCharge = stringsAreEqual(requestEntry->nodeID, localReductionFinish->nodeID);

				dumpEntry(tableEntry, i);
				// lo actualizo en la tabla de estados
				tableEntry->stage = GLOBAL_REDUCTION;
				tableEntry->status = IN_PROCESS;
				tableEntry->tempPath = strdup(requestEntry->globalReduceTempPath);
			}

			ipc_sendMessage(fd, MASTER_CONTINUE_WITH_GLOBAL_REDUCTION_REQUEST, request);
			list_destroy(entriesToReduce);
			freeContinueWithGlobalReductionRequest(request);
			free(globalReduceTempPath);
		}

		free(localReductionFinish->nodeID);
		free(localReductionFinish->tempPath);
		free(localReductionFinish);
		break;
	}
	case YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH: {
		ipc_struct_yama_notify_stage_finish *globalReductionFinish = ipc_recvMessage(fd, YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH);
		log_debug(logger, "[YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH] nodeID: %s. tempPath: %s. succeeded: %d", globalReductionFinish->nodeID, globalReductionFinish->tempPath, globalReductionFinish->succeeded);

		ipc_struct_master_continueWithFinalStorageRequest *request = malloc(sizeof(ipc_struct_master_continueWithFinalStorageRequest));
		request->globalReductionTempPath = strdup(globalReductionFinish->tempPath);
		request->nodeID = strdup(globalReductionFinish->nodeID);

		// Lo marco como terminado
		setState(getJobIDForTempPath(request->globalReductionTempPath), GLOBAL_REDUCTION, FINISHED_OK);

		WorkerInfo *workerInfo = dictionary_get(workersDict, request->nodeID);
		request->workerIP = strdup(workerInfo->ip);
		request->workerPort = workerInfo->port;

		ipc_sendMessage(fd, MASTER_CONTINUE_WITH_FINAL_STORAGE_REQUEST, request);

		free(request->globalReductionTempPath);
		free(request->nodeID);
		free(request->workerIP);
		free(request);

		free(globalReductionFinish->nodeID);
		free(globalReductionFinish->tempPath);
		free(globalReductionFinish);
		break;
	}
	case YAMA_NOTIFY_FINAL_STORAGE_FINISH: {
		ipc_struct_yama_notify_stage_finish *finalStorageFinish = ipc_recvMessage(fd, YAMA_NOTIFY_FINAL_STORAGE_FINISH);
		log_debug(logger, "[YAMA_NOTIFY_FINAL_STORAGE_FINISH] nodeID: %s. tempPath: %s. succeeded: %d", finalStorageFinish->nodeID, finalStorageFinish->tempPath, finalStorageFinish->succeeded);
		//TODO ACTUALIZAR TABLA DE ESTADOS
		free(finalStorageFinish->nodeID);
		free(finalStorageFinish->tempPath);
		free(finalStorageFinish);
		break;
	}

	default:
		break;
	}
}

void *server_mainThread() {
	log_debug(logger, "Waiting for masters on port %s", configuration.serverPort);
	ipc_createSelectServer(configuration.serverPort, newConnectionHandler, incomingDataHandler, disconnectionHandler);
	return NULL;
}

void sigint_handler() {
	log_debug(logger, "Received signal: SIGINT");
	close(fsFd);
	log_debug(logger, "Closed filesystem connection");
	destroyStateTable();
	free(configuration.filesytemIP);
	free(configuration.serverPort);

	exit(0);
}

void initialize() {
	signal(SIGINT, sigint_handler);
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

void test() {

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

	test();
	pthread_create(&serverThread, NULL, server_mainThread, NULL);

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
