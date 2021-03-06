/*
 * configuration.c
 *
 *  Created on: Sep 8, 2017
 *      Author: Hernan Canzonetta
 */

#include "configuration.h"
#include <stdlib.h>
#include <string.h>

yama_configuration fetchConfiguration(char *filePath) {
	t_config *configFile = config_create(filePath);
	yama_configuration config;

	config.filesystemPort = config_get_int_value(configFile, "FS_PUERTO");
	config.filesytemIP = strdup(config_get_string_value(configFile, "FS_IP"));
	config.schedulingDelay = config_get_int_value(configFile, "RETARDO_PLANIFICACION");
	config.baseAvailability = config_get_int_value(configFile, "DISP_BASE");
	config.serverPort = strdup(config_get_string_value(configFile, "PUERTO"));

	char *algorithm = config_get_string_value(configFile, "ALGORITMO_BALANCEO");
	config.balancingAlgorithm = strcmp(algorithm, "CLOCK") == 0 ? CLOCK : W_CLOCK;

	config_destroy(configFile);
	return config;
}
