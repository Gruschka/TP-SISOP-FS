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

// FS_GET_FILE_INFO
void *deserializeFSGetFileInfo(char *buffer) {
	ipc_struct_fs_get_file_info *getFileInfo = malloc(sizeof(ipc_struct_fs_get_file_info));
	getFileInfo->filePath = strdup(buffer);
	return getFileInfo;
}

// YAMA_START_TRANSFORMATION_REQUEST
void *deserializeYAMAStartTransformationRequest(char *buffer) {
	ipc_struct_start_transformation_request *request = malloc(sizeof(ipc_struct_start_transformation_request));
	request->filePath = strdup(buffer);
	return request;
}

// YAMA_START_TRANSFORMATION_RESPONSE
void *deserializeYAMAStartTransformationResponse(char *buffer) {
	int offset = 0, i;
	ipc_struct_start_transformation_response *response = malloc(sizeof(ipc_struct_start_transformation_response));
	memcpy(&(response->entriesCount), buffer + offset, sizeof(uint32_t)); sizeof(uint32_t);
	offset += sizeof(uint32_t);
	memcpy(&(response->entriesSize), buffer + offset, sizeof(uint32_t)); sizeof(uint32_t);
	offset += sizeof(uint32_t);

	response->entries = malloc(response->entriesSize);
	int entriesOffset = 0;
	void *entriesPtr = response->entries;
	for (i = 0; i < response->entriesCount; i++) {
		memcpy(entriesPtr + entriesOffset, buffer + offset, sizeof(uint32_t)); //nodeID
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);
		char *tmpConnectionString = strdup(buffer + offset);
		int tmpConnectionStringLen = strlen(tmpConnectionString);
		memcpy(entriesPtr + entriesOffset, tmpConnectionString, tmpConnectionStringLen + 1); //connectionString
		entriesOffset += tmpConnectionStringLen + 1;
		offset += tmpConnectionStringLen + 1;
		memcpy(entriesPtr + entriesOffset, buffer + offset, sizeof(uint32_t)); //blockID
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);
		memcpy(entriesPtr + entriesOffset, buffer + offset, sizeof(uint32_t)); //usedBytes
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);
		free(tmpConnectionString);
	}
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

// FS_GET_FILE_INFO
char *serializeFSGetFileInfo(void *data, int *size) {
	ipc_struct_fs_get_file_info *getFileInfo = data;

	char *buffer = malloc(*size = strlen(getFileInfo->filePath) + 1);

	memcpy(buffer, getFileInfo->filePath, strlen(getFileInfo->filePath) + 1);
	buffer[strlen(getFileInfo->filePath)] = '\0';

	return buffer;
}

// YAMA_START_TRANSFORMATION_REQUEST
char *serializeYAMAStartTransformationRequest(void *data, int *size) {
	ipc_struct_start_transformation_request *request = data;

	char *buffer = malloc(*size = strlen(request->filePath) + 1);

	memcpy(buffer, request->filePath, strlen(request->filePath) + 1);
	buffer[strlen(request->filePath)] = '\0';

	return buffer;
}

// YAMA_START_TRANSFORMATION_RESPONSE
uint32_t getYAMAStartTransformationResponseEntrySize(ipc_struct_start_transformation_response_entry *entry) {
	return sizeof(uint32_t) + strlen(entry->connectionString) + 1 + (2 * sizeof(uint32_t)) + strlen(entry->tempPath) + 1;
}

uint32_t getYAMAStartTransformationResponseEntriesSize(ipc_struct_start_transformation_response *response) {
	int i;
	uint32_t result = 0;

	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_start_transformation_response_entry *currentEntry = response->entries + i;
		result += getYAMAStartTransformationResponseEntrySize(currentEntry);
	}

	return result;
}

uint32_t getYAMAStartTransformationResponseSize(ipc_struct_start_transformation_response *response) {
	uint32_t result = 0;

	result += sizeof(uint32_t) * 2; //entriesCount + entriesSize
	result += getYAMAStartTransformationResponseEntriesSize(response);

	return result;
}

char *serializeYAMAStartTransformationResponse(void *data, int *size) {
	int offset = 0, i;
	ipc_struct_start_transformation_response *response = data;
	uint32_t entriesSize = getYAMAStartTransformationResponseEntriesSize(response);
	char *buffer;

	buffer = malloc(*size = getYAMAStartTransformationResponseSize(response));
	memcpy(buffer + offset, &(response->entriesCount), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(buffer + offset, &entriesSize, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	response->entriesSize = entriesSize;

	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_start_transformation_response_entry *currentEntry = response->entries + i;
		memcpy(buffer + offset, &(currentEntry->nodeID), sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(buffer + offset, currentEntry->connectionString, strlen(currentEntry->connectionString));
		offset += strlen(currentEntry->connectionString);
		buffer[offset] = '\0';
		offset += 1;
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
	serializationArray[FS_GET_FILE_INFO] = serializeFSGetFileInfo;
	serializationArray[YAMA_START_TRANSFORMATION_REQUEST] = serializeYAMAStartTransformationRequest;
	serializationArray[YAMA_START_TRANSFORMATION_RESPONSE] = serializeYAMAStartTransformationResponse;
}

void initializeDeserialization () {
	deserializationArray[TEST_MESSAGE] = deserializeTestMessage;
	deserializationArray[FS_GET_FILE_INFO] = deserializeFSGetFileInfo;
	deserializationArray[YAMA_START_TRANSFORMATION_REQUEST] = deserializeYAMAStartTransformationRequest;
	deserializationArray[YAMA_START_TRANSFORMATION_RESPONSE] = deserializeYAMAStartTransformationResponse;
}

void serialization_initialize() {
	initializeSerialization();
	initializeDeserialization();
}
