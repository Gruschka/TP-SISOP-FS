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
// 3. TODO: enviarle el programa de transformación
// 4. TODO: indicarle sobre qué bloque debe ejecutar el programa, la cantidad
//    de bytes ocupados en dicho bloque, y el archivo temporal donde
//    guardar el resultado.
// 5. TODO: esperar confirmación de cada etapa y comunicar a YAMA el resultado

void *connectToWorkerAndMakeRequest(void *requestAsVoidPointer) {
	master_TransformRequest *request = (master_TransformRequest *)requestAsVoidPointer;
	int sockfd = ipc_createAndConnect(request->port, request->ip);
	ipc_struct_test_message testMessage;
	testMessage.blah = 'A';
	testMessage.bleh = strdup("QTI AMIGO");
	ipc_sendMessage(sockfd, TEST_MESSAGE, &testMessage);
	return NULL;
}

void master_transform_start(master_TransformRequest *requests) {
	master_TransformRequest *request = requests;

	while (request != NULL) {
		// Creamos un hilo por cada worker
		pthread_t thread;
		if (pthread_create(&thread, NULL, connectToWorkerAndMakeRequest, request)) {
			//FIXME: (Fede) acá hay error
		}

		request += 1;
	}
}
