/*
 * local_reduce.c
 *
 *  Created on: Nov 24, 2017
 *      Author: Federico Trimboli
 */

#include "local_reduce.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>

#include <ipc/ipc.h>
#include <ipc/serialization.h>

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
// 5. TODO: notificar resultado a YAMA.

typedef struct WorkerRequest {
	char *nodeID;
	char *ip;
	int port;
	ipc_struct_worker_start_local_reduce_request workerRequest;
} WorkerRequest;

void *master_transform_connectToWorkerAndMakeRequest(void *requestAsVoidPointer) {
	WorkerRequest *request = (WorkerRequest *)requestAsVoidPointer;
	int sockfd = ipc_createAndConnect(request->port, request->ip);

	uint32_t operation = WORKER_START_LOCAL_REDUCTION_REQUEST;
	send(sockfd, &operation, sizeof(uint32_t), 0);

	send(sockfd, &(request->workerRequest.scriptContentLength), sizeof(uint32_t), 0);
	send(sockfd, request->workerRequest.scriptContent, (request->workerRequest.scriptContentLength + 1) * sizeof(char), 0);
	send(sockfd, &(request->workerRequest.transformTempFilePathLength), sizeof(uint32_t), 0);
	send(sockfd, request->workerRequest.transformTempFilePath, (request->workerRequest.transformTempFilePathLength + 1) * sizeof(char), 0);
	send(sockfd, &(request->workerRequest.reduceTempFilePathLength), sizeof(uint32_t), 0);
	send(sockfd, request->workerRequest.reduceTempFilePath, (request->workerRequest.reduceTempFilePathLength + 1) * sizeof(char), 0);

	// Esperamos respuesta del worker
	uint32_t incomingOperation = 666;
	recv(sockfd, &incomingOperation, sizeof(uint32_t), 0);

	uint32_t transformSucceeded = 0;

	if (incomingOperation == WORKER_START_LOCAL_REDUCTION_RESPONSE) {
		recv(sockfd, &transformSucceeded, sizeof(uint32_t), 0);
	}

	//FIXME: (Fede) acá enviar resultado de la operación a YAMA

	free(request->workerRequest.scriptContent);
	free(request->workerRequest.transformTempFilePath);
	free(request->workerRequest.reduceTempFilePath);
	free(request->ip);
	free(request);

	return NULL;
}

void master_requestWorkersLocalReduce(ipc_struct_master_continueWithLocalReductionRequest *yamaRequest, char *localReduceScript) {
	int i = 0;
	while (i < yamaRequest->entriesCount) {
		ipc_struct_master_continueWithLocalReductionRequestEntry *entry = yamaRequest->entries + i;

		ipc_struct_worker_start_local_reduce_request workerRequest;
		workerRequest.scriptContentLength = strlen(localReduceScript);
		workerRequest.scriptContent = strdup(localReduceScript);
		workerRequest.transformTempFilePathLength = strlen(entry->transformTempPath);
		workerRequest.transformTempFilePath = strdup(entry->transformTempPath);
		workerRequest.reduceTempFilePathLength = strlen(entry->localReduceTempPath);
		workerRequest.reduceTempFilePath = strdup(entry->localReduceTempPath);

		WorkerRequest *request = malloc(sizeof(WorkerRequest));
		request->nodeID = strdup(entry->nodeID);
		request->ip = strdup(entry->workerIP);
		request->port = entry->workerPort;
		request->workerRequest = workerRequest;

		// Creamos un hilo por cada worker
		pthread_t thread;
		if (pthread_create(&thread, NULL, master_transform_connectToWorkerAndMakeRequest, request)) {
			//FIXME: (Fede) acá hay error
		}

		free(entry->localReduceTempPath);
		free(entry->transformTempPath);
		free(entry->workerIP);
		free(entry->nodeID);

		i++;
	}

	free(yamaRequest->entries);
	free(yamaRequest);
	free(localReduceScript);
}
