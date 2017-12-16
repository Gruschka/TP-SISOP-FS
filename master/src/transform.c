/*
 * transform.c
 *
 *  Created on: Nov 3, 2017
 *      Author: Federico Trimboli
 */

#include "transform.h"
#include "yama_socket.h"
#include "metrics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <ipc/ipc.h>
#include <ipc/serialization.h>

#include <commons/log.h>

// Por cada tarea de transformación:
// 1. crear un hilo
// 2. conectarse al worker correspondiente
// 3. enviarle el programa de transformación
// 4. indicarle sobre qué bloque debe ejecutar el programa, la cantidad
//    de bytes ocupados en dicho bloque, y el archivo temporal donde
//    guardar el resultado.
// 5. esperar confirmación de cada etapa
// 6. Comunicar a YAMA el resultado de cada etapa

typedef struct WorkerRequest {
	char *nodeID;
	char *ip;
	int port;
	ipc_struct_worker_start_transform_request workerRequest;
} WorkerRequest;

extern t_log *master_log;

void *master_localReduce_connectToWorkerAndMakeRequest(void *requestAsVoidPointer) {
	time_t startTimestamp;
	time(&startTimestamp);

	WorkerRequest *request = (WorkerRequest *)requestAsVoidPointer;
	int sockfd = ipc_createAndConnect(request->port, request->ip);
	log_debug(master_log, "TRANSFORMACIÓN. Conectado al worker '%s' (fd: %d).", request->nodeID, sockfd);

	uint32_t operation = WORKER_START_TRANSFORM_REQUEST;
	send(sockfd, &operation, sizeof(uint32_t), MSG_NOSIGNAL);

	send(sockfd, &(request->workerRequest.scriptContentLength), sizeof(uint32_t), MSG_NOSIGNAL);
	send(sockfd, request->workerRequest.scriptContent, (request->workerRequest.scriptContentLength + 1) * sizeof(char), MSG_NOSIGNAL);
	send(sockfd, &(request->workerRequest.block), sizeof(uint32_t), MSG_NOSIGNAL);
	send(sockfd, &(request->workerRequest.usedBytes), sizeof(uint32_t), MSG_NOSIGNAL);
	send(sockfd, &(request->workerRequest.tempFilePathLength), sizeof(uint32_t), MSG_NOSIGNAL);
	send(sockfd, request->workerRequest.tempFilePath, (request->workerRequest.tempFilePathLength + 1) * sizeof(char), MSG_NOSIGNAL);

	// Esperamos respuesta del worker
	uint32_t incomingOperation = 666;
	recv(sockfd, &incomingOperation, sizeof(uint32_t), 0);

	uint32_t transformSucceeded = 0;

	if (incomingOperation == WORKER_START_TRANSFORM_RESPONSE) {
		recv(sockfd, &transformSucceeded, sizeof(uint32_t), 0);
	}

	if (transformSucceeded == 0) {
		master_incrementNumberOfFailures();
	}

	ipc_struct_yama_notify_stage_finish *notification = malloc(sizeof(ipc_struct_yama_notify_stage_finish));
	notification->nodeID = strdup(request->nodeID);
	notification->tempPath = strdup(request->workerRequest.tempFilePath);
	notification->succeeded = transformSucceeded;
	ipc_sendMessage(yamaSocket, YAMA_NOTIFY_TRANSFORM_FINISH, notification);
	log_debug(master_log, "TRANSFORMACIÓN. Éxito: %d (worker: '%s'; file: %s; fd: %d).", transformSucceeded, request->nodeID, request->workerRequest.tempFilePath, sockfd);
	close(sockfd);

	time_t endTimestamp;
	time(&endTimestamp);
	double duration = difftime(endTimestamp, startTimestamp);
	master_incrementNumberOfTransformTasksRan(duration);

	free(notification->nodeID);
	free(notification->tempPath);
	free(notification);
	free(request->workerRequest.scriptContent);
	free(request->workerRequest.tempFilePath);
	free(request->ip);
	free(request->nodeID);
	free(request);
	return NULL;
}

void master_requestWorkersTransform(ipc_struct_start_transform_reduce_response *yamaResponse, char *transformScript) {
	int maxBatchSize = 1000;
//	int maxBatchSize = 20;
	int numberOfBatches = floor(yamaResponse->entriesCount / maxBatchSize) + 1;
	int numberOfTasksInFinalBatch = yamaResponse->entriesCount % maxBatchSize;

	int i;
	for (i = 0; i < numberOfBatches; i++) {
		int isLastBatch = i == (numberOfBatches - 1);
		int numberOfTasksToRun = isLastBatch ? numberOfTasksInFinalBatch : maxBatchSize;

		pthread_t threads[maxBatchSize];
		int j;
		for (j = 0; j < numberOfTasksToRun; j++) {
			int taskIndex = (i * maxBatchSize) + j;
			ipc_struct_start_transform_reduce_response_entry *entry = yamaResponse->entries + taskIndex;

			ipc_struct_worker_start_transform_request workerRequest;
			workerRequest.scriptContentLength = strlen(transformScript);
			workerRequest.scriptContent = strdup(transformScript);
			workerRequest.block = entry->blockID;
			workerRequest.usedBytes = entry->usedBytes;
			workerRequest.tempFilePathLength = strlen(entry->tempPath);
			workerRequest.tempFilePath = strdup(entry->tempPath);

			WorkerRequest *request = malloc(sizeof(WorkerRequest));
			request->nodeID = strdup(entry->nodeID);
			request->ip = strdup(entry->workerIP);
			request->port = entry->workerPort;
			request->workerRequest = workerRequest;

			// Creamos un hilo por cada worker
		    pthread_attr_t attrs;
		    pthread_attr_init(&attrs);
		    pthread_attr_setstacksize(&attrs, 65536);
			if (pthread_create(&(threads[j]), &attrs, master_localReduce_connectToWorkerAndMakeRequest, request)) {
				log_error(master_log, "Falló la creación de thread en etapa de transformación.");
				exit(EXIT_FAILURE);
			}

			free(entry->tempPath);
			free(entry->workerIP);
			free(entry->nodeID);
		}

		for (j = 0; j < numberOfTasksToRun; j++) {
			pthread_join(threads[j], NULL);
		}
	}

	free(yamaResponse->entries);
	free(yamaResponse);
	free(transformScript);
}
