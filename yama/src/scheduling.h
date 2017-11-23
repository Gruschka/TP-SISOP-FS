/*
 * scheduling.h
 *
 *  Created on: Nov 21, 2017
 *      Author: Hernan Canzonetta
 */

#ifndef SCHEDULING_H_
#define SCHEDULING_H_

#include <stdint.h>
#include <ipc/serialization.h>

#define ALGORITHMS_COUNT 2

typedef struct {
	char *workerID;
	uint32_t blockID;
} ExecutionPlanEntry;

typedef struct {
	uint32_t entriesCount;
	ExecutionPlanEntry *entries;
} ExecutionPlan;

typedef enum {
	CLOCK, W_CLOCK
} scheduling_algorithm;

typedef struct {
	char name;
	uint32_t historicalLoad;
	uint32_t currentLoad;
	uint32_t availability;
} Worker;

typedef uint32_t (*WorkloadCalculationFunction)(Worker *);

WorkloadCalculationFunction workloadCalculationFunctions[ALGORITHMS_COUNT];
scheduling_algorithm scheduling_currentAlgorithm;
uint32_t scheduling_getAvailability(Worker *);
void scheduling_addWorker(Worker *worker);

typedef ipc_struct_fs_get_file_info_response FileInfo;
typedef ipc_struct_fs_get_file_info_response_entry BlockInfo;
ExecutionPlan *getExecutionPlan(FileInfo *response);

#endif /* SCHEDULING_H_ */
