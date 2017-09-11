/*
 * configWorker.c
 *
 *  Created on: 10/9/2017
 *      Author: utnso
 */

#include "configWorker.h"
#include <stdlib.h>
#include <string.h>

worker_configuration fetchConfiguration(char *filePath) {
	t_config *configFile = config_create(filePath);
	worker_configuration config;

	config.filesystemPort = config_get_int_value(configFile, "PUERTO_FILESYSTEM");
	config.filesystemIP = config_get_string_value(configFile, "IP_FILESYSTEM");
	config.NodeName = config_get_string_value(configFile, "NOMBRE_NODO");
	config.workerPort = config_get_int_value(configFile, "PUERTO_WORKER");
	config.binPath = config_get_string_value(configFile, "RUTA_DATABIN");

	free(configFile);
	return config;
}

