/*
 * global_reduce.h
 *
 *  Created on: Nov 25, 2017
 *      Author: Federico Trimboli
 */

#ifndef GLOBAL_REDUCE_H_
#define GLOBAL_REDUCE_H_

#include <ipc/serialization.h>

void master_requestInChargeWorkerGlobalReduce(ipc_struct_master_continueWithGlobalReductionRequest *yamaRequest, char *globalReduceScript);

#endif /* GLOBAL_REDUCE_H_ */
