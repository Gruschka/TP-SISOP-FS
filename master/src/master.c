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

int main() {
	printf("Master");
	int sockfd = ipc_createAndConnect(8888, "0.0.0.0");
	ipc_struct_test_message testMessage;
	testMessage.blah = 'A';
	testMessage.bleh = strdup("QTI AMIGO");
	ipc_sendMessage(sockfd, TEST_MESSAGE, &testMessage);
	return EXIT_SUCCESS;
}

