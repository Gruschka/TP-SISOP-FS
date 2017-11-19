/*
 * master.c
 *
 *  Created on: Sep 8, 2017
 *      Author: Federico Trimboli
 *
 *      Insanely great master process.
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>

#include <string.h>
#include <ipc/ipc.h>
#include <ipc/serialization.h>

#include "transform.h"

// Setup
// Al iniciar, comunicarse con YAMA e indicarle el archivo
// sobre el cual quiero operar.
//
// Etapa de transformación
// Recibir de YAMA qué procesos worker conectarme (IP y puerto)
// qué bloque de cada uno de ellos se debe aplicar transformación,
// y nombre del archivo temporal donde se debe almacenar el resultado.
// Por cada tarea de transformación:
// 1. crear un hilo
// 2. conectarse al worker correspondiente
// 3. TODO: enviarle el programa de transformación
// 4. TODO: indicarle sobre qué bloque debe ejecutar el programa, la cantidad
//    de bytes ocupados en dicho bloque, y el archivo temporal donde
//    guardar el resultado.
// 5. TODO: esperar confirmación de cada etapa y comunicar a YAMA el resultado
//
// Etapa de reducción local
// TODO: esperar indicación de YAMA con el nombre de los archivos
// temporales por cada nodo sobre el cual se deba aplicar reducción
// y el nombre del archivo temporal resultado de la reducción.
// Por cada nodo:
// 1. TODO: crear un hilo
// 2. TOOD: conectarse al worker
// 3. TODO: enviarle el programa de reducción, la lista de archivos
//    temporales del nodo y el nombre del temporal resultante.
// 4. TODO: esperar confirmación del worker y notificar resultado
//    a YAMA.
//
// Etapa de reducción global
// TODO: recibir de YAMA la IP y puerto del worker "encargado",
// el nombre del archivo temporal de reducción de cada worker,
// el nombre del archivo temporal donde guardar el resultado.
// TODO: conectarse al worker encargado
// TODO: enviarle el programa de reducción, lista de todos los
// workers con sus puertos e IPs, y los nombres de los temporales
// de reducción local.
// TODO: esperar confirmación del worker encargado y notificar
// resultado a YAMA.
//
// Almacenado final
// TODO: recibir de YAMA la IP y puerto del worker "encargado",
// y el nombre del archivo resultado de la reducción global.
// TODO: conectarse al worker y pedirle que le envíe al FS
// TODO: el archivo resultado de la reducción global
// y el nombre y path bajo el cual deberá almacenarse.
// TODO: esperar confirmación y notificar a YAMA.
//
// TODO: replanificación
// TODO: métricas
// TODO: logs

void connectToYamaAndSendData(char *inputFilePath) {
	t_config *config = config_create("conf/master.conf");
	int yamaPort = config_get_int_value(config, "YAMA_PUERTO");
	char *yamaIP = config_get_string_value(config, "YAMA_IP");

	int sockfd = ipc_createAndConnect(yamaPort, strdup(yamaIP));
	ipc_struct_start_transform_reduce_request request;
	request.filePath = strdup(inputFilePath);
	ipc_sendMessage(sockfd, YAMA_START_TRANSFORM_REDUCE_REQUEST, &request);

	ipc_struct_start_transform_reduce_response *response = ipc_recvMessage(sockfd, YAMA_START_TRANSFORM_REDUCE_RESPONSE);

	master_TransformRequest *transformRequests = malloc(sizeof(master_TransformRequest) * response->entriesCount);

	int i;
	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_start_transform_reduce_response_entry *entry = response->entries + i;
		master_TransformRequest *transformRequest = transformRequests + i;
		transformRequest->ip = strdup(entry->workerIP);
		transformRequest->port = entry->workerPort;
		transformRequest->block = entry->blockID;
		transformRequest->usedBytes = entry->usedBytes;
		transformRequest->tempFilePath = strdup(entry->tempPath);
	}

	master_transform_start(transformRequests);

	free(response->entries);
	free(response);
	free(inputFilePath);
	config_destroy(config);
}

int main(int argc, char **argv) {
	if (argc != 5) {
		printf("El proceso master debe recibir 4 argumentos.");
		return EXIT_FAILURE;
	}

	char *transformScriptPath = argv[1];
	char *reduceScriptPath = argv[2];
	char *inputFilePath = argv[3];
	char *outputFilePath = argv[4];

	serialization_initialize();
	connectToYamaAndSendData(strdup(inputFilePath));
	return EXIT_SUCCESS;
}
