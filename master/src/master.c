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

int main(int argc, char **argv) {
	if (argc != 5) {
		printf("El proceso master debe recibir 4 argumentos.");
		return EXIT_FAILURE;
	}

	char *transformScriptPath = argv[1];
	char *reduceScriptPath = argv[2];
	char *inputFilePath = argv[3];
	char *outputFilePath = argv[4];

	return EXIT_SUCCESS;
}
