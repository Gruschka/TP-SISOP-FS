/*
 * scheduling.h
 *
 *  Created on: Nov 21, 2017
 *      Author: Hernan Canzonetta
 */

#ifndef SCHEDULING_H_
#define SCHEDULING_H_

#include <stdint.h>

#define ALGORITHMS_COUNT 2

typedef enum {
	CLOCK, W_CLOCK
} scheduling_algorithm;

typedef struct {
	char name;
	uint32_t historicalLoad;
	uint32_t currentLoad;
} Worker;
typedef uint32_t (*AvailabilityFunction)(Worker *);

AvailabilityFunction availabilityFunctions[ALGORITHMS_COUNT];
scheduling_algorithm scheduling_currentAlgorithm;
uint32_t scheduling_getAvailability(Worker *);
void scheduling_addWorker(Worker *worker);

#endif /* SCHEDULING_H_ */
