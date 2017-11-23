/*
 * scheduling.c
 *
 *  Created on: Nov 21, 2017
 *      Author: Hernan Canzonetta
 */
#include "scheduling.h"

#include <commons/collections/list.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>

extern uint32_t scheduling_baseAvailability;
uint32_t maximumLoad = 0;
uint32_t workersList_count;
pthread_mutex_t workersList_mutex;
t_list *workersList; //Circular list
Worker *maximumAvailabilityWorker = NULL;
uint32_t maximumAvailabilityWorkerIdx = 0;

uint32_t scheduling_getAvailability(Worker *worker) {
	uint32_t availability = scheduling_baseAvailability + workloadCalculationFunctions[scheduling_currentAlgorithm](worker);

	return availability;
}

uint32_t clock_getWorkload(Worker *worker) {
	return 0;
}

uint32_t wClock_getWorkload(Worker *worker) {
	return maximumLoad - worker->currentLoad;
}

void calculateMaximumWorkload() {
	int i;

	for (i = 0; i < workersList_count; i++) {
		Worker *worker = list_get(workersList, i);
		if (worker->currentLoad > maximumLoad)
			maximumLoad = worker->currentLoad;
	}
}

void calculateAvailability() { //calcula availability y maximumAvailabilityWorker
	int i;

	for (i = 0; i < workersList_count; i++) {
		Worker *worker = list_get(workersList, i);
		uint32_t availability = scheduling_getAvailability(worker);
		worker->availability = availability;

		int hasSameAvailabilityThanMax = maximumAvailabilityWorker->availability == worker->availability;
		int hasLessHistoricalLoadThanMax = worker->historicalLoad < maximumAvailabilityWorker->historicalLoad;

		if (maximumAvailabilityWorker == NULL ||
				maximumAvailabilityWorker->availability < worker->availability ||
				(hasSameAvailabilityThanMax && hasLessHistoricalLoadThanMax)) {
			maximumAvailabilityWorker = worker;
			maximumAvailabilityWorkerIdx = i;
		}
	}
}

void scheduling_initialize() {
	workloadCalculationFunctions[CLOCK] = clock_getWorkload;
	workloadCalculationFunctions[W_CLOCK] = wClock_getWorkload;
	workersList = list_create();
	pthread_mutex_init(&workersList_mutex, NULL);
}

void scheduling_addWorker(Worker *worker) {
	list_add_in_index(workersList, workersList_count, worker);

	int i = workersList_count;
	t_link_element *last = workersList->head;
	while (i--) {
		last = last->next;
	}

	last->next = workersList->head;

	workersList_count++;
}

Copy workerContainsBlock(Worker *worker, BlockInfo *block) {
	if (strcmp(block->firstCopyNodeID, worker->name) == 0) {
		return FIRST;
	} else if (strcmp(block->secondCopyNodeID, worker->name) == 0) {
		return SECOND;
	}

	return NONE;
}

ExecutionPlan *getExecutionPlan(FileInfo *response) {
	uint32_t blocksCount = response->entriesCount;
	ExecutionPlan *executionPlan = malloc(sizeof(ExecutionPlan));
	executionPlan->entriesCount = blocksCount;
	executionPlan->entries = malloc(sizeof(ExecutionPlanEntry) * blocksCount);

	calculateMaximumWorkload();
	calculateAvailability(); //calcula availability y maximumAvailabilityWorker

	Worker *clock = maximumAvailabilityWorker;
	uint32_t offset = maximumAvailabilityWorkerIdx;
	uint32_t baseAvailabilitySnapshot = scheduling_baseAvailability;

	int i;
	for (i = 0; i < blocksCount; i++) { // este loop por cada bloque
		int moves = 0; // cantidad de veces que movi el clock
		ExecutionPlanEntry *currentPlanEntry = executionPlan->entries + i;
		BlockInfo *blockInfo = response->entries;

		Copy copy = workerContainsBlock(clock, blockInfo);

//		if (copy != NONE && clock->availability > 0) { //si esta en el primero y tiene disponibilidad
//			clock->availability--; // se le baja disponibilidad en 1
//			clock = list_get(workersList, offset + moves); moves++; // se avanza el clock
//			currentPlanEntry->blockID = (copy == FIRST) ? blockInfo->firstCopyBlockID : blockInfo->secondCopyBlockID;
//			currentPlanEntry->workerID = (copy == FIRST) ? blockInfo->firstCopyNodeID : blockInfo->secondCopyNodeID;
//			break;
//		}
		int assigned = 0;
		while (!assigned) { // toda la vueltita bb
			clock = list_get(workersList, offset + moves);
			Copy copy = workerContainsBlock(clock, blockInfo);

			if (copy == NONE) {
				moves++;
			} else if (clock->availability > 0) { // lo encontre y tiene disponibilidad
				clock->availability--;
				clock = list_get(workersList, offset + moves); moves++;
				currentPlanEntry->blockID = (copy == FIRST) ? blockInfo->firstCopyBlockID : blockInfo->secondCopyBlockID;
				currentPlanEntry->workerID = (copy == FIRST) ? blockInfo->firstCopyNodeID : blockInfo->secondCopyNodeID;
				assigned = 1;
			} else { // tiene el bloque pero no tiene disponibilidad
				clock->availability = scheduling_baseAvailability;
				moves++;
			}

			if (moves == workersList_count && !assigned) { //es porque di toda la vuelta y no lo encontre.
				//todo: toda la vueltita sumando uase
				int i;
				for (i = 0; i < workersList_count; i++) {
					Worker *worker = list_get(workersList, i);
					worker->availability += baseAvailabilitySnapshot;
				}
			}
		}


	}

	return executionPlan;
}

//Worker *current = NULL;
//t_list *circular = getCircularList(workersList);
//
//int index = 0;
//while (current != worker) {
//	current = list_get(circular, index);
//	index++;
//}
//index--; // index es la posicion del elemento en el que arranco dentro del clock
//
//while (1) {
//
//}
