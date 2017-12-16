/*
 * metrics.h
 *
 *  Created on: Dec 16, 2017
 *      Author: Federico Trimboli
 */

#ifndef METRICS_H_
#define METRICS_H_

void master_initMetrics();

void master_incrementNumberOfTransformTasksRan(double taskDuration);
int master_getNumberOfTransformTasksRan();
double master_getTransformAverageDuration();

void master_incrementNumberOfLocalReductionTasksRan(double taskDuration);
int master_getNumberOfLocalReductionTasksRan();
double master_getLocalReductionAverageDuration();

void master_setGlobalReductionDuration(double duration);
double master_getGlobalReductionDuration();

void master_incrementNumberOfFailures();
int master_getNumberOfFailures();

#endif /* METRICS_H_ */
