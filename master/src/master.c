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
#include "utils.h"

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
// 3. enviarle el programa de transformación
// 4. indicarle sobre qué bloque debe ejecutar el programa, la cantidad
//    de bytes ocupados en dicho bloque, y el archivo temporal donde
//    guardar el resultado.
// 5. esperar confirmación de cada etapa
// 6. TODO: comunicar a YAMA el resultado de cada etapa
//
// Etapa de reducción local
// Esperar indicación de YAMA con el nombre de los archivos
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
// TODO: replanificación.
// TODO: métricas
// TODO: logs

int yamaPort;
char *yamaIP;
int yamaSocket;

int main(int argc, char **argv) {
	if (argc != 5) {
		printf("El proceso master debe recibir 4 argumentos.");
		return EXIT_FAILURE;
	}

	// Levanto los argumentos
	char *transformScriptPath = argv[1];
	char *reduceScriptPath = argv[2];
	char *inputFilePath = argv[3];
	char *outputFilePath = argv[4];

	// Inicializo IPC
	serialization_initialize();

	// Levanto la config
	t_config *config = config_create("conf/master.conf");
	yamaPort = config_get_int_value(config, "YAMA_PUERTO");
	yamaIP = strdup(config_get_string_value(config, "YAMA_IP"));
	config_destroy(config);

	// Me conecto con YAMA
	yamaSocket = ipc_createAndConnect(yamaPort, yamaIP);

	// Le aviso a YAMA que deseo comenzar un transform
	// sobre un archivo
	ipc_struct_start_transform_reduce_request request;
	request.filePath = strdup(inputFilePath);
	ipc_sendMessage(yamaSocket, YAMA_START_TRANSFORM_REDUCE_REQUEST, &request);

	// Espero respuesta de YAMA indicándome a qué workers
	// conectarme y qué enviarles
	ipc_struct_start_transform_reduce_response *yamaResponse = ipc_recvMessage(yamaSocket, YAMA_START_TRANSFORM_REDUCE_RESPONSE);

	// Me conecto con los workers indicados y les
	// envío la información necesaria para la transformación
	master_requestWorkersTransform(yamaResponse, master_utils_readFile(transformScriptPath));

//	ipc_struct_start_transform_reduce_response *yamaResponse = malloc(sizeof(ipc_struct_start_transform_reduce_response));
//	yamaResponse->entries = malloc(sizeof(ipc_struct_start_transform_reduce_response_entry));
//	yamaResponse->entries->workerIP = strdup("10.0.1.19");
//	yamaResponse->entries->workerPort = 5050;
//	yamaResponse->entries->blockID = 3;
//	yamaResponse->entries->usedBytes = 26;
//	yamaResponse->entries->tempPath = strdup("this/is/the/temp/path.bin");
//	yamaResponse->entriesCount = 1;
//
//	master_requestWorkersTransform(yamaResponse, master_utils_readFile(transformScriptPath));

	// Aguardo de YAMA las instrucciones para la reducción local
	ipc_struct_master_continueWithLocalReductionRequest *yamaLocalReduceRequest = ipc_recvMessage(yamaSocket, MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST);

	// Me conecto con los workers indicados y les envío
	// la información necesaria para el reduce local
	master_requestWorkersLocalReduce(yamaLocalReduceRequest, master_utils_readFile(reduceScriptPath));

	while (1) {

	}

	return EXIT_SUCCESS;
}
