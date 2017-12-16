/*
 * local_reduce.c
 *
 *  Created on: Nov 24, 2017
 *      Author: Federico Trimboli
 */

#include "local_reduce.h"
#include "yama_socket.h"
#include "metrics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#include <ipc/ipc.h>
#include <ipc/serialization.h>

#include <commons/log.h>

// Etapa de reducción local
// Esperar indicación de YAMA con el nombre de los archivos
// temporales por cada nodo sobre el cual se deba aplicar reducción
// y el nombre del archivo temporal resultado de la reducción.
// Por cada nodo:
// 1. crear un hilo
// 2. conectarse al worker
// 3. enviarle el programa de reducción, la lista de archivos
//    temporales del nodo y el nombre del temporal resultante.
// 4. esperar confirmación del worker
// 5. Notificar resultado a YAMA.

typedef struct WorkerRequest {
	char *nodeID;
	char *ip;
	int port;
	ipc_struct_worker_start_local_reduce_request workerRequest;
} WorkerRequest;

extern t_log *master_log;

void *master_transform_connectToWorkerAndMakeRequest(void *requestAsVoidPointer) {
	clock_t startTimestamp = clock();

	WorkerRequest *request = (WorkerRequest *)requestAsVoidPointer;
	int sockfd = ipc_createAndConnect(request->port, request->ip);
	log_debug(master_log, "REDUCCIÓN LOCAL. Conectado al worker '%s' (fd: %d).", request->nodeID, sockfd);

	uint32_t operation = WORKER_START_LOCAL_REDUCTION_REQUEST;
	send(sockfd, &operation, sizeof(uint32_t), 0);

	send(sockfd, &(request->workerRequest.scriptContentLength), sizeof(uint32_t), 0);
	send(sockfd, request->workerRequest.scriptContent, (request->workerRequest.scriptContentLength + 1) * sizeof(char), 0);

	send(sockfd, &(request->workerRequest.transformTempEntriesCount), sizeof(uint32_t), 0);
	int i;
	for (i = 0; i < request->workerRequest.transformTempEntriesCount; i++) {
		ipc_struct_worker_start_local_reduce_TransformTempEntry *entry = request->workerRequest.transformTempEntries + i;
		send(sockfd, &(entry->tempPathLen), sizeof(uint32_t), 0);
		send(sockfd, entry->tempPath, (entry->tempPathLen + 1) * sizeof(char), 0);

		free(entry->tempPath);
	}

	send(sockfd, &(request->workerRequest.reduceTempPathLen), sizeof(uint32_t), 0);
	send(sockfd, request->workerRequest.reduceTempPath, (request->workerRequest.reduceTempPathLen + 1) * sizeof(char), 0);

	// Esperamos respuesta del worker
	uint32_t incomingOperation = 666;
	recv(sockfd, &incomingOperation, sizeof(uint32_t), 0);

	uint32_t reduceSucceeded = 0;

	if (incomingOperation == WORKER_START_LOCAL_REDUCTION_RESPONSE) {
		recv(sockfd, &reduceSucceeded, sizeof(uint32_t), 0);
	}

	if (reduceSucceeded == 0) {
		master_incrementNumberOfFailures();
	}

	ipc_struct_yama_notify_stage_finish *notification = malloc(sizeof(ipc_struct_yama_notify_stage_finish));
	notification->nodeID = strdup(request->nodeID);
	notification->tempPath = strdup(request->workerRequest.reduceTempPath);
	notification->succeeded = reduceSucceeded;
	ipc_sendMessage(yamaSocket, YAMA_NOTIFY_LOCAL_REDUCTION_FINISH, notification);
	log_debug(master_log, "REDUCCIÓN LOCAL. Éxito: %d (file: %s. fd: %d).", reduceSucceeded, request->workerRequest.reduceTempPath, sockfd);
	close(sockfd);

	clock_t endTimestamp = clock();
	double duration = ((double)(endTimestamp - startTimestamp)) / CLOCKS_PER_SEC;
	master_incrementNumberOfLocalReductionTasksRan(duration);

	free(notification->nodeID);
	free(notification->tempPath);
	free(notification);
	free(request->workerRequest.scriptContent);
	free(request->workerRequest.transformTempEntries);
	free(request->workerRequest.reduceTempPath);
	free(request->ip);
	free(request->nodeID);
	free(request);

	return NULL;
}

void master_requestWorkersLocalReduce(ipc_struct_master_continueWithLocalReductionRequest *yamaRequest, char *localReduceScript) {
	int requestsCount = 0;
	WorkerRequest *requests = NULL;

	int i = 0;
	char *currentNodeID = NULL;
	for (i = 0; i < yamaRequest->entriesCount; i++) {
		ipc_struct_master_continueWithLocalReductionRequestEntry *entry = yamaRequest->entries + i;

		if (strcmp(entry->nodeID, currentNodeID ?: "") != 0) {
			currentNodeID = strdup(entry->nodeID);
			requestsCount++;
			requests = realloc(requests, sizeof(WorkerRequest) * requestsCount);

			ipc_struct_worker_start_local_reduce_request workerRequest;
			workerRequest.scriptContentLength = strlen(localReduceScript);
			workerRequest.scriptContent = strdup(localReduceScript);

			workerRequest.transformTempEntriesCount = 1;
			workerRequest.transformTempEntries = malloc(sizeof(ipc_struct_worker_start_local_reduce_TransformTempEntry));
			ipc_struct_worker_start_local_reduce_TransformTempEntry *tempEntry = workerRequest.transformTempEntries;
			tempEntry->tempPathLen = strlen(entry->transformTempPath);
			tempEntry->tempPath = strdup(entry->transformTempPath);

			workerRequest.reduceTempPathLen = strlen(entry->localReduceTempPath);
			workerRequest.reduceTempPath = strdup(entry->localReduceTempPath);

			WorkerRequest *request = requests + (requestsCount - 1);
			request->nodeID = strdup(entry->nodeID);
			request->ip = strdup(entry->workerIP);
			request->port = entry->workerPort;
			request->workerRequest = workerRequest;
		} else {
			WorkerRequest *request = requests + (requestsCount - 1);

			request->workerRequest.transformTempEntriesCount++;
			request->workerRequest.transformTempEntries = realloc(request->workerRequest.transformTempEntries, sizeof(ipc_struct_worker_start_local_reduce_TransformTempEntry) * request->workerRequest.transformTempEntriesCount);
			ipc_struct_worker_start_local_reduce_TransformTempEntry *tempEntry = request->workerRequest.transformTempEntries + (request->workerRequest.transformTempEntriesCount - 1);
			tempEntry->tempPathLen = strlen(entry->transformTempPath);
			tempEntry->tempPath = strdup(entry->transformTempPath);
		}

		free(entry->localReduceTempPath);
		free(entry->transformTempPath);
		free(entry->workerIP);
		free(entry->nodeID);
	}

	for (i = 0; i < requestsCount; i++) {
		WorkerRequest *request = requests + i;

	 	// Creamos un hilo por cada worker
		pthread_t thread;
		if (pthread_create(&thread, NULL, master_transform_connectToWorkerAndMakeRequest, request)) {
			log_error(master_log, "Falló la creación de thread en etapa de reducción local.");
			exit(EXIT_FAILURE);
		}


	}

	free(currentNodeID);
	free(yamaRequest->entries);
	free(yamaRequest);
	free(localReduceScript);
}
