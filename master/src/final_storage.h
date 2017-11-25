/*
 * final_storage.h
 *
 *  Created on: Nov 25, 2017
 *      Author: Federico Trimboli
 */

#ifndef FINAL_STORAGE_H_
#define FINAL_STORAGE_H_

#include <ipc/serialization.h>

void master_requestInChargeWorkerFinalStorage(ipc_struct_master_continueWithFinalStorageRequest *yamaRequest, char *resultPath);

#endif /* FINAL_STORAGE_H_ */
