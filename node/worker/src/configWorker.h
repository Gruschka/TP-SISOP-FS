/*
 * configWorker.h
 *
 *  Created on: 10/9/2017
 *      Author: utnso
 */

#ifndef CONFIGWORKER_H_
#define CONFIGWORKER_H_

#include <commons/config.h>

typedef struct configuration {
	char *filesystemIP;
	int filesystemPort;
	char *NodeName;
	int workerPort;
	char *binPath;
}worker_configuration;

worker_configuration fetchConfiguration(char *filePath);

#endif /* CONFIGWORKER_H_ */
