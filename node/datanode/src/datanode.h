/*
 * datanode.h
 *
 *  Created on: 9/9/2017
 *      Author: utnso
 */

#ifndef DATANODE_H_
#define DATANODE_H_
#define BLOCK_SIZE 1048576

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
	char *dataBinMMapedPointer;
	int dataBinFileDescriptor;
	FILE *dataBinFile;

} t_dataNode;

int dataNode_loadConfig(t_dataNode *aDataNode);
int dataNode_openOrCreateDataBinFile(char *dataBinPath, int sizeInMb);
void dataNode_connectToFileSystem();
void dataNode_setBlockInformation(t_dataNode *aDataNode);
void *dataNode_getBlock(int blockNumber);
int dataNode_setBlock(int blockNumber, void *data);
#endif /* DATANODE_H_ */
