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

#include <ipc/ipc.h>
#include <ipc/serialization.h>

// Por cada tarea de transformación:
// 1. crear un hilo
// 2. conectarse al worker correspondiente
// 3. enviarle el programa de transformación
// 4. indicarle sobre qué bloque debe ejecutar el programa, la cantidad
//    de bytes ocupados en dicho bloque, y el archivo temporal donde
//    guardar el resultado.
// 5. TODO: esperar confirmación de cada etapa y comunicar a YAMA el resultado

void *connectToWorkerAndMakeRequest(master_TransformRequest *request) {
	int sockfd = ipc_createAndConnect(request->port, request->ip);
	ipc_struct_worker_start_transform_request workerRequest;
	workerRequest.scriptContentLength = strlen(request->transformScript);
	workerRequest.scriptContent = strdup(request->transformScript);
	workerRequest.block = request->block;
	workerRequest.usedBytes = request->usedBytes;
	workerRequest.tempFilePathLength = strlen(request->tempFilePath);
	workerRequest.tempFilePath = strdup(request->tempFilePath);

	uint32_t operation = WORKER_START_TRANSFORM_REQUEST;
	send(sockfd, &operation, sizeof(uint32_t), 0);

	send(sockfd, &(workerRequest.scriptContentLength), sizeof(uint32_t), 0);
	send(sockfd, workerRequest.scriptContent, (workerRequest.scriptContentLength + 1) * sizeof(char), 0);
	send(sockfd, &(workerRequest.block), sizeof(uint32_t), 0);
	send(sockfd, &(workerRequest.usedBytes), sizeof(uint32_t), 0);
	send(sockfd, &(workerRequest.tempFilePathLength), sizeof(uint32_t), 0);
	send(sockfd, workerRequest.tempFilePath, (workerRequest.tempFilePathLength + 1) * sizeof(char), 0);

	return NULL;
}

void master_transform_start(master_TransformRequest *requests) {
	master_TransformRequest *request = requests;
//	while (request != '\0') {
		// Creamos un hilo por cada worker
		pthread_t thread;
		if (pthread_create(&thread, NULL, connectToWorkerAndMakeRequest, request)) {
			//FIXME: (Fede) acá hay error
		}

//		request += 1;
//	}
}
