/*
 * serialization.h
 *
 *  Created on: Sep 28, 2017
 *      Author: utnso
 */

#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_

#include <stdint.h>

#define OPERATIONS_COUNT 20

typedef enum ipc_operation {
	TEST_MESSAGE,
	FS_GET_FILE_INFO_REQUEST,
	FS_GET_FILE_INFO_RESPONSE,
	YAMA_START_TRANSFORM_REDUCE_REQUEST,
	YAMA_START_TRANSFORM_REDUCE_RESPONSE,
	YAMA_NOTIFY_TRANSFORM_FINISH,
	YAMA_NOTIFY_LOCAL_REDUCTION_FINISH,
	YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH,
	YAMA_NOTIFY_FINAL_STORAGE_FINISH,
	WORKER_START_TRANSFORM_REQUEST,
	WORKER_START_TRANSFORM_RESPONSE,
	WORKER_START_LOCAL_REDUCTION_REQUEST,
	WORKER_START_LOCAL_REDUCTION_RESPONSE,
	WORKER_START_GLOBAL_REDUCTION_REQUEST,
	WORKER_START_GLOBAL_REDUCTION_RESPONSE,
	WORKER_START_FINAL_STORAGE_REQUEST,
	WORKER_START_FINAL_STORAGE_RESPONSE,
	WORKER_SEND_FILE_TO_YAMA,
	MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST,
	MASTER_CONTINUE_WITH_GLOBAL_REDUCTION_REQUEST,
	MASTER_CONTINUE_WITH_FINAL_STORAGE_REQUEST
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
	char *firstCopyNodeIP;
	uint32_t firstCopyNodePort;
	uint32_t firstCopyBlockID;
	char *secondCopyNodeID;
	char *secondCopyNodeIP;
	uint32_t secondCopyNodePort;
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
	char *nodeID;
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

//YAMA_NOTIFY_TRANSFORM_FINISH,
//YAMA_NOTIFY_LOCAL_REDUCTION_FINISH,
//YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH,
//YAMA_NOTIFY_FINAL_STORAGE_FINISH,
typedef struct {
	char *nodeID;
	char *tempPath;
	char succeeded;
}__attribute__((packed)) ipc_struct_yama_notify_stage_finish;

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

typedef struct ipc_struct_worker_start_local_reduce_TransformTempEntry {
	uint32_t tempPathLen;
	char *tempPath;
} ipc_struct_worker_start_local_reduce_TransformTempEntry;

typedef struct {
	uint32_t scriptContentLength;
	char *scriptContent;
	uint32_t transformTempEntriesCount;
	ipc_struct_worker_start_local_reduce_TransformTempEntry *transformTempEntries;
	uint32_t reduceTempPathLen;
	char *reduceTempPath;
}__attribute__((packed)) ipc_struct_worker_start_local_reduce_request;

typedef struct {
	uint32_t succeeded;
}__attribute__((packed)) ipc_struct_worker_start_local_reduce_response;

typedef struct ipc_struct_worker_start_global_reduce_WorkerEntry {
	uint32_t nodeIDLen;
	char *nodeID;
	uint32_t workerIPLen;
	char *workerIP;
	uint32_t workerPort;
	uint32_t tempPathLen;
	char *tempPath;
} ipc_struct_worker_start_global_reduce_WorkerEntry;

typedef struct {
	uint32_t scriptContentLength;
	char *scriptContent;
	uint32_t workersEntriesCount;
	ipc_struct_worker_start_global_reduce_WorkerEntry *workersEntries;
	uint32_t globalTempPathLen;
	char *globalTempPath;
}__attribute__((packed)) ipc_struct_worker_start_global_reduce_request;

typedef struct {
	uint32_t succeeded;
}__attribute__((packed)) ipc_struct_worker_start_global_reduce_response;

typedef struct {
	uint32_t finalResultPathLen;
	char *finalResultPath;
}__attribute__((packed)) ipc_struct_worker_start_final_storage_request;

typedef struct {
	uint32_t succeeded;
}__attribute__((packed)) ipc_struct_worker_start_final_storage_response;

typedef struct {
	char *nodeID;
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

typedef struct {
	char *nodeID;
	char *workerIP;
	uint32_t workerPort;
	char *localReduceTempPath;
	char *globalReduceTempPath;
	uint32_t isWorkerInCharge;
}__attribute__((packed)) ipc_struct_master_continueWithGlobalReductionRequestEntry;

typedef struct {
	uint32_t entriesCount;
	uint32_t entriesSize;
	ipc_struct_master_continueWithGlobalReductionRequestEntry *entries;
}__attribute__((packed)) ipc_struct_master_continueWithGlobalReductionRequest;

typedef struct {
	char *nodeID;
	char *workerIP;
	uint32_t workerPort;
	char *globalReductionTempPath;
}__attribute__((packed)) ipc_struct_master_continueWithFinalStorageRequest;

typedef struct {
	char *pathName;
	char *file;
}__attribute__((packed)) ipc_struct_worker_file_to_yama;

void serialization_initialize();

#endif /* SERIALIZATION_H_ */
