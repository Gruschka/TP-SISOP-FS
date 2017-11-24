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
		memcpy(&(currentEntry->firstCopyBlockID), buffer + offset, sizeof(uint32_t)); //firstCopyBlockID
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);

		tmp = strdup(buffer + offset);
		tmpLen = strlen(tmp);
		currentEntry->secondCopyNodeID = malloc(tmpLen + 1);
		memcpy(currentEntry->secondCopyNodeID, tmp, tmpLen + 1); //secondCopyNodeID
		entriesOffset += tmpLen + 1;
		offset += tmpLen + 1;
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
	return (sizeof(uint32_t) * 3) +
			strlen(entry->firstCopyNodeID) + 1 +
			strlen(entry->secondCopyNodeID) + 1;
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

		memcpy(buffer + offset, &(currentEntry->firstCopyBlockID), sizeof(uint32_t)); //firstCopyBlockID
		offset += sizeof(uint32_t);

		memcpy(buffer + offset, currentEntry->secondCopyNodeID, strlen(currentEntry->secondCopyNodeID)); //secondCopyNodeID
		offset += strlen(currentEntry->secondCopyNodeID);
		buffer[offset] = '\0';
		offset += 1;

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
	return sizeof(uint32_t) + strlen(entry->workerIP) + 1 + (3 * sizeof(uint32_t)) + strlen(entry->tempPath) + 1;
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

void initializeSerialization() {
	serializationArray[TEST_MESSAGE] = serializeTestMessage;
	serializationArray[FS_GET_FILE_INFO_REQUEST] = serializeFSGetFileInfoRequest;
	serializationArray[FS_GET_FILE_INFO_RESPONSE] = serializeFSGetFileInfoResponse;
	serializationArray[YAMA_START_TRANSFORM_REDUCE_REQUEST] = serializeYAMAStartTransformationRequest;
	serializationArray[YAMA_START_TRANSFORM_REDUCE_RESPONSE] = serializeYAMAStartTransformationResponse;
}

void initializeDeserialization () {
	deserializationArray[TEST_MESSAGE] = deserializeTestMessage;
	deserializationArray[FS_GET_FILE_INFO_REQUEST] = deserializeFSGetFileInfoRequest;
	deserializationArray[FS_GET_FILE_INFO_RESPONSE] = deserializeFSGetFileInfoResponse;
	deserializationArray[YAMA_START_TRANSFORM_REDUCE_REQUEST] = deserializeYAMAStartTransformationRequest;
	deserializationArray[YAMA_START_TRANSFORM_REDUCE_RESPONSE] = deserializeYAMAStartTransformationResponse;
}

void serialization_initialize() {
	initializeSerialization();
	initializeDeserialization();
}
