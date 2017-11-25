/*
 * final_storage.c
 *
 *  Created on: Nov 25, 2017
 *      Author: Federico Trimboli
 */

#include "final_storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>

#include <ipc/ipc.h>
#include <ipc/serialization.h>

// Almacenado final
// 1. Recibir de YAMA la IP y puerto del worker "encargado",
// y el nombre del archivo resultado de la reducción global.
// 2. Conectarse al worker y pedirle que le envíe al FS
// el archivo resultado de la reducción global
// y el nombre y path bajo el cual deberá almacenarse.
// 3. Esperar confirmación
// 4. TODO: notificar a YAMA.

void master_requestInChargeWorkerFinalStorage(ipc_struct_master_continueWithFinalStorageRequest *yamaRequest) {
	int sockfd = ipc_createAndConnect(yamaRequest->workerPort, yamaRequest->workerIP);

	uint32_t operation = WORKER_START_FINAL_STORAGE_REQUEST;
	send(sockfd, &operation, sizeof(uint32_t), 0);

	int resultPathLen = strlen(yamaRequest->resultPath);
	send(sockfd, &resultPathLen, sizeof(uint32_t), 0);
	send(sockfd, yamaRequest->resultPath, resultPathLen + 1, 0);

	// Esperamos respuesta del worker
	uint32_t incomingOperation = 666;
	recv(sockfd, &incomingOperation, sizeof(uint32_t), 0);

	uint32_t transformSucceeded = 0;

	if (incomingOperation == WORKER_START_FINAL_STORAGE_RESPONSE) {
		recv(sockfd, &transformSucceeded, sizeof(uint32_t), 0);
	}

	//FIXME: (Fede) acá enviar resultado de la operación a YAMA
}
