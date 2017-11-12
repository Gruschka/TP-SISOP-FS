/*
 * serialization.h
 *
 *  Created on: Sep 28, 2017
 *      Author: utnso
 */

#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_

#include <stdint.h>

#define OPERATIONS_COUNT 4

typedef enum ipc_operation {
	TEST_MESSAGE,
	FS_GET_FILE_INFO,
	YAMA_START_TRANSFORM_REDUCE_REQUEST,
	YAMA_START_TRANSFORM_REDUCE_RESPONSE
} ipc_operation;

typedef void *(*DeserializationFunction)(char *buffer);
typedef char *(*SerializationFunction)(void *data, int *size);

SerializationFunction serializationArray[OPERATIONS_COUNT];
DeserializationFunction deserializationArray[OPERATIONS_COUNT];

typedef struct {
	char type;
	uint16_t length;
} __attribute__((packed)) ipc_struct_header;

typedef struct {
    char *bleh;
    char blah;
}__attribute__((packed)) ipc_struct_test_message;

typedef struct {
	char *filePath;
}__attribute__((packed)) ipc_struct_fs_get_file_info;

typedef struct {
	char *filePath;
}__attribute__((packed)) ipc_struct_start_transform_reduce_request;

typedef struct {
	uint32_t nodeID;
	char *connectionString;
	uint32_t blockID;
	uint32_t usedBytes;
	char *tempPath;
}__attribute__((packed)) ipc_struct_start_transform_reduce_response_entry;

typedef struct {
	uint32_t entriesCount;
	uint32_t entriesSize;
	ipc_struct_start_transform_reduce_response_entry *entries;
}__attribute__((packed)) ipc_struct_start_transform_reduce_response;

void serialization_initialize();

#endif /* SERIALIZATION_H_ */
