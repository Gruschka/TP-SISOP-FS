/*
 * scheduling.c
 *
 *  Created on: Nov 21, 2017
 *      Author: Hernan Canzonetta
 */
#include "scheduling.h"
#include <commons/collections/list.h>

extern uint32_t scheduling_baseAvailability;
t_list *workersList; //Circular list

uint32_t scheduling_getAvailability(worker *worker) {
	return scheduling_baseAvailability + availabilityFunctions[scheduling_currentAlgorithm](worker);
}

uint32_t clock_getAvailability(worker *worker) {
	return 0;
}

uint32_t wClock_getAvailability(worker *worker) {
	return 1;
}

void scheduling_initialize() {
	availabilityFunctions[CLOCK] = clock_getAvailability;
	availabilityFunctions[W_CLOCK] = wClock_getAvailability;
	workersList = list_create();
}

void scheduling_addWorker(worker *worker) {
	list_add(workersList, worker);

//	int i;
//	t_link_element *element = workersList.head;
//	for (i = 0; i < workersList->elements_count; i++) {
//		element = element->next;
//	}
}
