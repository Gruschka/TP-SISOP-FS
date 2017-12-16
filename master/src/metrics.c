/*
 * metrics.c
 *
 *  Created on: Dec 16, 2017
 *      Author: Federico Trimboli
 */

#include "metrics.h"
#include <pthread.h>

pthread_mutex_t numberOfTransformTasksRanMutex;
int numberOfTransformTasksRan;
double sumOfTransformTasksDurations;
pthread_mutex_t numberOfCurrentRunningTransformsMutex;
int numberOfCurrentRunningTransforms;
int maxNumberOfConcurrentRunningTransforms;

pthread_mutex_t numberOfLocalReductionTasksRanMutex;
int numberOfLocalReductionTasksRan;
double sumOfLocalReductionTasksDurations;
pthread_mutex_t numberOfCurrentRunningLocalReductionsMutex;
int numberOfCurrentRunningLocalReductions;
int maxNumberOfConcurrentRunningLocalReductions;

double globalReductionDuration;

pthread_mutex_t numberOfFailuresMutex;
int numberOfFailures;

void master_initMetrics() {
	pthread_mutex_init(&numberOfTransformTasksRanMutex, NULL);
	numberOfTransformTasksRan = 0;
	sumOfTransformTasksDurations = 0;
	pthread_mutex_init(&numberOfCurrentRunningTransformsMutex, NULL);
	numberOfCurrentRunningTransforms = 0;
	maxNumberOfConcurrentRunningTransforms = 0;

	pthread_mutex_init(&numberOfLocalReductionTasksRanMutex, NULL);
	numberOfLocalReductionTasksRan = 0;
	sumOfLocalReductionTasksDurations = 0;
	pthread_mutex_init(&numberOfCurrentRunningLocalReductionsMutex, NULL);
	numberOfCurrentRunningLocalReductions = 0;
	maxNumberOfConcurrentRunningLocalReductions = 0;

	globalReductionDuration = 0;

	pthread_mutex_init(&numberOfFailuresMutex, NULL);
	numberOfFailures = 0;
}

void master_incrementNumberOfTransformTasksRan(double taskDuration) {
	pthread_mutex_lock(&numberOfTransformTasksRanMutex);
	numberOfTransformTasksRan++;
	sumOfTransformTasksDurations += taskDuration;
	pthread_mutex_unlock(&numberOfTransformTasksRanMutex);
}

int master_getNumberOfTransformTasksRan() {
	return numberOfTransformTasksRan;
}

double master_getTransformAverageDuration() {
	return sumOfTransformTasksDurations / numberOfTransformTasksRan;
}

void master_incrementNumberOfCurrentRunningTransforms() {
	pthread_mutex_lock(&numberOfCurrentRunningTransformsMutex);
	numberOfCurrentRunningTransforms++;
	if (numberOfCurrentRunningTransforms > maxNumberOfConcurrentRunningTransforms) {
		maxNumberOfConcurrentRunningTransforms = numberOfCurrentRunningTransforms;
	}
	pthread_mutex_unlock(&numberOfCurrentRunningTransformsMutex);
}

void master_decrementNumberOfCurrentRunningTransforms() {
	pthread_mutex_lock(&numberOfCurrentRunningTransformsMutex);
	numberOfCurrentRunningTransforms--;
	pthread_mutex_unlock(&numberOfCurrentRunningTransformsMutex);
}

int master_getMaxNumberOfConcurrentRunningTransforms() {
	return maxNumberOfConcurrentRunningTransforms;
}

// Local Reduction

void master_incrementNumberOfLocalReductionTasksRan(double taskDuration) {
	pthread_mutex_lock(&numberOfLocalReductionTasksRanMutex);
	numberOfLocalReductionTasksRan++;
	sumOfLocalReductionTasksDurations += taskDuration;
	pthread_mutex_unlock(&numberOfLocalReductionTasksRanMutex);
}

int master_getNumberOfLocalReductionTasksRan() {
	return numberOfLocalReductionTasksRan;
}

double master_getLocalReductionAverageDuration() {
	return sumOfLocalReductionTasksDurations / numberOfLocalReductionTasksRan;
}

void master_incrementNumberOfCurrentRunningLocalReductions() {
	pthread_mutex_lock(&numberOfCurrentRunningLocalReductionsMutex);
	numberOfCurrentRunningLocalReductions++;
	if (numberOfCurrentRunningLocalReductions > maxNumberOfConcurrentRunningLocalReductions) {
		maxNumberOfConcurrentRunningLocalReductions = numberOfCurrentRunningLocalReductions;
	}
	pthread_mutex_unlock(&numberOfCurrentRunningLocalReductionsMutex);
}

void master_decrementNumberOfCurrentRunningLocalReductions() {
	pthread_mutex_lock(&numberOfCurrentRunningLocalReductionsMutex);
	numberOfCurrentRunningLocalReductions--;
	pthread_mutex_unlock(&numberOfCurrentRunningLocalReductionsMutex);
}

int master_getMaxNumberOfConcurrentRunningLocalReductions() {
	return maxNumberOfConcurrentRunningLocalReductions;
}

// Global Reduction

void master_setGlobalReductionDuration(double duration) {
	globalReductionDuration = duration;
}

double master_getGlobalReductionDuration() {
	return globalReductionDuration;
}

void master_incrementNumberOfFailures() {
	pthread_mutex_lock(&numberOfFailuresMutex);
	numberOfFailures++;
	pthread_mutex_unlock(&numberOfFailuresMutex);
}

int master_getNumberOfFailures() {
	return numberOfFailures;
}
