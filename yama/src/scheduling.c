/*
 * scheduling.c
 *
 *  Created on: Nov 21, 2017
 *      Author: Hernan Canzonetta
 */
#include "scheduling.h"

#include <commons/collections/list.h>
#include <stddef.h>

extern uint32_t scheduling_baseAvailability;
uint32_t maximumLoad = 0;
uint32_t workersList_count;
t_list *workersList; //Circular list

t_list *getCircularList(t_list *linear) {
	t_list *circular = list_create();

	int i;
	for (i = 0; i < list_size(linear); i++) {
		list_add(circular, list_get(linear, i));
	}

	if (i > 0) { //list has items
		t_link_element *last = circular->head;

		while (last->next != NULL) {
			last = last->next;
		}

		last->next = linear->head;
	}

	return circular;
}

uint32_t scheduling_getAvailability(Worker *worker) {
	return scheduling_baseAvailability + availabilityFunctions[scheduling_currentAlgorithm](worker);
}

uint32_t clock_getAvailability(Worker *worker) {
	return 0;
}

uint32_t wClock_getAvailability(Worker *worker) {
	return maximumLoad - worker->currentLoad;
}

void scheduling_initialize() {
	availabilityFunctions[CLOCK] = clock_getAvailability;
	availabilityFunctions[W_CLOCK] = wClock_getAvailability;
	workersList = list_create();
}

void scheduling_addWorker(Worker *worker) {
	list_add_in_index(workersList, workersList_count, worker);

	if (workersList_count == 0) {
		t_link_element *last = workersList->head;

		while (last->next != NULL) {
			last = last->next;
		}

		last->next = workersList->head;
	} else {
		int i = workersList_count;
		t_link_element *last = workersList->head;
		while (i--) {
			last = last->next;
		}

		last->next = workersList->head;
	}

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
