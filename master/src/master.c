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
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

#include <string.h>
#include <ipc/ipc.h>
#include <ipc/serialization.h>

#include <commons/config.h>
#include <commons/log.h>

#include "transform.h"
#include "local_reduce.h"
#include "global_reduce.h"
#include "final_storage.h"
#include "utils.h"
#include "yama_socket.h"
#include "metrics.h"

#include <unistd.h>

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
// 6. Comunicar a YAMA el resultado de cada etapa
//
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
// 5. Notificar resultado a YAMA.
//
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
//
// Almacenado final
// 1. Recibir de YAMA la IP y puerto del worker "encargado",
// y el nombre del archivo resultado de la reducción global.
// 2. Conectarse al worker y pedirle que le envíe al FS
// el archivo resultado de la reducción global
// y el nombre y path bajo el cual deberá almacenarse.
// 3. Esperar confirmación
// 4. Notificar a YAMA.
//

int yamaSocket;

t_log *master_log;
char *transformScript;
char *reduceScript;

int main(int argc, char **argv) {
	time_t jobStartTimestamp;
	time(&jobStartTimestamp);

	if (argc != 5) {
		printf("El proceso master debe recibir 4 argumentos.");
		return EXIT_FAILURE;
	}

	// Inicializo métricas
	master_initMetrics();

	// Levanto los argumentos
	char *transformScriptPath = argv[1];
	char *reduceScriptPath = argv[2];
	char *inputFilePath = argv[3];
	char *outputFilePath = argv[4];

	// Creo el logger
	master_log = log_create("log.txt", "master", 1, LOG_LEVEL_DEBUG);

	// Levanto los scripts de transformación y reducción
	log_debug(master_log, "Levantando los scripts de transformación y reducción.");
	transformScript = master_utils_readFile(transformScriptPath);
	reduceScript = master_utils_readFile(reduceScriptPath);

	// Inicializo IPC
	log_debug(master_log, "Inicializando librería IPC.");
	serialization_initialize();

	// Levanto la config
	log_debug(master_log, "Levantando archivo de configuración.");
	t_config *config = config_create("conf/master.conf");
	int yamaPort = config_get_int_value(config, "YAMA_PUERTO");
	char *yamaIP = strdup(config_get_string_value(config, "YAMA_IP"));
	config_destroy(config);

	// Me conecto con YAMA
	log_debug(master_log, "Conectando con YAMA...");
	yamaSocket = ipc_createAndConnect(yamaPort, yamaIP);
	log_debug(master_log, "Conectado a YAMA correctamente.");

	// Le aviso a YAMA que deseo comenzar un transform
	// sobre un archivo
	log_debug(master_log, "Enviando solicitud de inicio de transformación a YAMA...");
	ipc_struct_start_transform_reduce_request request;
	request.filePath = strdup(inputFilePath);
	ipc_sendMessage(yamaSocket, YAMA_START_TRANSFORM_REDUCE_REQUEST, &request);
	free(request.filePath);
	log_debug(master_log, "Se envió solicitud correctamente.");
	log_debug(master_log, "Esperando órdenes de YAMA...");

	while (1) {
		int operation = ipc_getNextOperationId(yamaSocket);
		switch (operation) {
		case YAMA_START_TRANSFORM_REDUCE_RESPONSE: {
			log_debug(master_log, "Iniciando etapa de transformación.");

			ipc_struct_start_transform_reduce_response *yamaResponse = ipc_recvMessage(yamaSocket, YAMA_START_TRANSFORM_REDUCE_RESPONSE);

			// Me conecto con los workers indicados y les
			// envío la información necesaria para la transformación
			master_requestWorkersTransform(yamaResponse, strdup(transformScript));
		} break;
		case MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST: {
			log_debug(master_log, "Iniciando etapa de reducción local.");

			ipc_struct_master_continueWithLocalReductionRequest *yamaLocalReduceRequest = ipc_recvMessage(yamaSocket, MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST);

			// Me conecto con los workers indicados y les envío
			// la información necesaria para el reduce local
			master_requestWorkersLocalReduce(yamaLocalReduceRequest, strdup(reduceScript));
		} break;
		case MASTER_CONTINUE_WITH_GLOBAL_REDUCTION_REQUEST: {
			log_debug(master_log, "Iniciando etapa de reducción global.");

			ipc_struct_master_continueWithGlobalReductionRequest *yamaGlobalReduceRequest = ipc_recvMessage(yamaSocket, MASTER_CONTINUE_WITH_GLOBAL_REDUCTION_REQUEST);

			// Me conecto con el worker encargado y le envío
			// información necesaria para el reduce global
			master_requestInChargeWorkerGlobalReduce(yamaGlobalReduceRequest, strdup(reduceScript));
		} break;
		case MASTER_CONTINUE_WITH_FINAL_STORAGE_REQUEST: {
			log_debug(master_log, "Iniciando etapa de almacenado final.");

			ipc_struct_master_continueWithFinalStorageRequest *yamaFinalStorageRequest = ipc_recvMessage(yamaSocket, MASTER_CONTINUE_WITH_FINAL_STORAGE_REQUEST);

			// Me conecto con el worker encargado y le envío
			// información necesaria para el almacenado final
			master_requestInChargeWorkerFinalStorage(yamaFinalStorageRequest, strdup(outputFilePath));
			goto Exit;
		} break;
		default: {
			log_debug(master_log, "Se recibió una orden desconocida. Cerrando.");
			close(yamaSocket);
			free(yamaIP);
			exit(EXIT_FAILURE);
		} break;
		}
	}

	Exit:
	close(yamaSocket);
	free(yamaIP);

	time_t jobEndTimestamp;
	time(&jobEndTimestamp);
	double jobDuration = difftime(jobEndTimestamp, jobStartTimestamp);
	log_info(master_log, "Métricas:");
	log_info(master_log, "1:");
	log_info(master_log, "El tiempo total de ejecución fue de %.0f segundos.", jobDuration);
	log_info(master_log, "2:");
	log_info(master_log, "El tiempo promedio de duración de una tarea de transformación fue de %.0f segundos.", master_getTransformAverageDuration());
	log_info(master_log, "El tiempo promedio de duración de una tarea de reducción local fue de %.0f segundos.", master_getLocalReductionAverageDuration());
	log_info(master_log, "El tiempo de duración de la reducción global fue de %.0f segundos.", master_getGlobalReductionDuration());
	log_info(master_log, "La cantidad máxima de tareas de transformación ejecutadas de forma paralela fue de %d.", master_getMaxNumberOfConcurrentRunningTransforms());
	log_info(master_log, "La cantidad máxima de tareas de reducción local ejecutadas de forma paralela fue de %d.", master_getMaxNumberOfConcurrentRunningLocalReductions());
	log_info(master_log, "3:");
	log_info(master_log, "La cantidad total de tareas de transformación ejecutadas es %d.", master_getNumberOfTransformTasksRan());
	log_info(master_log, "La cantidad total de tareas de reducción local ejecutadas es %d.", master_getNumberOfLocalReductionTasksRan());
	log_info(master_log, "La cantidad total de tareas de reducción global ejecutadas es 1.");
	log_info(master_log, "4:");
	log_info(master_log, "La cantidad total de fallos ocurridos es %d.", master_getNumberOfFailures());

	return EXIT_SUCCESS;
}
