/*
 * scheduling.c
 *
 *  Created on: Nov 21, 2017
 *      Author: Hernan Canzonetta
 */
#include "scheduling.h"

#include <commons/collections/list.h>
#include <stddef.h>
#include <pthread.h>

extern uint32_t scheduling_baseAvailability;
uint32_t maximumLoad = 0;
uint32_t workersList_count;
pthread_mutex_t workersList_mutex;
t_list *workersList; //Circular list

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

void calculateMaximumWorkload() { // + MEJORAS EN EL MENU
	int i;

	for (i = 0; i < workersList_count; i++) {
		Worker *worker = list_get(workersList, i);
		if (worker->currentLoad > maximumLoad)
			maximumLoad = worker->currentLoad;
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
