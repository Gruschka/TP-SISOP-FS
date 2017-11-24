/*
 * serialization.h
 *
 *  Created on: Sep 28, 2017
 *      Author: utnso
 */

#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_

#include <stdint.h>

#define OPERATIONS_COUNT 12

typedef enum ipc_operation {
	TEST_MESSAGE,
	FS_GET_FILE_INFO_REQUEST,
	FS_GET_FILE_INFO_RESPONSE,
	YAMA_START_TRANSFORM_REDUCE_REQUEST,
	YAMA_START_TRANSFORM_REDUCE_RESPONSE,
	WORKER_START_TRANSFORM_REQUEST,
	WORKER_START_TRANSFORM_RESPONSE,
	WORKER_START_LOCAL_REDUCTION_REQUEST,
	WORKER_START_LOCAL_REDUCTION_RESPONSE,
	WORKER_START_GLOBAL_REDUCTION_REQUEST,
	WORKER_START_GLOBAL_REDUCTION_RESPONSE,
	MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST
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
}__attribute__((packed)) ipc_struct_fs_get_file_info_request;


typedef struct {
	char *firstCopyNodeID;
	uint32_t firstCopyBlockID;
	char *secondCopyNodeID;
	uint32_t secondCopyBlockID;
	uint32_t blockSize;
}__attribute__((packed)) ipc_struct_fs_get_file_info_response_entry;

typedef struct {
	uint32_t entriesCount;
	uint32_t entriesSize;
	ipc_struct_fs_get_file_info_response_entry *entries;
}__attribute__((packed)) ipc_struct_fs_get_file_info_response;

typedef struct {
	char *filePath;
}__attribute__((packed)) ipc_struct_start_transform_reduce_request;

typedef struct {
	uint32_t nodeID;
	char *workerIP;
	uint32_t workerPort;
	uint32_t blockID;
	uint32_t usedBytes;
	char *tempPath;
}__attribute__((packed)) ipc_struct_start_transform_reduce_response_entry;

typedef struct {
	uint32_t entriesCount;
	uint32_t entriesSize;
	ipc_struct_start_transform_reduce_response_entry *entries;
}__attribute__((packed)) ipc_struct_start_transform_reduce_response;

typedef struct {
	uint32_t scriptContentLength;
	char *scriptContent;
	uint32_t block;
	uint32_t usedBytes;
	uint32_t tempFilePathLength;
	char *tempFilePath;
}__attribute__((packed)) ipc_struct_worker_start_transform_request;

typedef struct {
	uint32_t succeeded;
}__attribute__((packed)) ipc_struct_worker_start_transform_response;

typedef struct {
	uint32_t nodeID;
	char *workerIP;
	uint32_t workerPort;
	char *transformTempPath;
	char *localReduceTempPath;
}__attribute__((packed)) ipc_struct_master_continueWithLocalReductionRequestEntry;

typedef struct {
	uint32_t entriesCount;
	uint32_t entriesSize;
	ipc_struct_master_continueWithLocalReductionRequestEntry *entries;
}__attribute__((packed)) ipc_struct_master_continueWithLocalReductionRequest;

void serialization_initialize();

#endif /* SERIALIZATION_H_ */
