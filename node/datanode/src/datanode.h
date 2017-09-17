/*
 * datanode.h
 *
 *  Created on: 9/9/2017
 *      Author: utnso
 */

#ifndef DATANODE_H_
#define DATANODE_H_

typedef struct dataNodeConfig {

	char *fsIP;
	char *nodeName;
	char *databinPath;
	int fsPortno;
	int workerPortno;
	int DataNodePortno;
	int sizeInMb;

} t_dataNodeConfig;

typedef struct blockInfo {

	int amountOfBlocks;
	int freeBlocks;
	int occupiedBlocks;

} t_dataNodeBlockInfo;

typedef struct dataNode {

	t_dataNodeBlockInfo blockInfo;
	t_dataNodeConfig config;

} t_dataNode;

int dataNode_loadConfig(t_dataNode *aDataNode);
FILE *dataNode_openOrCreateDataBinFile(char *dataBinPath, int sizeInMb);
void dataNode_connectToFileSystem();
void dataNode_setBlockInformation(t_dataNode *aDataNode);
#endif /* DATANODE_H_ */
