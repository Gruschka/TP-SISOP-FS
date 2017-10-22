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
#include <string.h>
#include <ipc/ipc.h>
#include <ipc/serialization.h>

<<<<<<< HEAD
int main(int argc, char **argv) {
	if (argc != 5) {
		printf("El proceso master debe recibir 4 argumentos.");
		return EXIT_FAILURE;
	}

	char *transformScriptPath = argv[1];
	char *reduceScriptPath = argv[2];
	char *inputFilePath = argv[3];
	char *outputFilePath = argv[4];

=======
int main() {
	printf("Master");
	initialize();
	int sockfd = ipc_createAndConnect(8888, "127.0.0.1");
	ipc_struct_test_message testMessage;
	testMessage.blah = 'A';
	testMessage.bleh = strdup("QTI AMIGO");
	ipc_sendMessage(sockfd, TEST_MESSAGE, &testMessage);
>>>>>>> branch 'master' of git@github.com:sisoputnfrba/tp-2017-2c-Deus-Vult.git
	return EXIT_SUCCESS;
}
