/*
 * transform.h
 *
 *  Created on: Nov 3, 2017
 *      Author: Federico Trimboli
 */

#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include <stdint.h>

//typedef struct master_ConnectionAddress {
//	char *ip;
//	uint32_t port;
//} master_ConnectionAddress;
//
//typedef struct master_TransformRequest {
//	uint32_t block;
//	uint32_t usedBytes;
//	char *tempFilePath;
//} master_TransformRequest;
//
//typedef struct master_WorkerTransformRequests {
//	master_ConnectionAddress workerAddress;
//	master_TransformRequest *requests;
//} master_WorkerTransformRequests;
//
//void master_transform_start(master_WorkerTransformRequests *workersRequests);

typedef struct master_TransformRequest {
	char *ip;
	uint32_t port;
	uint32_t block;
	uint32_t usedBytes;
	char *tempFilePath;
} master_TransformRequest;

void master_transform_start(master_TransformRequest *requests);

#endif /* TRANSFORM_H_ */
