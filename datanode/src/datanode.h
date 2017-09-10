/*
 * datanode.h
 *
 *  Created on: 9/9/2017
 *      Author: utnso
 */

#ifndef DATANODE_H_
#define DATANODE_H_


typedef struct dataNodeConfig{

	char *fsIP;
	char *nodeName;
	char *databinPath;
	int fsPortno;
	int workerPortno;
	int DataNodePortno;

}t_dataNodeConfig;

typedef struct dataNode{

	t_dataNodeConfig config;

}t_dataNode;



int dataNode_loadConfig(t_dataNode *aDataNode);


#endif /* DATANODE_H_ */
