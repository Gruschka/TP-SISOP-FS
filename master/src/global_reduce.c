/*
 * global_reduce.c
 *
 *  Created on: Nov 24, 2017
 *      Author: Federico Trimboli
 */

#include "global_reduce.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>

#include <ipc/ipc.h>
#include <ipc/serialization.h>

// Etapa de reducción global
// Recibir de YAMA la IP y puerto del worker "encargado",
// el nombre del archivo temporal de reducción de cada worker,
// el nombre del archivo temporal donde guardar el resultado.
// 1. Conectarse al worker encargado
// 2. Enviarle el programa de reducción, lista de todos los
// workers con sus puertos e IPs, y los nombres de los temporales
// de reducción local.
// 3. Esperar confirmación del worker encargado
// 4. TODO: notificar resultado a YAMA.

void master_requestInChargeWorkerGlobalReduce(ipc_struct_master_continueWithGlobalReductionRequest *yamaRequest, char *globalReduceScript) {
	ipc_struct_master_continueWithGlobalReductionRequestEntry *workerInChargeEntry = NULL;

	int i;
	for (i = 0; i < yamaRequest->entriesCount; i++) {
		ipc_struct_master_continueWithGlobalReductionRequestEntry *entry = yamaRequest->entries + i;
		if (entry->isWorkerInCharge) {
			workerInChargeEntry = entry;
			break;
		}
	}

	int sockfd = ipc_createAndConnect(workerInChargeEntry->workerPort, workerInChargeEntry->workerIP);
	uint32_t operation = WORKER_START_GLOBAL_REDUCTION_REQUEST;
	send(sockfd, &operation, sizeof(uint32_t), 0);

	send(sockfd, strlen(globalReduceScript), sizeof(uint32_t), 0);
	send(sockfd, globalReduceScript, strlen(globalReduceScript) + 1, 0);

	send(sockfd, &(yamaRequest->entriesCount), sizeof(uint32_t), 0);
	for (i = 0; i < yamaRequest->entriesCount; i++) {
		ipc_struct_master_continueWithGlobalReductionRequestEntry *entry = yamaRequest->entries + i;
		send(sockfd, strlen(entry->nodeID), sizeof(uint32_t), 0);
		send(sockfd, entry->nodeID, strlen(entry->nodeID) + 1, 0);
		send(sockfd, strlen(entry->workerIP), sizeof(uint32_t), 0);
		send(sockfd, entry->workerIP, strlen(entry->workerIP) + 1, 0);
		send(sockfd, &(entry->workerPort), sizeof(uint32_t), 0);
		send(sockfd, strlen(entry->localReduceTempPath), sizeof(uint32_t), 0);
		send(sockfd, entry->localReduceTempPath, strlen(entry->localReduceTempPath) + 1, 0);
	}

	send(sockfd, strlen(workerInChargeEntry->globalReduceTempPath), sizeof(uint32_t), 0);
	send(sockfd, workerInChargeEntry->globalReduceTempPath, strlen(workerInChargeEntry->globalReduceTempPath) + 1, 0);

	// Esperamos respuesta del worker
	uint32_t incomingOperation = 666;
	recv(sockfd, &incomingOperation, sizeof(uint32_t), 0);

	uint32_t transformSucceeded = 0;

	if (incomingOperation == WORKER_START_GLOBAL_REDUCTION_RESPONSE) {
		recv(sockfd, &transformSucceeded, sizeof(uint32_t), 0);
	}

	//FIXME: (Fede) acá enviar resultado de la operación a YAMA
}
