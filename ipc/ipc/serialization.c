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

void *deserializeFSGetFileInfoRequest2(char *buffer) {
	ipc_struct_fs_get_file_info_request *request = malloc(sizeof(ipc_struct_fs_get_file_info_request));
	request->filePath = strdup(buffer);
	return request;
}
void *deserializeWorkerHandshakeToFS(void *buffer) {
	ipc_struct_worker_handshake_to_fs *handshake = malloc(sizeof(ipc_struct_worker_handshake_to_fs));
	int offset = 0;

	memcpy(&handshake->status,buffer+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	return handshake;
}

void *deserializeSendFileToFS(void *buffer) {
	char *tmp; int tmpLen;
	ipc_struct_worker_file_to_fs *sendFile = malloc(sizeof(ipc_struct_worker_file_to_fs));
	int offset = 0;

	tmp = strdup(buffer + offset);
	tmpLen = strlen(tmp);
	sendFile->pathName = malloc(tmpLen + 1);
	memcpy(sendFile->pathName, tmp, tmpLen + 1); //pathName
	offset += tmpLen + 1;
	free(tmp);

	tmp = strdup(buffer + offset);
	tmpLen = strlen(tmp);
	sendFile->content = malloc(tmpLen + 1);
	memcpy(sendFile->content, tmp, tmpLen + 1); //pathName
	offset += tmpLen + 1;
	free(tmp);

	return sendFile;
}

// FS_GET_FILE_INFO_RESPONSE
void *deserializeFSGetFileInfoResponse2(char *buffer) {
	int offset = 0;
	ipc_struct_fs_get_file_info_response_2 *response = malloc(sizeof(ipc_struct_fs_get_file_info_response_2));

	memcpy(&response->amountOfblocks,buffer+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	response->amountOfCopiesPerBlock = calloc(response->amountOfblocks, sizeof(uint32_t));
	memcpy(response->amountOfCopiesPerBlock,buffer+offset,sizeof(uint32_t)*response->amountOfblocks);
	offset+=sizeof(uint32_t)*response->amountOfblocks;

	int blockIterator = 0;
	int copyIterator = 0;
	response->blocks = calloc(response->amountOfblocks,sizeof(t_block));

	for (blockIterator = 0; blockIterator < response->amountOfblocks; blockIterator++) {
		memcpy(&((response->blocks)[blockIterator].copies),buffer+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);
		uint32_t numberOfCopies = (response->blocks)[blockIterator].copies;

		memcpy(&((response->blocks)[blockIterator].blockSize),buffer+offset,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		(response->blocks)[blockIterator].blockIds = calloc(numberOfCopies, sizeof(uint32_t));
		memcpy((response->blocks)[blockIterator].blockIds,buffer+offset,sizeof(uint32_t) * numberOfCopies);
		offset+=sizeof(uint32_t) * numberOfCopies;

		(response->blocks)[blockIterator].ports = calloc(numberOfCopies, sizeof(uint32_t));
		memcpy((response->blocks)[blockIterator].ports,buffer+offset,sizeof(uint32_t) * numberOfCopies);
		offset+=sizeof(uint32_t) * numberOfCopies;

		(response->blocks)[blockIterator].copyIds = calloc(numberOfCopies, sizeof(uint32_t));
		memcpy((response->blocks)[blockIterator].copyIds,buffer+offset,sizeof(uint32_t) * numberOfCopies);
		offset+=sizeof(uint32_t) * numberOfCopies;

		(response->blocks)[blockIterator].nodeIds = calloc(numberOfCopies, sizeof(char *));
		(response->blocks)[blockIterator].nodeIps = calloc(numberOfCopies, sizeof(char *));
		for (copyIterator = 0; copyIterator < numberOfCopies; copyIterator++) {

			(response->blocks)[blockIterator].nodeIds[copyIterator] = malloc(strlen(buffer+offset)+1);
			memcpy((response->blocks)[blockIterator].nodeIds[copyIterator],buffer+offset,strlen(buffer+offset)+1);
			offset+=strlen(buffer+offset)+1;

			(response->blocks)[blockIterator].nodeIps[copyIterator] = malloc(strlen(buffer+offset)+1);
			memcpy((response->blocks)[blockIterator].nodeIps[copyIterator],buffer+offset,strlen(buffer+offset)+1);
			offset+=strlen(buffer+offset)+1;
		}

	}

	return response;
}
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
		free(tmp);

		tmp = strdup(buffer + offset);
		tmpLen = strlen(tmp);
		currentEntry->firstCopyNodeIP = malloc(tmpLen + 1);
		memcpy(currentEntry->firstCopyNodeIP, tmp, tmpLen + 1); //firstCopyNodeIP
		entriesOffset += tmpLen + 1;
		offset += tmpLen + 1;
		free(tmp);

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
		free(tmp);

		tmp = strdup(buffer + offset);
		tmpLen = strlen(tmp);
		currentEntry->secondCopyNodeIP = malloc(tmpLen + 1);
		memcpy(currentEntry->secondCopyNodeIP, tmp, tmpLen + 1); //secondCopyNodeIP
		entriesOffset += tmpLen + 1;
		offset += tmpLen + 1;
		free(tmp);

		memcpy(&(currentEntry->secondCopyNodePort), buffer + offset, sizeof(uint32_t)); //secondCopyNodePort
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);

		memcpy(&(currentEntry->secondCopyBlockID), buffer + offset, sizeof(uint32_t)); //secondCopyBlockID
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);

		memcpy(&(currentEntry->blockSize), buffer + offset, sizeof(uint32_t)); //blockSize
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);
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
		currentEntry->nodeID = malloc(tmpNodeIDStringLen + 1);
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

		char *tmpNodeID = strdup(buffer + offset);
		int tmpNodeIDStringLen = strlen(tmpNodeID);
		currentEntry->nodeID = malloc(tmpNodeIDStringLen + 1);
		memcpy(currentEntry->nodeID, tmpNodeID, tmpNodeIDStringLen + 1); //nodeID
		entriesOffset += tmpNodeIDStringLen + 1;
		offset += tmpNodeIDStringLen + 1;

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
		free(tmpNodeID);
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
	ipc_struct_master_continueWithGlobalReductionRequestEntry *entries = malloc(sizeof(ipc_struct_master_continueWithGlobalReductionRequestEntry) * response->entriesCount);
	for (i = 0; i < response->entriesCount; i++) {
		ipc_struct_master_continueWithGlobalReductionRequestEntry *currentEntry = entries + i;

		char *tmpNodeID = strdup(buffer + offset);
		int tmpNodeIDStringLen = strlen(tmpNodeID);
		currentEntry->nodeID = malloc(tmpNodeIDStringLen + 1);
		memcpy(currentEntry->nodeID, tmpNodeID, tmpNodeIDStringLen + 1); //nodeID
		entriesOffset += tmpNodeIDStringLen + 1;
		offset += tmpNodeIDStringLen + 1;

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

		memcpy(&(currentEntry->isWorkerInCharge), buffer + offset, sizeof(uint32_t)); //workerInCharge
		entriesOffset += sizeof(uint32_t);
		offset += sizeof(uint32_t);

		free(tmpGlobalReducePath);
		free(tmpLocalReducePath);
		free(tmpWorkerIP);
		free(tmpNodeID);
	}
	response->entries = entries;
	return response;
}

// MASTER_CONTINUE_WITH_FINAL_STORAGE_REQUEST
void *deserializeMasterContinueWithFinalStorageRequest(char *buffer) {
	int offset = 0;
	ipc_struct_master_continueWithFinalStorageRequest *response = malloc(sizeof(ipc_struct_master_continueWithFinalStorageRequest));

	char *tmpNodeID = strdup(buffer + offset);
	int tmpNodeIDStringLen = strlen(tmpNodeID);
	response->nodeID = malloc(tmpNodeIDStringLen + 1);
	memcpy(response->nodeID, tmpNodeID, tmpNodeIDStringLen + 1); //nodeID
	offset += tmpNodeIDStringLen + 1;

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
	free(tmpNodeID);
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
	free(tmp);

	tmp = strdup(buffer + offset);
	tmpLength = strlen(tmp);
	stageFinish->tempPath = malloc(tmpLength + 1);
	memcpy(stageFinish->tempPath, tmp, tmpLength + 1); //tempPath
	offset += tmpLength + 1;
	free(tmp);

	memcpy(&(stageFinish->succeeded), buffer + offset, sizeof(char)); //succeeded

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
char *serializeFSGetFileInfoRequest2(void *data, int *size) {
	ipc_struct_fs_get_file_info_request *request = data;

	char *buffer = malloc(*size = strlen(request->filePath) + 1);

	memcpy(buffer, request->filePath, strlen(request->filePath) + 1);
	buffer[strlen(request->filePath)] = '\0';

	return buffer;
}

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
int getSizeOfBlockArrayOfBlockCount(t_block *block, int blockCount){
	int blockIterator = 0;
	int copyIterator = 0;
	int totalArraySize = 0;
	for (blockIterator = 0; blockIterator < blockCount; blockIterator++) {
		totalArraySize+= sizeof(uint32_t); //blockSize
		totalArraySize+= sizeof(uint32_t); //copies
		totalArraySize+= sizeof(uint32_t) * block[blockIterator].copies; // blockIds
		totalArraySize+= sizeof(uint32_t) * block[blockIterator].copies; // ports
		totalArraySize+= sizeof(uint32_t) * block[blockIterator].copies; // copyIds
		for (copyIterator = 0; copyIterator < block[blockIterator].copies; copyIterator++) {
			totalArraySize+= strlen(block[blockIterator].nodeIds[copyIterator]) +1; //node Ids
			totalArraySize+= strlen(block[blockIterator].nodeIps[copyIterator]) +1; //node Ips
		}
	}
	return totalArraySize;
}

char *serializeFSGetFileInfoResponse2(void *data, int *size) {
	int offset = 0, i;
	ipc_struct_fs_get_file_info_response_2 *response = data;
	int sizeOfBlockArray = getSizeOfBlockArrayOfBlockCount(response->blocks,response->amountOfblocks);
	int sizeOfCopiesPerBlockArray = sizeof(uint32_t) * response->amountOfblocks;
	t_block blah;
	char *buffer = malloc(*size = sizeof(uint32_t) + sizeOfBlockArray + sizeOfCopiesPerBlockArray);
	uint32_t *copiesPerBlock = malloc(sizeOfCopiesPerBlockArray);

	int blockIterator = 0;
	int copyIterator = 0;
	int bufferOffset = 0;
	int copiesPerBlockOffset = 0;

	memcpy(buffer+bufferOffset,&(response->amountOfblocks),sizeof(uint32_t));
	bufferOffset+=sizeof(uint32_t);
	copiesPerBlockOffset = bufferOffset;
	bufferOffset+=sizeOfCopiesPerBlockArray; //dejo lugar para escribir esto despues

	for (blockIterator = 0; blockIterator < response->amountOfblocks; blockIterator++) {
		uint32_t numberOfCopies = (response->blocks)[blockIterator].copies;
		memcpy(copiesPerBlock+blockIterator,&((response->blocks)[blockIterator].copies),sizeof(uint32_t));

		memcpy(buffer+bufferOffset,&((response->blocks)[blockIterator].copies),sizeof(uint32_t));
		bufferOffset+=sizeof(uint32_t);

		memcpy(buffer+bufferOffset,&((response->blocks)[blockIterator].blockSize),sizeof(uint32_t));
		bufferOffset+=sizeof(uint32_t);

		memcpy(buffer+bufferOffset,(response->blocks)[blockIterator].blockIds,sizeof(uint32_t) * numberOfCopies);
		bufferOffset+=sizeof(uint32_t)*numberOfCopies;

		memcpy(buffer+bufferOffset,(response->blocks)[blockIterator].ports,sizeof(uint32_t) * numberOfCopies);
		bufferOffset+=sizeof(uint32_t)*(response->blocks)[blockIterator].copies;

		memcpy(buffer+bufferOffset,(response->blocks)[blockIterator].copyIds,sizeof(uint32_t) * numberOfCopies);
		bufferOffset+=sizeof(uint32_t)*(response->blocks)[blockIterator].copies;

		for (copyIterator = 0; copyIterator < numberOfCopies; copyIterator++) {
			memcpy(buffer+bufferOffset,(response->blocks)[blockIterator].nodeIds[copyIterator],strlen((response->blocks)[blockIterator].nodeIds[copyIterator])+1);
			bufferOffset+=strlen((response->blocks)[blockIterator].nodeIds[copyIterator])+1;

			memcpy(buffer+bufferOffset,(response->blocks)[blockIterator].nodeIps[copyIterator],strlen((response->blocks)[blockIterator].nodeIps[copyIterator])+1);
			bufferOffset+=strlen((response->blocks)[blockIterator].nodeIps[copyIterator])+1;
		}
	}

	memcpy(buffer+copiesPerBlockOffset,copiesPerBlock,sizeOfCopiesPerBlockArray);



	return buffer;
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
	return strlen(entry->nodeID) + 1 + strlen(entry->workerIP) + 1 + sizeof(uint32_t) +
			strlen(entry->transformTempPath) + 1 + strlen(entry->localReduceTempPath) + 1;
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
	return 	strlen(entry->nodeID) + 1 +
			strlen(entry->workerIP) + 1 +
			sizeof(uint32_t) +
			strlen(entry->localReduceTempPath) + 1 +
			strlen(entry->globalReduceTempPath) + 1 +
			sizeof(uint32_t);
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

char *serializeWorkerHandshakeToFS(void *data, int *size) {
	int offset = 0;
	ipc_struct_worker_handshake_to_fs *handshake = data;
	char *buffer = malloc(*size = sizeof(uint32_t));

	memcpy(buffer + offset, &handshake->status, sizeof(uint32_t)); //pathName
	offset += sizeof(uint32_t);

	return buffer;
}

char *serializeSendFileToFS(void *data, int *size) {
	int offset = 0;
	ipc_struct_worker_file_to_fs *sendFile = data;
	char *buffer;

	buffer = malloc(*size = strlen(sendFile->pathName) + 1 + strlen(sendFile->content) + 1);
	memcpy(buffer + offset, sendFile->pathName, strlen(sendFile->pathName)); //pathName
	offset += strlen(sendFile->pathName);
	buffer[offset] = '\0';
	offset += 1;

	memcpy(buffer + offset, sendFile->content, strlen(sendFile->content)); //pathName
	offset += strlen(sendFile->content);
	buffer[offset] = '\0';
	offset += 1;

	return buffer;
}

// DATANODE OPERATIONS
char *serializeSendDatanodeHandshakeToFS(void *data, int *size) {
	int offset = 0;
	ipc_struct_datanode_handshake_to_fs *sendHandshake = data;
	*size = sizeof(uint32_t) * 3 + sendHandshake->nameLength + 1;
	void *buffer = malloc(*size);

	memcpy(buffer+offset,&sendHandshake->nameLength,sizeof(uint32_t)); //nameLength
	offset+=sizeof(uint32_t);

	memcpy(buffer+offset,sendHandshake->nodeName,sendHandshake->nameLength+1); //name
	offset+=sendHandshake->nameLength + 1;

	memcpy(buffer+offset,&sendHandshake->amountOfBlocks,sizeof(uint32_t)); //block amount
	offset+=sizeof(uint32_t);

	memcpy(buffer+offset,&sendHandshake->portNumber,sizeof(uint32_t)); //portno
	offset+=sizeof(uint32_t);

	return buffer;
}

void *deserializeSendDatanodeHandshakeToFS(char *buffer) {
	int offset = 0;
	ipc_struct_datanode_handshake_to_fs *response = malloc(sizeof(ipc_struct_datanode_handshake_to_fs));

	memcpy(&response->nameLength,buffer+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	response->nodeName = malloc(response->nameLength+1);

	memcpy(response->nodeName,buffer+offset,response->nameLength+1);
	offset+=response->nameLength+1;

	memcpy(&response->amountOfBlocks,buffer+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	memcpy(&response->portNumber,buffer+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	return response;
}

char *serializeSendDatanodeHandshakeToFSResponse(void *data, int *size) {
	int offset = 0;
	ipc_struct_datanode_handshake_response *sendHandshake = data;
	char *buffer = malloc(*size = sizeof(uint32_t));

	memcpy(buffer+offset,&sendHandshake->status,sizeof(uint32_t)); //status
	offset+=sizeof(uint32_t);


	return buffer;
}

void *deserializeDatanodeHandshakeToFSResponse(char *buffer) {
	int offset = 0;
	ipc_struct_datanode_handshake_response *response = malloc(sizeof(ipc_struct_datanode_handshake_response));

	memcpy(&response->status,buffer+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	return response;
}

void *serializeDatanodeWriteBlockRequest(void *data, int *size) {
	int offset = 0;
	ipc_struct_datanode_write_block_request *request = data;
	void *buffer = malloc(*size = (1024*1024) + sizeof(uint32_t)); //1024^2 = block size

	memcpy(buffer+offset,request->buffer,1024*1024);
	offset+=1024*1024;

	memcpy(buffer+offset,&request->blockNumber,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	return buffer;
}

void *deserializeDatanodeWriteBlockRequest(char *buffer) {
	int offset = 0;
	ipc_struct_datanode_write_block_request *request = malloc(sizeof(ipc_struct_datanode_write_block_request));
	request->buffer = malloc(1024*1024);

	memcpy(request->buffer,buffer+offset,1024*1024);
	offset+=1024*1024;

	memcpy(&request->blockNumber,buffer+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	return request;
}

void *serializeDatanodeReadBlockRequest(void *data, int *size) {
	int offset = 0;
	ipc_struct_datanode_read_block_request *request = data;
	void *buffer = malloc(*size = sizeof(uint32_t));

	memcpy(buffer+offset,&request->blockNumber,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	return buffer;
}

void *deserializeDatanodeReadBlockRequest(char *buffer) {
	int offset = 0;
	ipc_struct_datanode_read_block_request *request = malloc(sizeof(ipc_struct_datanode_read_block_request));

	memcpy(&request->blockNumber,buffer+offset,sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	return request;
}

void *serializeDatanodeReadBlockResponse(void *data, int *size) {
	int offset = 0;
	ipc_struct_datanode_read_block_response *response = data;
	void *buffer = malloc(*size = 1024*1024);

	memcpy(buffer+offset,response->buffer,1024*1024);
	offset+=1024*1024;

	return buffer;
}

void *deserializeDatanodeReadBlockResponse(char *buffer) {
	int offset = 0;
	ipc_struct_datanode_read_block_response *response = malloc(sizeof(ipc_struct_datanode_read_block_response));
	response->buffer = malloc(1024*1024);

	memcpy(response->buffer,buffer+offset,1024*1024);
	offset+=1024*1024;

	return response;
}

void initializeSerialization() {
	serializationArray[TEST_MESSAGE] = serializeTestMessage;
	serializationArray[FS_GET_FILE_INFO_REQUEST] = serializeFSGetFileInfoRequest;
	serializationArray[FS_GET_FILE_INFO_RESPONSE] = serializeFSGetFileInfoResponse;
	serializationArray[FS_GET_FILE_INFO_REQUEST_2] = serializeFSGetFileInfoRequest2;
	serializationArray[FS_GET_FILE_INFO_RESPONSE_2] = serializeFSGetFileInfoResponse2;
	serializationArray[YAMA_START_TRANSFORM_REDUCE_REQUEST] = serializeYAMAStartTransformationRequest;
	serializationArray[YAMA_START_TRANSFORM_REDUCE_RESPONSE] = serializeYAMAStartTransformationResponse;
	serializationArray[YAMA_NOTIFY_FINAL_STORAGE_FINISH] = serializeYamaNotifyStageFinish;
	serializationArray[YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH] = serializeYamaNotifyStageFinish;
	serializationArray[YAMA_NOTIFY_LOCAL_REDUCTION_FINISH] = serializeYamaNotifyStageFinish;
	serializationArray[YAMA_NOTIFY_TRANSFORM_FINISH] = serializeYamaNotifyStageFinish;
	serializationArray[MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST] = serializeMasterContinueWithLocalReductionRequest;
	serializationArray[MASTER_CONTINUE_WITH_GLOBAL_REDUCTION_REQUEST] = serializeMasterContinueWithGlobalReductionRequest;
	serializationArray[MASTER_CONTINUE_WITH_FINAL_STORAGE_REQUEST] = serializeMasterContinueWithFinalStorageRequest;
	serializationArray[WORKER_SEND_FILE_TO_FS] = serializeSendFileToFS;
	serializationArray[WORKER_HANDSHAKE_TO_FS] = serializeWorkerHandshakeToFS;
	serializationArray[DATANODE_HANDSHAKE_REQUEST] = serializeSendDatanodeHandshakeToFS;
	serializationArray[DATANODE_HANDSHAKE_RESPONSE] = serializeSendDatanodeHandshakeToFSResponse;
	serializationArray[DATANODE_WRITE_BLOCK_REQUEST] = serializeDatanodeWriteBlockRequest;
	serializationArray[DATANODE_READ_BLOCK_REQUEST] = serializeDatanodeReadBlockRequest;
	serializationArray[DATANODE_READ_BLOCK_RESPONSE] = serializeDatanodeReadBlockResponse;



}

void initializeDeserialization () {
	deserializationArray[TEST_MESSAGE] = deserializeTestMessage;
	deserializationArray[FS_GET_FILE_INFO_REQUEST] = deserializeFSGetFileInfoRequest;
	deserializationArray[FS_GET_FILE_INFO_RESPONSE] = deserializeFSGetFileInfoResponse;
	deserializationArray[FS_GET_FILE_INFO_REQUEST_2] = deserializeFSGetFileInfoRequest2;
	deserializationArray[FS_GET_FILE_INFO_RESPONSE_2] = deserializeFSGetFileInfoResponse2;
	deserializationArray[YAMA_START_TRANSFORM_REDUCE_REQUEST] = deserializeYAMAStartTransformationRequest;
	deserializationArray[YAMA_START_TRANSFORM_REDUCE_RESPONSE] = deserializeYAMAStartTransformationResponse;
	deserializationArray[YAMA_NOTIFY_FINAL_STORAGE_FINISH] = deserializeYamaNotifyStageFinish;
	deserializationArray[YAMA_NOTIFY_GLOBAL_REDUCTION_FINISH] = deserializeYamaNotifyStageFinish;
	deserializationArray[YAMA_NOTIFY_LOCAL_REDUCTION_FINISH] = deserializeYamaNotifyStageFinish;
	deserializationArray[YAMA_NOTIFY_TRANSFORM_FINISH] = deserializeYamaNotifyStageFinish;
	deserializationArray[MASTER_CONTINUE_WITH_LOCAL_REDUCTION_REQUEST] = deserializeMasterContinueWithLocalReductionRequest;
	deserializationArray[MASTER_CONTINUE_WITH_GLOBAL_REDUCTION_REQUEST] = deserializeMasterContinueWithGlobalReductionRequest;
	deserializationArray[MASTER_CONTINUE_WITH_FINAL_STORAGE_REQUEST] = deserializeMasterContinueWithFinalStorageRequest;
	deserializationArray[WORKER_SEND_FILE_TO_FS] = deserializeSendFileToFS;
	deserializationArray[WORKER_HANDSHAKE_TO_FS] = deserializeWorkerHandshakeToFS;
	deserializationArray[DATANODE_HANDSHAKE_REQUEST] = deserializeSendDatanodeHandshakeToFS;
	deserializationArray[DATANODE_HANDSHAKE_RESPONSE] = deserializeDatanodeHandshakeToFSResponse;
	deserializationArray[DATANODE_WRITE_BLOCK_REQUEST] = deserializeDatanodeWriteBlockRequest;
	deserializationArray[DATANODE_READ_BLOCK_REQUEST] = deserializeDatanodeReadBlockRequest;
	deserializationArray[DATANODE_READ_BLOCK_RESPONSE] = deserializeDatanodeReadBlockResponse;
}

void serialization_initialize() {
	initializeSerialization();
	initializeDeserialization();
}
