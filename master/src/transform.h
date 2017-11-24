/*
 * transform.h
 *
 *  Created on: Nov 3, 2017
 *      Author: Federico Trimboli
 */

#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include <ipc/serialization.h>

void master_requestWorkersTransform(ipc_struct_start_transform_reduce_response *yamaResponse, char *transformScript);

#endif /* TRANSFORM_H_ */
