/*
 * global_reduce.c
 *
 *  Created on: Nov 24, 2017
 *      Author: Federico Trimboli
 */

#include "global_reduce.h"
#include "yama_socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ipc/ipc.h>
#include <ipc/serialization.h>

#include <commons/log.h>

// Etapa de reducción global
// Recibir de YAMA la IP y puerto del worker "encargado",
// el nombre del archivo temporal de reducción de cada worker,
// el nombre del archivo temporal donde guardar el resultado.
// 1. Conectarse al worker encargado
// 2. Enviarle el programa de reducción, lista de todos los
// workers con sus puertos e IPs, y los nombres de los temporales
// de reducción local.
// 3. Esperar confirmación del worker encargado
// 4. Notificar resultado a YAMA.

extern t_log *master_log;

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
	log_debug(master_log, "REDUCCIÓN GLOBAL. Conectado al worker '%s' (fd: %d).", workerInChargeEntry->nodeID, sockfd);

	uint32_t operation = WORKER_START_GLOBAL_REDUCTION_REQUEST;
	send(sockfd, &operation, sizeof(uint32_t), 0);

	int globalReduceScriptLen = strlen(globalReduceScript);
	send(sockfd, &globalReduceScriptLen, sizeof(uint32_t), 0);
	send(sockfd, globalReduceScript, globalReduceScriptLen + 1, 0);

	send(sockfd, &(yamaRequest->entriesCount), sizeof(uint32_t), 0);
	for (i = 0; i < yamaRequest->entriesCount; i++) {
		ipc_struct_master_continueWithGlobalReductionRequestEntry *entry = yamaRequest->entries + i;
		int nodeIDLen = strlen(entry->nodeID);
		send(sockfd, &nodeIDLen, sizeof(uint32_t), 0);
		send(sockfd, entry->nodeID, nodeIDLen + 1, 0);
		int workerIPLen = strlen(entry->workerIP);
		send(sockfd, &workerIPLen, sizeof(uint32_t), 0);
		send(sockfd, entry->workerIP, workerIPLen + 1, 0);
		send(sockfd, &(entry->workerPort), sizeof(uint32_t), 0);
		int localReduceTempPathLen = strlen(entry->localReduceTempPath);
		send(sockfd, &localReduceTempPathLen, sizeof(uint32_t), 0);
		send(sockfd, entry->localReduceTempPath, localReduceTempPathLen + 1, 0);

		if (entry->isWorkerInCharge == 0) {
			free(entry->nodeID);
			free(entry->workerIP);
			free(entry->localReduceTempPath);
			free(entry->globalReduceTempPath);
		}
	}

	int globalReduceTempPathLen = strlen(workerInChargeEntry->globalReduceTempPath);
	send(sockfd, &globalReduceTempPathLen, sizeof(uint32_t), 0);
	send(sockfd, workerInChargeEntry->globalReduceTempPath, globalReduceTempPathLen + 1, 0);

	// Esperamos respuesta del worker
	uint32_t incomingOperation = 666;
	recv(sockfd, &incomingOperation, sizeof(uint32_t), 0);

	uint32_t reduceSucceeded = 0;

	if (incomingOperation == WORKER_START_GLOBAL_REDUCTION_RESPONSE) {
		recv(sockfd, &reduceSucceeded, sizeof(uint32_t), 0);
	}

	ipc_struct_yama_notify_stage_finish notification;
	notification.nodeID = strdup(workerInChargeEntry->nodeID);
	notification.tempPath = strdup(workerInChargeEntry->globalReduceTempPath);
	notification.succeeded = reduceSucceeded;
	ipc_sendMessage(yamaSocket, YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH, &notification);
	log_debug(master_log, "REDUCCIÓN GLOBAL. Éxito: %d (file: %s. fd: %d).", reduceSucceeded, workerInChargeEntry->globalReduceTempPath, sockfd);

	close(sockfd);

	free(workerInChargeEntry->nodeID);
	free(workerInChargeEntry->workerIP);
	free(workerInChargeEntry->localReduceTempPath);
	free(workerInChargeEntry->globalReduceTempPath);
	free(notification.nodeID);
	free(notification.tempPath);
	free(yamaRequest->entries);
	free(yamaRequest);
	free(globalReduceScript);
}
