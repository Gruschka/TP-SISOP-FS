/*
 * yama.h
 *
 *  Created on: Sep 8, 2017
 *      Author: utnso
 */

#ifndef YAMA_H_
#define YAMA_H_

typedef enum yama_stage {
	TRANSFORMATION, LOCAL_REDUCTION, GLOBAL_REDUCTION
} yama_job_stage;

typedef enum yama_job_status {
	IN_PROCESS, ERROR, FINISHED_OK
} yama_job_status;

typedef struct yama_state_table_entry {
	uint32_t jobID;
	uint32_t masterID;
	uint32_t nodeID;
	uint32_t blockNumber;
	yama_job_stage stage;
	char *tempPath;
	yama_job_status status;
} yama_state_table_entry;

void loadConfiguration();
void signalHandler(int);

#endif /* YAMA_H_ */
