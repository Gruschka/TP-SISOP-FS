/*
 * final_storage.c
 *
 *  Created on: Nov 25, 2017
 *      Author: Federico Trimboli
 */

#include "final_storage.h"
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

// Almacenado final
// 1. Recibir de YAMA la IP y puerto del worker "encargado",
// y el nombre del archivo resultado de la reducción global.
// 2. Conectarse al worker y pedirle que le envíe al FS
// el archivo resultado de la reducción global
// y el nombre y path bajo el cual deberá almacenarse.
// 3. Esperar confirmación
// 4. Notificar a YAMA.

extern t_log *master_log;

void master_requestInChargeWorkerFinalStorage(ipc_struct_master_continueWithFinalStorageRequest *yamaRequest, char *resultPath) {
	int sockfd = ipc_createAndConnect(yamaRequest->workerPort, yamaRequest->workerIP);
	log_debug(master_log, "ALMACENADO FINAL. Conectado al worker '%s' (fd: %d).", yamaRequest->nodeID, sockfd);

	uint32_t operation = WORKER_START_FINAL_STORAGE_REQUEST;
	send(sockfd, &operation, sizeof(uint32_t), 0);

	int globalReductionTempPathLen = strlen(yamaRequest->globalReductionTempPath);
	send(sockfd, &globalReductionTempPathLen, sizeof(uint32_t), 0);
	send(sockfd, yamaRequest->globalReductionTempPath, globalReductionTempPathLen + 1, 0);

	int resultPathLen = strlen(resultPath);
	send(sockfd, &resultPathLen, sizeof(uint32_t), 0);
	send(sockfd, resultPath, resultPathLen + 1, 0);

	// Esperamos respuesta del worker
	uint32_t incomingOperation = 666;
	recv(sockfd, &incomingOperation, sizeof(uint32_t), 0);

	uint32_t storageSucceeded = 0;

	if (incomingOperation == WORKER_START_FINAL_STORAGE_RESPONSE) {
		recv(sockfd, &storageSucceeded, sizeof(uint32_t), 0);
	}

	ipc_struct_yama_notify_stage_finish notification;
	notification.nodeID = strdup(yamaRequest->nodeID);
	notification.tempPath = strdup(yamaRequest->globalReductionTempPath);
	notification.succeeded = storageSucceeded;
	ipc_sendMessage(yamaSocket, YAMA_NOTIFY_FINAL_STORAGE_FINISH, &notification);
	log_debug(master_log, "ALMACENADO FINAL. Éxito: %d (file: %s. fd: %d).", storageSucceeded, yamaRequest->globalReductionTempPath, sockfd);

	close(sockfd);

	free(notification.nodeID);
	free(notification.tempPath);
	free(yamaRequest->nodeID);
	free(yamaRequest->workerIP);
	free(yamaRequest->globalReductionTempPath);
	free(yamaRequest);
	free(resultPath);
}
