/*
 * scheduling.c
 *
 *  Created on: Nov 21, 2017
 *      Author: Hernan Canzonetta
 */
#include "scheduling.h"

#include "configuration.h"
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

extern yama_configuration configuration;
extern uint32_t scheduling_baseAvailability;
uint32_t maximumLoad = 0;
uint32_t workersList_count = 0;
pthread_mutex_t workersList_mutex;
t_list *workersList; //Circular list
Worker *maximumAvailabilityWorker = NULL;
uint32_t maximumAvailabilityWorkerIdx = 0;
extern t_log *logger;

static void *circularlist_get(t_list *list, uint32_t index);

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
		Worker *worker = circularlist_get(workersList, i);
		if (worker->currentLoad > maximumLoad)
			maximumLoad = worker->currentLoad;
	}
}

void calculateAvailability() { //calcula availability y maximumAvailabilityWorker
	int i;

	for (i = 0; i < workersList_count; i++) {
		Worker *worker = circularlist_get(workersList, i);
		uint32_t availability = scheduling_getAvailability(worker);
		worker->availability = availability;

		if (maximumAvailabilityWorker == NULL ||
				maximumAvailabilityWorker->availability < worker->availability ||
				((maximumAvailabilityWorker->availability == worker->availability) && (worker->historicalLoad < maximumAvailabilityWorker->historicalLoad))) {
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

t_dictionary *getWorkersDictionary(t_list *workersList) {
	t_dictionary *result = dictionary_create();
	int i;
	for (i = 0; i < workersList_count; i++) {
		Worker *currentWorker = list_get(workersList, i);

		if (!dictionary_has_key(result, currentWorker->name))
			dictionary_put(result, currentWorker->name, currentWorker);
	}

	return result;
}

void updateWorkload(ExecutionPlan *executionPlan) {
	t_dictionary *workersDictionary = getWorkersDictionary(workersList);

	int i;
	for (i = 0; i < executionPlan->entriesCount; i++) {
		ExecutionPlanEntry *entry = executionPlan->entries + i;

		Worker *worker = dictionary_get(workersDictionary, entry->workerID);
		worker->currentLoad++;
		worker->historicalLoad++;
	}

	dictionary_destroy(workersDictionary);
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
	t_dictionary *incremented = dictionary_create();

	int moves = 0; // cantidad de veces que movi el clock

	int i;
	for (i = 0; i < blocksCount; i++) { // este loop por cada bloque
		ExecutionPlanEntry *currentPlanEntry = executionPlan->entries + i;
		BlockInfo *blockInfo = response->entries + i;
		int movesForBlock = 0;
		int assigned = 0;
		dictionary_clean(incremented);
		log_debug(logger, "Scheduling block no. %d. 1st copy: %s. 2nd copy: %s", i, blockInfo->firstCopyNodeID, blockInfo->secondCopyNodeID);
		while (!assigned) { // toda la vueltita bb
			usleep(configuration.schedulingDelay);

			clock = circularlist_get(workersList, offset + moves);
			Copy copy = workerContainsBlock(clock, blockInfo);
			log_debug(logger, "Clock: node: %s. availability: %d", clock->name, clock->availability);

			if (clock->availability > 0) {
				if (copy == FIRST || copy == SECOND) {
					// lo encontre y tiene disponibilidad
					log_debug(logger, "Tiene el bloque y disponibilidad. Asignado");
					clock->availability--;
					clock = circularlist_get(workersList, offset + moves); moves++;
					currentPlanEntry->blockID = (copy == FIRST) ? blockInfo->firstCopyBlockID : blockInfo->secondCopyBlockID;
					currentPlanEntry->workerID = (copy == FIRST) ? strdup(blockInfo->firstCopyNodeID) : strdup(blockInfo->secondCopyNodeID);
					currentPlanEntry->usedBytes = blockInfo->blockSize;
					assigned = 1;
				} else {
					moves++;
					log_debug(logger, "No tiene el bloque, sigo avanzando");
				}
			} else {
				moves++;
				clock->availability++;
				dictionary_put(incremented, clock->name, 1);
			}

			movesForBlock++;

			if (movesForBlock % workersList_count == 0 && !assigned) { //es porque di toda la vuelta y no lo encontre.
				log_debug(logger, "[scheduling] Di toda la vuelta y no lo encontre. Agregando 1 de disp a todos los workers");
				int i;
				for (i = 0; i < workersList_count; i++) {
					Worker *worker = circularlist_get(workersList, i);
					// si no lo incremente antes, ahora si
					if (!dictionary_has_key(incremented, worker->name))
						worker->availability += baseAvailabilitySnapshot;
				}
			}
		}


	}

	return executionPlan;
}

static void *circularlist_get(t_list *list, uint32_t index) {
	t_link_element *element = list->head;
	while (index--) {
		element = element->next;
	}
	return element->data;
}
