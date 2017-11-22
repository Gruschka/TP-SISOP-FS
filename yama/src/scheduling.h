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
	uint32_t totalTasksCount;
	uint32_t availability;
} worker;
typedef uint32_t (*AvailabilityFunction)(worker *);

AvailabilityFunction availabilityFunctions[ALGORITHMS_COUNT];
scheduling_algorithm scheduling_currentAlgorithm;
uint32_t scheduling_getAvailability(worker *);
void scheduling_addWorker(worker *worker);

#endif /* SCHEDULING_H_ */