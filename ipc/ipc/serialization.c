/*
 * serialization.c
 *
 *  Created on: Sep 28, 2017
 *      Author: utnso
 */
#include "serialization.h"

#include <stdlib.h>
#include <string.h>

// Deserialization functions

// TEST_MESSAGE
void *deserializeTestMessage(char *buffer){
	ipc_struct_test_message *testMessage = malloc(sizeof(ipc_struct_test_message));
	testMessage->bleh = strdup(buffer);
	memcpy(&testMessage->blah, buffer+strlen(buffer)+1, sizeof(char));
    return testMessage;
}

// FS_GET_FILE_INFO_REQUEST
void *deserializeFSGetFileInfoRequest(char *buffer) {
	ipc_struct_fs_get_file_info_request *request = malloc(sizeof(ipc_struct_fs_get_file_info_request));
	request->filePath = strdup(buffer);
	return request;
}

void *deserializeSendFileToYama(char *buffer) {
	char *tmp; int tmpLen;
	ipc_struct_worker_file_to_yama *sendFile = malloc(sizeof(ipc_struct_worker_file_to_yama));
	int offset = 0;

	tmp = strdup(buffer + offset);
	tmpLen = strlen(tmp);
	sendFile->pathName = malloc(tmpLen + 1);
	memcpy(sendFile->pathName, tmp, tmpLen + 1); //pathName
	offset += tmpLen + 1;

	tmp = strdup(buffer + offset);
	tmpLen = strlen(tmp);
	sendFile->file = malloc(tmpLen + 1);
	memcpy(sendFile->file, tmp, tmpLen + 1); //pathName
	offset += tmpLen + 1;

	return sendFile;
}

// FS_GET_FILE_INFO_RESPONSE
void *deserializeFSGetFileInfoResponse(char *buffer) {
	int offset = 0, i;
	ipc_struct_fs_get_file_info_response *response = malloc(sizeof(ipc_struct_fs_get_file_info_response));
	memcpy(&(response->entriesCount), buffer + offset, sizeof(uint32_t)); sizeof(uint32_t);
	offset += sizeof(uint32_t);
	memcpy(&(response->entriesSize), buffer + offset, sizeof(uint32_t)); sizeof(uint32_t);
	offset += sizeof(uint32_t);

	int entriesOffset = 0;
	ipc_struct_fs_get_file_info_response_entry *entries = malloc(sizeof(ipc_struct_fs_get_file_info_response_entry) * response->entriesCount);
	for (i = 0; i < response->entriesCount; i++) {
		char *tmp; int tmpLen;
		ipc_struct_fs_get_file_info_response_entry *currentEntry = entries + i;
		tmp = strdup(buffer + offset);
		tmpLen = strlen(tmp);
		currentEntry->firstCopyNodeID = malloc(tmpLen + 1);
		memcpy(currentEntry->firstCopyNodeID, tmp, tmpLen + 1); //firstCopyNodeID
		entriesOffset += tmpLen + 1;
		offset += tmpLen + 1;

		tmp = strdup(buffer + offset);
		tmpLen = strlen(tmp);
		currentEntry->firstCopyNodeIP = malloc(tmpLen + 1);
		memcpy(currentEntry->firstCopyNodeIP, tmp, tmpLen + 1); //firstCopyNodeIP
		entriesOffset += tmpLen + 1;
		offset += tmpLen + 1;

		memcpy(&(currentEntry->firstCopyNodePort), buffer + offset, sizeof(uint32_t)); //firstCopyNodePort
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);

		memcpy(&(currentEntry->firstCopyBlockID), buffer + offset, sizeof(uint32_t)); //firstCopyBlockID
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);

		tmp = strdup(buffer + offset);
		tmpLen = strlen(tmp);
		currentEntry->secondCopyNodeID = malloc(tmpLen + 1);
		memcpy(currentEntry->secondCopyNodeID, tmp, tmpLen + 1); //secondCopyNodeID
		entriesOffset += tmpLen + 1;
		offset += tmpLen + 1;

		tmp = strdup(buffer + offset);
		tmpLen = strlen(tmp);
		currentEntry->secondCopyNodeIP = malloc(tmpLen + 1);
		memcpy(currentEntry->secondCopyNodeIP, tmp, tmpLen + 1); //secondCopyNodeIP
		entriesOffset += tmpLen + 1;
		offset += tmpLen + 1;

		memcpy(&(currentEntry->secondCopyNodePort), buffer + offset, sizeof(uint32_t)); //secondCopyNodePort
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);

		memcpy(&(currentEntry->secondCopyBlockID), buffer + offset, sizeof(uint32_t)); //secondCopyBlockID
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);

		memcpy(&(currentEntry->blockSize), buffer + offset, sizeof(uint32_t)); //blockSize
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);
		free(tmp);
	}

	response->entries = entries;

	return response;
}

// YAMA_START_TRANSFORMATION_REQUEST
void *deserializeYAMAStartTransformationRequest(char *buffer) {
	ipc_struct_start_transform_reduce_request *request = malloc(sizeof(ipc_struct_start_transform_reduce_request));
	request->filePath = strdup(buffer);
	return request;
}

// YAMA_START_TRANSFORMATION_RESPONSE
void *deserializeYAMAStartTransformationResponse(char *buffer) {
	int offset = 0, i;
	ipc_struct_start_transform_reduce_response *response = malloc(sizeof(ipc_struct_start_transform_reduce_response));
	memcpy(&(response->entriesCount), buffer + offset, sizeof(uint32_t)); sizeof(uint32_t);
	offset += sizeof(uint32_t);
	memcpy(&(response->entriesSize), buffer + offset, sizeof(uint32_t)); sizeof(uint32_t);
	offset += sizeof(uint32_t);

	int entriesOffset = 0;
	ipc_struct_start_transform_reduce_response_entry *entries = malloc(sizeof(ipc_struct_start_transform_reduce_response_entry) * response->entriesCount);
	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_start_transform_reduce_response_entry *currentEntry = entries + i;
		char *tmpNodeID = strdup(buffer + offset);
		int tmpNodeIDStringLen = strlen(tmpNodeID);
		memcpy(currentEntry->nodeID, tmpNodeID, tmpNodeIDStringLen + 1); //nodeID
		entriesOffset += tmpNodeIDStringLen + 1;
		offset += tmpNodeIDStringLen + 1;

		char *tmpConnectionString = strdup(buffer + offset);
		int tmpConnectionStringLen = strlen(tmpConnectionString);
		currentEntry->workerIP = malloc(tmpConnectionStringLen + 1);
		memcpy(currentEntry->workerIP, tmpConnectionString, tmpConnectionStringLen + 1); //workerIP
		entriesOffset += tmpConnectionStringLen + 1;
		offset += tmpConnectionStringLen + 1;

		memcpy(&(currentEntry->workerPort), buffer + offset, sizeof(uint32_t)); //workerPort
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);

		memcpy(&(currentEntry->blockID), buffer + offset, sizeof(uint32_t)); //blockID
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);

		memcpy(&(currentEntry->usedBytes), buffer + offset, sizeof(uint32_t)); //usedBytes
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);
		char *tmpPath = strdup(buffer + offset);
		int tmpPathLen = strlen(tmpPath);
		currentEntry->tempPath = malloc(tmpPathLen + 1);

		memcpy(currentEntry->tempPath, tmpPath, tmpPathLen + 1); //tmpPath
		entriesOffset += tmpPathLen + 1;
		offset += tmpPathLen + 1;

		free(tmpNodeID);
		free(tmpPath);
		free(tmpConnectionString);
	}
	response->entries = entries;
	return response;
}

// MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST
void *deserializeMasterContinueWithLocalReductionRequest(char *buffer) {
	int offset = 0, i;
	ipc_struct_master_continueWithLocalReductionRequest *response = malloc(sizeof(ipc_struct_master_continueWithLocalReductionRequest));
	memcpy(&(response->entriesCount), buffer + offset, sizeof(uint32_t)); sizeof(uint32_t);
	offset += sizeof(uint32_t);
	memcpy(&(response->entriesSize), buffer + offset, sizeof(uint32_t)); sizeof(uint32_t);
	offset += sizeof(uint32_t);

	int entriesOffset = 0;
	ipc_struct_master_continueWithLocalReductionRequestEntry *entries = malloc(sizeof(ipc_struct_master_continueWithLocalReductionRequestEntry) * response->entriesCount);
	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_master_continueWithLocalReductionRequestEntry *currentEntry = entries + i;
		memcpy(&(currentEntry->nodeID), buffer + offset, sizeof(uint32_t)); //nodeID
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);
		char *tmpWorkerIP = strdup(buffer + offset);
		int tpmWorkerIPLength = strlen(tmpWorkerIP);
		currentEntry->workerIP = malloc(tpmWorkerIPLength + 1);
		memcpy(currentEntry->workerIP, tmpWorkerIP, tpmWorkerIPLength + 1); //workerIP
		entriesOffset += tpmWorkerIPLength + 1;
		offset += tpmWorkerIPLength + 1;
		memcpy(&(currentEntry->workerPort), buffer + offset, sizeof(uint32_t)); //workerPort
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);
		char *tmpTransformPath = strdup(buffer + offset);
		int tmpTransformPathLen = strlen(tmpTransformPath);
		currentEntry->transformTempPath = malloc(tmpTransformPathLen + 1);
		memcpy(currentEntry->transformTempPath, tmpTransformPath, tmpTransformPathLen + 1); //tmpTransformPath
		entriesOffset += tmpTransformPathLen + 1;
		offset += tmpTransformPathLen + 1;
		char *tmpLocalReducePath = strdup(buffer + offset);
		int tmpLocalReducePathLen = strlen(tmpLocalReducePath);
		currentEntry->localReduceTempPath = malloc(tmpLocalReducePathLen + 1);
		memcpy(currentEntry->localReduceTempPath, tmpLocalReducePath, tmpLocalReducePathLen + 1); //tmpLocalReducePath
		entriesOffset += tmpLocalReducePathLen + 1;
		offset += tmpLocalReducePathLen + 1;
		free(tmpTransformPath);
		free(tmpLocalReducePath);
		free(tmpWorkerIP);
	}
	response->entries = entries;
	return response;
}

// MASTER_CONTINUE_WITH_GLOBAL_REDUCTION_REQUEST
void *deserializeMasterContinueWithGlobalReductionRequest(char *buffer) {
	int offset = 0, i;
	ipc_struct_master_continueWithGlobalReductionRequest *response = malloc(sizeof(ipc_struct_master_continueWithGlobalReductionRequest));
	memcpy(&(response->entriesCount), buffer + offset, sizeof(uint32_t)); sizeof(uint32_t);
	offset += sizeof(uint32_t);
	memcpy(&(response->entriesSize), buffer + offset, sizeof(uint32_t)); sizeof(uint32_t);
	offset += sizeof(uint32_t);

	int entriesOffset = 0;
	ipc_struct_master_continueWithGlobalReductionRequestEntry *entries = malloc(sizeof(ipc_struct_master_continueWithGlobalReductionRequest) * response->entriesCount);
	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_master_continueWithGlobalReductionRequestEntry *currentEntry = entries + i;
		memcpy(&(currentEntry->nodeID), buffer + offset, sizeof(uint32_t)); //nodeID
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);
		char *tmpWorkerIP = strdup(buffer + offset);
		int tpmWorkerIPLength = strlen(tmpWorkerIP);
		currentEntry->workerIP = malloc(tpmWorkerIPLength + 1);
		memcpy(currentEntry->workerIP, tmpWorkerIP, tpmWorkerIPLength + 1); //workerIP
		entriesOffset += tpmWorkerIPLength + 1;
		offset += tpmWorkerIPLength + 1;
		memcpy(&(currentEntry->workerPort), buffer + offset, sizeof(uint32_t)); //workerPort
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);
		char *tmpLocalReducePath = strdup(buffer + offset);
		int tmpLocalReducePathLen = strlen(tmpLocalReducePath);
		currentEntry->localReduceTempPath = malloc(tmpLocalReducePathLen + 1);
		memcpy(currentEntry->localReduceTempPath, tmpLocalReducePath, tmpLocalReducePathLen + 1); //tmpLocalReducePath
		entriesOffset += tmpLocalReducePathLen + 1;
		offset += tmpLocalReducePathLen + 1;
		char *tmpGlobalReducePath = strdup(buffer + offset);
		int tmpGlobalReducePathLen = strlen(tmpGlobalReducePath);
		currentEntry->globalReduceTempPath = malloc(tmpGlobalReducePathLen + 1);
		memcpy(currentEntry->globalReduceTempPath, tmpGlobalReducePath, tmpGlobalReducePathLen + 1); //tmpTransformPath
		entriesOffset += tmpGlobalReducePathLen + 1;
		offset += tmpGlobalReducePathLen + 1;
		memcpy(&(currentEntry->isWorkerInCharge), buffer + offset, sizeof(uint32_t)); //workerPort
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);
		free(tmpGlobalReducePath);
		free(tmpLocalReducePath);
		free(tmpWorkerIP);
	}
	response->entries = entries;
	return response;
}

// MASTER_CONTINUE_WITH_FINAL_STORAGE_REQUEST
void *deserializeMasterContinueWithFinalStorageRequest(char *buffer) {
	int offset = 0;
	ipc_struct_master_continueWithFinalStorageRequest *response = malloc(sizeof(ipc_struct_master_continueWithFinalStorageRequest));
	memcpy(&(response->nodeID), buffer + offset, sizeof(uint32_t)); //nodeID
	offset += sizeof(uint32_t);
	char *tmpWorkerIP = strdup(buffer + offset);
	int tpmWorkerIPLength = strlen(tmpWorkerIP);
	response->workerIP = malloc(tpmWorkerIPLength + 1);
	memcpy(response->workerIP, tmpWorkerIP, tpmWorkerIPLength + 1); //workerIP
	offset += tpmWorkerIPLength + 1;
	memcpy(&(response->workerPort), buffer + offset, sizeof(uint32_t)); //workerPort
	offset += sizeof(uint32_t);
	char *tmpResultPath = strdup(buffer + offset);
	int tmpResultPathLen = strlen(tmpResultPath);
	response->globalReductionTempPath = malloc(tmpResultPathLen + 1);
	memcpy(response->globalReductionTempPath, tmpResultPath, tmpResultPathLen + 1); //tmpLocalReducePath
	offset += tmpResultPathLen + 1;
	free(tmpResultPath);
	free(tmpWorkerIP);
	return response;
}

void *deserializeYamaNotifyStageFinish(char *buffer) {
	int offset = 0, tmpLength;
	char *tmp;
	ipc_struct_yama_notify_stage_finish *stageFinish = malloc(sizeof(ipc_struct_yama_notify_stage_finish));

	tmp = strdup(buffer + offset);
	tmpLength = strlen(tmp);
	stageFinish->nodeID = malloc(tmpLength + 1);
	memcpy(stageFinish->nodeID, tmp, tmpLength + 1); //nodeID
	offset += tmpLength + 1;

	tmp = strdup(buffer + offset);
	tmpLength = strlen(tmp);
	stageFinish->tempPath = malloc(tmpLength + 1);
	memcpy(stageFinish->tempPath, tmp, tmpLength + 1); //tempPath
	offset += tmpLength + 1;

	memcpy(&(stageFinish->succeeded), buffer + offset, sizeof(char)); //succeeded

	free(tmp);
	return stageFinish;
}

// Serialization functions

// TEST_MESSAGE
char *serializeTestMessage(void *data, int *size){
	int offset;
	ipc_struct_test_message *testMessage = data;
	char *buffer = malloc(*size = strlen(testMessage->bleh) + 1 + sizeof(char));

    memcpy(buffer, testMessage->bleh, offset = strlen(testMessage->bleh)+1 );
    memcpy(buffer+offset, &testMessage->blah, sizeof(char));

	return buffer;
}

// FS_GET_FILE_INFO_REQUEST
char *serializeFSGetFileInfoRequest(void *data, int *size) {
	ipc_struct_fs_get_file_info_request *request = data;

	char *buffer = malloc(*size = strlen(request->filePath) + 1);

	memcpy(buffer, request->filePath, strlen(request->filePath) + 1);
	buffer[strlen(request->filePath)] = '\0';

	return buffer;
}

// FS_GET_FILE_INFO_RESPONSE
uint32_t getFSGetFileInfoResponseEntrySize(ipc_struct_fs_get_file_info_response_entry *entry) {
	return (sizeof(uint32_t) * 5) +
			strlen(entry->firstCopyNodeID) + 1 +
			strlen(entry->firstCopyNodeIP) + 1 +
			strlen(entry->secondCopyNodeID) + 1 +
			strlen(entry->secondCopyNodeIP) + 1;
}

uint32_t getFSGetFileInfoResponseEntriesSize(ipc_struct_fs_get_file_info_response *response) {
	int i;
	uint32_t result = 0;

	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_fs_get_file_info_response_entry *currentEntry = response->entries + i;
		result += getFSGetFileInfoResponseEntrySize(currentEntry);
	}

	return result;
}

uint32_t getFSGetFileInfoResponseSize(ipc_struct_fs_get_file_info_response *response) {
	uint32_t result = 0;

	result += sizeof(uint32_t) * 2; //entriesCount + entriesSize
	result += getFSGetFileInfoResponseEntriesSize(response);

	return result;
}

char *serializeFSGetFileInfoResponse(void *data, int *size) {
	int offset = 0, i;
	ipc_struct_fs_get_file_info_response *response = data;
	uint32_t entriesSize = getFSGetFileInfoResponseEntriesSize(response);
	char *buffer;

	buffer = malloc(*size = getFSGetFileInfoResponseSize(response));
	memcpy(buffer + offset, &(response->entriesCount), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer + offset, &entriesSize, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	response->entriesSize = entriesSize;

	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_fs_get_file_info_response_entry *currentEntry = response->entries + i;
		memcpy(buffer + offset, currentEntry->firstCopyNodeID, strlen(currentEntry->firstCopyNodeID)); //firstCopyNodeID
		offset += strlen(currentEntry->firstCopyNodeID);
		buffer[offset] = '\0';
		offset += 1;

		memcpy(buffer + offset, currentEntry->firstCopyNodeIP, strlen(currentEntry->firstCopyNodeIP)); //firstCopyNodeIP
		offset += strlen(currentEntry->firstCopyNodeIP);
		buffer[offset] = '\0';
		offset += 1;

		memcpy(buffer + offset, &(currentEntry->firstCopyNodePort), sizeof(uint32_t)); //firstCopyNodePort
		offset += sizeof(uint32_t);

		memcpy(buffer + offset, &(currentEntry->firstCopyBlockID), sizeof(uint32_t)); //firstCopyBlockID
		offset += sizeof(uint32_t);

		memcpy(buffer + offset, currentEntry->secondCopyNodeID, strlen(currentEntry->secondCopyNodeID)); //secondCopyNodeID
		offset += strlen(currentEntry->secondCopyNodeID);
		buffer[offset] = '\0';
		offset += 1;

		memcpy(buffer + offset, currentEntry->secondCopyNodeIP, strlen(currentEntry->secondCopyNodeIP)); //secondCopyNodeIP
		offset += strlen(currentEntry->secondCopyNodeIP);
		buffer[offset] = '\0';
		offset += 1;

		memcpy(buffer + offset, &(currentEntry->secondCopyNodePort), sizeof(uint32_t)); //secondCopyNodePort
		offset += sizeof(uint32_t);

		memcpy(buffer + offset, &(currentEntry->secondCopyBlockID), sizeof(uint32_t)); //secondCopyBlockID
		offset += sizeof(uint32_t);

		memcpy(buffer + offset, &(currentEntry->blockSize), sizeof(uint32_t)); //blockSize
		offset += sizeof(uint32_t);
	}

	return buffer;
}

// YAMA_START_TRANSFORMATION_REQUEST
char *serializeYAMAStartTransformationRequest(void *data, int *size) {
	ipc_struct_start_transform_reduce_request *request = data;

	char *buffer = malloc(*size = strlen(request->filePath) + 1);

	memcpy(buffer, request->filePath, strlen(request->filePath) + 1);
	buffer[strlen(request->filePath)] = '\0';

	return buffer;
}

// YAMA_START_TRANSFORMATION_RESPONSE
uint32_t getYAMAStartTransformationResponseEntrySize(ipc_struct_start_transform_reduce_response_entry *entry) {
	return strlen(entry->nodeID) + strlen(entry->workerIP) + 2 + (3 * sizeof(uint32_t)) + strlen(entry->tempPath) + 1;
}

uint32_t getYAMAStartTransformationResponseEntriesSize(ipc_struct_start_transform_reduce_response *response) {
	int i;
	uint32_t result = 0;

	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_start_transform_reduce_response_entry *currentEntry = response->entries + i;
		result += getYAMAStartTransformationResponseEntrySize(currentEntry);
	}

	return result;
}

uint32_t getYAMAStartTransformationResponseSize(ipc_struct_start_transform_reduce_response *response) {
	uint32_t result = 0;

	result += sizeof(uint32_t) * 2; //entriesCount + entriesSize
	result += getYAMAStartTransformationResponseEntriesSize(response);

	return result;
}

char *serializeYAMAStartTransformationResponse(void *data, int *size) {
	int offset = 0, i;
	ipc_struct_start_transform_reduce_response *response = data;
	uint32_t entriesSize = getYAMAStartTransformationResponseEntriesSize(response);
	char *buffer;

	buffer = malloc(*size = getYAMAStartTransformationResponseSize(response));
	memcpy(buffer + offset, &(response->entriesCount), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer + offset, &entriesSize, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	response->entriesSize = entriesSize;

	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_start_transform_reduce_response_entry *currentEntry = response->entries + i;
		memcpy(buffer + offset, currentEntry->nodeID, strlen(currentEntry->nodeID)); //nodeID
		offset += strlen(currentEntry->nodeID);
		buffer[offset] = '\0';
		offset += 1;
		memcpy(buffer + offset, currentEntry->workerIP, strlen(currentEntry->workerIP)); //workerIP
		offset += strlen(currentEntry->workerIP);
		buffer[offset] = '\0';
		offset += 1;
		memcpy(buffer + offset, &(currentEntry->workerPort), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(buffer + offset, &(currentEntry->blockID), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(buffer + offset, &(currentEntry->usedBytes), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(buffer + offset, currentEntry->tempPath, strlen(currentEntry->tempPath));
		offset += strlen(currentEntry->tempPath);
		buffer[offset] = '\0';
		offset += 1;
	}

	return buffer;
}

// MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST

uint32_t getMasterContinueWithLocalReductionRequestEntrySize(ipc_struct_master_continueWithLocalReductionRequestEntry *entry) {
	return sizeof(uint32_t) + strlen(entry->workerIP) + 1 + sizeof(uint32_t) + strlen(entry->transformTempPath) + 1 + strlen(entry->localReduceTempPath) + 1;
}

uint32_t getMasterContinueWithLocalReductionRequestEntriesSize(ipc_struct_master_continueWithLocalReductionRequest *request) {
	int i;
	uint32_t result = 0;

	for (i = 0; i < request->entriesCount; i++) {
		ipc_struct_master_continueWithLocalReductionRequestEntry *currentEntry = request->entries + i;
		result += getMasterContinueWithLocalReductionRequestEntrySize(currentEntry);
	}

	return result;
}

uint32_t getMasterContinueWithLocalReductionRequestSize(ipc_struct_master_continueWithLocalReductionRequest *request) {
	uint32_t result = 0;

	result += sizeof(uint32_t) * 2; //entriesCount + entriesSize
	result += getMasterContinueWithLocalReductionRequestEntriesSize(request);

	return result;
}

char *serializeMasterContinueWithLocalReductionRequest(void *data, int *size) {
	int offset = 0, i;
	ipc_struct_master_continueWithLocalReductionRequest *request = data;
	uint32_t entriesSize = getMasterContinueWithLocalReductionRequestEntriesSize(request);
	char *buffer;

	buffer = malloc(*size = getMasterContinueWithLocalReductionRequestSize(request));
	memcpy(buffer + offset, &(request->entriesCount), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer + offset, &entriesSize, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	request->entriesSize = entriesSize;

	for (i = 0; i < request->entriesCount; i++) {
		ipc_struct_master_continueWithLocalReductionRequestEntry *currentEntry = request->entries + i;
		memcpy(buffer + offset, currentEntry->nodeID, strlen(currentEntry->nodeID)); //nodeID
		offset += strlen(currentEntry->nodeID);
		buffer[offset] = '\0';
		offset += 1;
		memcpy(buffer + offset, currentEntry->workerIP, strlen(currentEntry->workerIP)); //workerIP
		offset += strlen(currentEntry->workerIP);
		buffer[offset] = '\0';
		offset += 1;
		memcpy(buffer + offset, &(currentEntry->workerPort), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(buffer + offset, currentEntry->transformTempPath, strlen(currentEntry->transformTempPath));
		offset += strlen(currentEntry->transformTempPath);
		buffer[offset] = '\0';
		offset += 1;
		memcpy(buffer + offset, currentEntry->localReduceTempPath, strlen(currentEntry->localReduceTempPath));
		offset += strlen(currentEntry->localReduceTempPath);
		buffer[offset] = '\0';
		offset += 1;
	}

	return buffer;
}

// MASTER_CONTINUE_WITH_GLOBAL_REDUCTION_REQUEST

uint32_t getMasterContinueWithGlobalReductionRequestEntrySize(ipc_struct_master_continueWithGlobalReductionRequestEntry *entry) {
	return sizeof(uint32_t) + strlen(entry->workerIP) + 1 + sizeof(uint32_t) + strlen(entry->localReduceTempPath) + 1 + strlen(entry->globalReduceTempPath) + 1 + sizeof(uint32_t);
}

uint32_t getMasterContinueWithGlobalReductionRequestEntriesSize(ipc_struct_master_continueWithGlobalReductionRequest *request) {
	int i;
	uint32_t result = 0;

	for (i = 0; i < request->entriesCount; i++) {
		ipc_struct_master_continueWithGlobalReductionRequestEntry *currentEntry = request->entries + i;
		result += getMasterContinueWithGlobalReductionRequestEntrySize(currentEntry);
	}

	return result;
}

uint32_t getMasterContinueWithGlobalReductionRequestSize(ipc_struct_master_continueWithGlobalReductionRequest *request) {
	uint32_t result = 0;

	result += sizeof(uint32_t) * 2; //entriesCount + entriesSize
	result += getMasterContinueWithGlobalReductionRequestEntriesSize(request);

	return result;
}

char *serializeMasterContinueWithGlobalReductionRequest(void *data, int *size) {
	int offset = 0, i;
	ipc_struct_master_continueWithGlobalReductionRequest *request = data;
	uint32_t entriesSize = getMasterContinueWithGlobalReductionRequestEntriesSize(request);
	char *buffer;

	buffer = malloc(*size = getMasterContinueWithGlobalReductionRequestSize(request));
	memcpy(buffer + offset, &(request->entriesCount), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer + offset, &entriesSize, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	request->entriesSize = entriesSize;

	for (i = 0; i < request->entriesCount; i++) {
		ipc_struct_master_continueWithGlobalReductionRequestEntry *currentEntry = request->entries + i;
		memcpy(buffer + offset, currentEntry->nodeID, strlen(currentEntry->nodeID)); //nodeID
		offset += strlen(currentEntry->nodeID);
		buffer[offset] = '\0';
		offset += 1;
		memcpy(buffer + offset, currentEntry->workerIP, strlen(currentEntry->workerIP)); //workerIP
		offset += strlen(currentEntry->workerIP);
		buffer[offset] = '\0';
		offset += 1;
		memcpy(buffer + offset, &(currentEntry->workerPort), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(buffer + offset, currentEntry->localReduceTempPath, strlen(currentEntry->localReduceTempPath));
		offset += strlen(currentEntry->localReduceTempPath);
		buffer[offset] = '\0';
		offset += 1;
		memcpy(buffer + offset, currentEntry->globalReduceTempPath, strlen(currentEntry->globalReduceTempPath));
		offset += strlen(currentEntry->globalReduceTempPath);
		buffer[offset] = '\0';
		offset += 1;
		memcpy(buffer + offset, &(currentEntry->isWorkerInCharge), sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	return buffer;
}

// MASTER_CONTINUE_WITH_FINAL_STORAGE_REQUEST

uint32_t getMasterContinueWithFinalStorageRequestSize(ipc_struct_master_continueWithFinalStorageRequest *request) {
	return strlen(request->nodeID) + 1 + strlen(request->workerIP) + 1 + sizeof(uint32_t) + strlen(request->globalReductionTempPath) + 1;
}

char *serializeMasterContinueWithFinalStorageRequest(void *data, int *size) {
	int offset = 0, i;
	ipc_struct_master_continueWithFinalStorageRequest *request = data;
	uint32_t entriesSize = getMasterContinueWithGlobalReductionRequestEntriesSize(request);
	char *buffer;

	buffer = malloc(*size = getMasterContinueWithFinalStorageRequestSize(request));
	memcpy(buffer + offset, request->nodeID, strlen(request->nodeID)); //nodeID
	offset += strlen(request->nodeID);
	buffer[offset] = '\0';
	offset += 1;
	memcpy(buffer + offset, request->workerIP, strlen(request->workerIP)); //workerIP
	offset += strlen(request->workerIP);
	buffer[offset] = '\0';
	offset += 1;
	memcpy(buffer + offset, &(request->workerPort), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer + offset, request->globalReductionTempPath, strlen(request->globalReductionTempPath));
	offset += strlen(request->globalReductionTempPath);
	buffer[offset] = '\0';
	offset += 1;

	return buffer;
}



char *serializeYamaNotifyStageFinish(void *data, int *size) {
	int offset = 0;
	ipc_struct_yama_notify_stage_finish *stageFinish = data;
	char *buffer;

	buffer = malloc(*size = strlen(stageFinish->nodeID) + strlen(stageFinish->tempPath) + 2 + sizeof(char));
	memcpy(buffer + offset, stageFinish->nodeID, strlen(stageFinish->nodeID)); //nodeID
	offset += strlen(stageFinish->nodeID);
	buffer[offset] = '\0';
	offset += 1;
	memcpy(buffer + offset, stageFinish->tempPath, strlen(stageFinish->tempPath)); //tempPath
	offset += strlen(stageFinish->tempPath);
	buffer[offset] = '\0';
	offset += 1;

	memcpy(buffer + offset, &(stageFinish->succeeded), sizeof(char)); //succeeded
	offset += sizeof(char);

	return buffer;
}

char *serializeSendFileToYama(void *data, int *size) {
	int offset = 0;
	ipc_struct_worker_file_to_yama *sendFile = data;
	char *buffer;

	buffer = malloc(*size = strlen(sendFile->file) + strlen(sendFile->pathName) + 2);
	memcpy(buffer + offset, sendFile->pathName, strlen(sendFile->pathName)); //pathName
	offset += strlen(sendFile->pathName);
	buffer[offset] = '\0';
	offset += 1;
	memcpy(buffer + offset, sendFile->file, strlen(sendFile->file)); //file
	offset += strlen(sendFile->file);
	buffer[offset] = '\0';
	offset += 1;

	return buffer;
}

void initializeSerialization() {
	serializationArray[TEST_MESSAGE] = serializeTestMessage;
	serializationArray[FS_GET_FILE_INFO_REQUEST] = serializeFSGetFileInfoRequest;
	serializationArray[FS_GET_FILE_INFO_RESPONSE] = serializeFSGetFileInfoResponse;
	serializationArray[YAMA_START_TRANSFORM_REDUCE_REQUEST] = serializeYAMAStartTransformationRequest;
	serializationArray[YAMA_START_TRANSFORM_REDUCE_RESPONSE] = serializeYAMAStartTransformationResponse;
	serializationArray[YAMA_NOTIFY_FINAL_STORAGE_FINISH] = serializeYamaNotifyStageFinish;
	serializationArray[YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH] = serializeYamaNotifyStageFinish;
	serializationArray[YAMA_NOTIFY_LOCAL_REDUCTION_FINISH] = serializeYamaNotifyStageFinish;
	serializationArray[YAMA_NOTIFY_TRANSFORM_FINISH] = serializeYamaNotifyStageFinish;
	serializationArray[MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST] = serializeMasterContinueWithLocalReductionRequest;
	serializationArray[MASTER_CONTINUE_WITH_GLOBAL_REDUCTION_REQUEST] = serializeMasterContinueWithGlobalReductionRequest;
	serializationArray[MASTER_CONTINUE_WITH_FINAL_STORAGE_REQUEST] = serializeMasterContinueWithFinalStorageRequest;
}

void initializeDeserialization () {
	deserializationArray[TEST_MESSAGE] = deserializeTestMessage;
	deserializationArray[FS_GET_FILE_INFO_REQUEST] = deserializeFSGetFileInfoRequest;
	deserializationArray[FS_GET_FILE_INFO_RESPONSE] = deserializeFSGetFileInfoResponse;
	deserializationArray[YAMA_START_TRANSFORM_REDUCE_REQUEST] = deserializeYAMAStartTransformationRequest;
	deserializationArray[YAMA_START_TRANSFORM_REDUCE_RESPONSE] = deserializeYAMAStartTransformationResponse;
	deserializationArray[YAMA_NOTIFY_FINAL_STORAGE_FINISH] = deserializeYamaNotifyStageFinish;
	deserializationArray[YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH] = deserializeYamaNotifyStageFinish;
	deserializationArray[YAMA_NOTIFY_LOCAL_REDUCTION_FINISH] = deserializeYamaNotifyStageFinish;
	deserializationArray[YAMA_NOTIFY_TRANSFORM_FINISH] = deserializeYamaNotifyStageFinish;
	deserializationArray[MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST] = deserializeMasterContinueWithLocalReductionRequest;
	deserializationArray[MASTER_CONTINUE_WITH_GLOBAL_REDUCTION_REQUEST] = deserializeMasterContinueWithGlobalReductionRequest;
	deserializationArray[MASTER_CONTINUE_WITH_FINAL_STORAGE_REQUEST] = deserializeMasterContinueWithFinalStorageRequest;
}

void serialization_initialize() {
	initializeSerialization();
	initializeDeserialization();
}
