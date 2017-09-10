/*
 * datanode.c
 *
 *  Created on: Sep 8, 2017
 *      Author: Hernan Canzonetta
 */
#include <stdio.h>
#include <stdlib.h>
#include "datanode.h"
#include <commons/config.h>
#include <commons/log.h>


t_log *logger;
t_config *config;
t_dataNode myDataNode;
t_dataNodeConfig nodeConfig;


int main(int argc, char **argv) {

	char *logFile = tmpnam(NULL);

	logger = log_create(logFile, "DataNode", 1, LOG_LEVEL_DEBUG);

	if (argc < 2) {
		log_error(logger, "Falta pasar la ruta del archivo de configuracion");
		return EXIT_FAILURE;
	} else {
		config = config_create(argv[1]);


		dataNode_loadConfig(&myDataNode);

	}

	printf("caca");

	return EXIT_SUCCESS;
}

int dataNode_loadConfig(t_dataNode *dataNode) {


	dataNode->config.DataNodePortno = config_get_int_value(config, "PUERTO_DATANODE");
	dataNode->config.workerPortno = config_get_int_value(config, "PUERTO_WORKER");
	dataNode->config.fsPortno = config_get_int_value(config, "PUERTO_FILESYSTEM");
	dataNode->config.fsIP = config_get_string_value(config, "IP_FILESYSTEM");
	dataNode->config.nodeName = config_get_string_value(config, "NOMBRE_NODO");
	dataNode->config.databinPath = config_get_string_value(config, "RUTA_DATABIN");


	return 0;



}

