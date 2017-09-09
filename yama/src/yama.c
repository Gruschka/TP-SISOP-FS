/*
 * yama.c
 *
 *  Created on: Sep 8, 2017
 *      Author: Hernan Canzonetta
 */
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>

#include "yama.h"
#include "configuration.h"

yama_configuration configuration;
t_log *logger;

int main(int argc, char** argv) {
	loadConfiguration();
	return EXIT_SUCCESS;
}

void loadConfiguration() {
	configuration = fetchConfiguration("conf/yama.conf");
}
