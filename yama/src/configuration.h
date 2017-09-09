/*
 * configuration.h
 *
 *  Created on: Sep 8, 2017
 *      Author: Hernan Canzonetta
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <commons/config.h>

typedef enum balancing_algorithm {
	WRR, RR
} yama_balancing_algorithm;

typedef struct configuration {
	char *filesytemIP;
	int filesystemPort;
	int schedulingDelay;
	yama_balancing_algorithm balancingAlgorithm;
} yama_configuration;

yama_configuration fetchConfiguration(char *filePath);

#endif /* CONFIGURATION_H_ */
