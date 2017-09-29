/*
 * serialization.h
 *
 *  Created on: Sep 28, 2017
 *      Author: utnso
 */

#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_

#include <stdint.h>

#define OPERATIONS_COUNT 1

#define TEST_MESSAGE 1


typedef void *(*DeserializationFunction)(char *buffer);
typedef char *(*SerializationFunction)(void *data, int *tamanio);

static SerializationFunction serializationArray[OPERATIONS_COUNT];
static DeserializationFunction deserializationArray[OPERATIONS_COUNT];

typedef struct {
	char type;
	uint16_t length;
} __attribute__((packed)) ipc_struct_header;

typedef struct {
    char *bleh;
    char blah;
}__attribute__((packed)) ipc_struct_test_message;

#endif /* SERIALIZATION_H_ */
