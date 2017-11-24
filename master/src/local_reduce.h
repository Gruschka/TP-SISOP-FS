/*
 * local_reduce.h
 *
 *  Created on: Nov 24, 2017
 *      Author: Federico Trimboli
 */

#ifndef LOCAL_REDUCE_H_
#define LOCAL_REDUCE_H_

#include <ipc/serialization.h>

void master_requestWorkersLocalReduce(ipc_struct_master_continueWithLocalReductionRequest *yamaRequest, char *localReduceScript);

#endif /* LOCAL_REDUCE_H_ */
