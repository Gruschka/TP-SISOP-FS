/*
 * transform.c
 *
 *  Created on: Nov 3, 2017
 *      Author: Federico Trimboli
 */

#include "transform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>

#include <ipc/ipc.h>
#include <ipc/serialization.h>

// Por cada tarea de transformación:
// 1. crear un hilo
// 2. conectarse al worker correspondiente
// 3. enviarle el programa de transformación
// 4. indicarle sobre qué bloque debe ejecutar el programa, la cantidad
//    de bytes ocupados en dicho bloque, y el archivo temporal donde
//    guardar el resultado.
// 5. esperar confirmación de cada etapa
// 6. TODO: comunicar a YAMA el resultado de cada etapa

typedef struct WorkerRequest {
	char *nodeID;
	char *ip;
	int port;
	ipc_struct_worker_start_transform_request workerRequest;
} WorkerRequest;

void *master_localReduce_connectToWorkerAndMakeRequest(void *requestAsVoidPointer) {
	WorkerRequest *request = (WorkerRequest *)requestAsVoidPointer;
	int sockfd = ipc_createAndConnect(request->port, request->ip);

	uint32_t operation = WORKER_START_TRANSFORM_REQUEST;
	send(sockfd, &operation, sizeof(uint32_t), 0);

	send(sockfd, &(request->workerRequest.scriptContentLength), sizeof(uint32_t), 0);
	send(sockfd, request->workerRequest.scriptContent, (request->workerRequest.scriptContentLength + 1) * sizeof(char), 0);
	send(sockfd, &(request->workerRequest.block), sizeof(uint32_t), 0);
	send(sockfd, &(request->workerRequest.usedBytes), sizeof(uint32_t), 0);
	send(sockfd, &(request->workerRequest.tempFilePathLength), sizeof(uint32_t), 0);
	send(sockfd, request->workerRequest.tempFilePath, (request->workerRequest.tempFilePathLength + 1) * sizeof(char), 0);

	// Esperamos respuesta del worker
	uint32_t incomingOperation = 666;
	recv(sockfd, &incomingOperation, sizeof(uint32_t), 0);

	uint32_t transformSucceeded = 0;

	if (incomingOperation == WORKER_START_TRANSFORM_RESPONSE) {
		recv(sockfd, &transformSucceeded, sizeof(uint32_t), 0);
	}

	//FIXME: (Fede) acá enviar resultado de la operación a YAMA

	free(request->workerRequest.scriptContent);
	free(request->workerRequest.tempFilePath);
	free(request->ip);
	free(request->nodeID);
	free(request);

	return NULL;
}

void master_requestWorkersTransform(ipc_struct_start_transform_reduce_response *yamaResponse, char *transformScript) {
	int i = 0;
	while (i < yamaResponse->entriesCount) {
		ipc_struct_start_transform_reduce_response_entry *entry = yamaResponse->entries + i;

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
		pthread_t thread;
		if (pthread_create(&thread, NULL, master_localReduce_connectToWorkerAndMakeRequest, request)) {
			//FIXME: (Fede) acá hay error
		}

		free(entry->tempPath);
		free(entry->workerIP);
		free(entry->nodeID);

		i++;
	}

	free(yamaResponse->entries);
	free(yamaResponse);
	free(transformScript);
}
