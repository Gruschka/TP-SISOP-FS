#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <commons/bitarray.h>
#include <semaphore.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <stdio.h>
#include <ipc/serialization.h>

#define BLOCK_SIZE 1048576


typedef struct {
 char *firstCopyNodeID;
 int firstCopyBlockID;
 char *secondCopyNodeID;
 int secondCopyBlockID;
 int blockSize;
}t_fileBlockTuple;

typedef enum fileTypes {
	T_BINARY,
	T_TEXT
}t_fileType;

typedef struct binaryStructure {
	char name[10];
	int age;
	int height;
	int weight;
} t_binaryStructure; //size = 22b

typedef struct threadOperation {
	uint32_t operationId;
	uint32_t size;
	uint32_t blockNumber;
	void *buffer;
} t_threadOperation;

typedef struct nodeConnection {
	char *ipAddress;
	int socketfd;
} t_nodeConnection;


typedef struct dataNode {
	int fd;
	char *name;
	char *IP;
	int amountOfBlocks;
	int freeBlocks;
	int occupiedBlocks;
	t_bitarray *bitmap;
	int bitmapFileDescriptor;
	int workerPortno;
	FILE *bitmapFile;
	char *bitmapMapedPointer;
	sem_t *threadSemaphore;
	sem_t *resultSemaphore;
	t_queue *operationsQueue;
	t_queue *resultsQueue;
} t_dataNode;

typedef struct blockPackage {

	int blockNumber;
	int blockCopyNumber;
	int blockSize;
	void *buffer;
	t_dataNode *destinationNode;
	int destinationBlock;

} t_blockPackage;

typedef struct t_directory {
	int index;
	char name[255];
	int parent;
} t_directory;

typedef struct FS {
	char *mountDirectoryPath;
	char *MetadataDirectoryPath;
	char *filesDirectoryPath;
	char *directoryPath;
	char *bitmapFilePath;
	char *nodeTablePath;
	char *directoryTablePath;
	char *tempFilesPath;
	int amountOfDirectories;
	FILE *directoryTableFile;
	char *FSMetadataFileName;
	char *FSFileList;
	int totalAmountOfBlocks;
	int freeBlocks;
	int occupiedBlocks;
	t_directory directoryTable[100];
	int bitmapFileDescriptor;
	int nodeTableFileDescriptor;
	////////////////////////////////////////////////////////////////// WARNING
	int _directoryIndexAutonumber; //no modificar valor a mano! ▁ ▂ ▄ ▅ ▆ ▇ █ ŴÃŘŇĮŇĞ █ ▇ ▆ ▅ ▄ ▂ ▁
	////////////////////////////////////////////////////////////////// WARNING
	int usePreviousStatus;
} t_FS;


//FS commands
int fs_mount(t_FS *FS);
int fs_openOrCreateDirectory(char * directory, int includeFlag);
int fs_openOrCreateNodeTableFile(char *directory);
int fs_updateNodeTable(t_dataNode aDataNode);
int fs_getTotalFreeBlocksOfConnectedDatanodes(t_list *connectedDataNodes);
int fs_amountOfElementsInArray(char** array);
int fs_arrayContainsString(char **array, char *string);
int fs_openOrCreateDirectoryTableFile(char *path);
int fs_includeDirectoryOnDirectoryFileTable(char *directory, t_directory *directoryFileTable);
int fs_isDirectoryIncludedInDirectoryTable(char *directory, t_directory *directoryFileTable);
int fs_getFirstFreeIndexOfDirectoryTable(t_directory *directoryTable);
int fs_updateDirectoryTableArrayElement(int indexToUpdate, int parent, char *directory, t_directory *directoryTable);
int fs_wipeDirectoryTableFromIndex(t_directory *directoryTable, int index);
int fs_openOrCreateBitmap(t_FS FS, t_dataNode *aDataNode);
int fs_writeNBytesOfXToFile(FILE *fileDescriptor, int n, int c);
void fs_dumpDataNodeBitmap(t_dataNode aDataNode);
int fs_checkNodeBlockTupleConsistency(char *dataNodeName, int blockNumber);
t_dataNode *fs_getNodeFromNodeName(char *nodeName);
int fs_getPreviouslyConnectedNodesNames();
float fs_bytesToMegaBytes(int bytes);
t_directory *fs_childOfParentExists(char *child, t_directory *parent);
t_directory *fs_directoryExists(char *directory);
int fs_directoryIsParent(t_directory *directory);
int fs_directoryIsEmpty(char *directory);
int fs_getDirectoryIndex();
int fs_getOffsetFromDirectory(t_directory *directory);
int fs_storeFile(char *fullFilePath, char *fileName, t_fileType fileType, void *buffer, int fileSize);
void *fs_readFile(char *fullFilePath);
t_dataNode *fs_getDataNodeWithMostFreeSpace(t_dataNode *excluding);
int *fs_sendPackagesToCorrespondingNodes(t_list *packageList);
int fs_getFirstFreeBlockFromNode(t_dataNode *dataNode);
void *fs_serializeFile(FILE *file, int fileSize);
int fs_mmapDataNodeBitmap(char * bitmapPath, t_dataNode *aDataNode);
void fs_dumpDataNodeBitmap(t_dataNode aDataNode);
int fs_getAmountOfFreeBlocksOfADataNode(t_dataNode *aDataNode);
int fs_setDataNodeBlock(t_dataNode *aDataNode, int blockNumber);
int fs_cleanBlockFromDataNode(t_dataNode *aDataNode, int blockNumber);
int fs_restorePreviousStatus();
int fs_isNodeFromPreviousSession(t_dataNode aDataNode);
int fs_isDataNodeAlreadyConnected(t_dataNode aDataNode);
int fs_isDataNodeNameInConnectedList(char* aDataNodeName);

int fs_checkNodeConnectionStatus(t_dataNode aDataNode);
char *fs_getParentPath(char *childPath);

ipc_struct_fs_get_file_info_response_entry *fs_getFileBlockTuples(char *filePath);
ipc_struct_fs_get_file_info_response *fs_yamaFileBlockTupleResponse(char *filePath);
int fs_isDataNodeIncludedInPreviouslyConnectedNodes(char *nodeName);
int fs_deleteFileFromIndex(char *path);
int fs_updateFileFromIndex(char *old, char *new);
int fs_deleteBlockFromMetadata(char *path,int block, int copy);
int fs_getNumberOfBlocksOfAFile(char *file);
int fs_getAmountOfBlocksAndCopiesOfAFile(char *file);
void fs_dumpBlockTuple(ipc_struct_fs_get_file_info_response_entry  blockTuple);
void fs_freeTuple(ipc_struct_fs_get_file_info_response_entry *tuple);
void fs_destroyNodeTupleArray(ipc_struct_fs_get_file_info_response_entry *array, int length);
char *fs_isAFile(char *path);
char *fs_isFileContainedBy(char *filePhysicalPath, t_directory *parent);
int *fs_moveFileTo(char *filePhysicalPath, t_directory *parent);
int fs_updateAllConnectedNodesOnTable();
int fs_wipeAllConnectedDataNodes();
int fs_wipeDirectoryTable();
int fs_directoryStartsWithSlash(char *directory);
t_dataNode *fs_pickNodeToSendRead(t_dataNode *first, t_dataNode *copy);
void *fs_readFile(char *filePath);
int fs_createBitmapsOfAllConnectedNodes();
int fs_getFileSize(char *filePath);
int fs_destroyPackageList(t_list **packageList);
int fs_downloadFile(char *yamaFilePath, char *destinationDirectory);
void *fs_downloadBlock(t_dataNode *target, int blockNumber);
int fs_uploadBlock(t_dataNode *target, char *blockNumber, void *buffer);
int fs_getAvailableCopiesFromTuple(ipc_struct_fs_get_file_info_response_entry *tuple);
int fs_createTempFileFromWorker(char *filePath, char *filecontent);
int fs_removeNodeFromConnectedNodeList(t_dataNode aDataNode);
int fs_getBlockSizesOfFileMetadata(char *fileMetadataPath, int amountOfBlocks);
int fs_sumOfIntArray(int *array, int length);
char *fs_removeYamafsFromPath(char *path);
t_dataNode *fs_getDataNodeFromFileDescriptor(int fd);
int fs_clean();
void fs_rebuildNodeTable();


//Console commands
int fs_format();
int fs_rm(char *filePath);
int fs_rm_dir(char *dirPath);
int fs_rm_block(char *filePath, int blockNumberToRemove, int numberOfCopyBlock);
int fs_rename(char *filePath, char *nombreFinal);
int fs_mv(char *origFilePath, char *destFilePath);
int fs_cat(char *filePath);
int fs_mkdir(char *filePath);
int fs_cpfrom(char *origFilePath, char *yama_directory, char *fileType);
int fs_cpto(char *origFilePath, char *yama_directory);
int fs_cpblock(char *origFilePath, int blockNumberToCopy, char* nodeId);
int fs_md5(char *filePath);
int fs_ls(char *filePath);
int fs_info(char *filePath);

//Connection functions
void fs_listenToDataNodesThread();
void fs_waitForDataNodes();
void fs_yamaConnectionThread();
void fs_waitForYama();
void fs_waitForYama_select();
void fs_yama_newConnectionHandler(int fd, char *ip);
void fs_yama_incomingDataHandler(int fd, ipc_struct_header header);
void fs_yama_disconnectionHandler(int fd, char *);
int fs_isStable();
void fs_show_connected_nodes();
void fs_print_connected_node_info(t_dataNode *aDataNode);
void fs_dataNodeConnectionHandler(t_nodeConnection *connection);
void fs_dataNodeThreadHandler(t_dataNode *aNode);
void fs_workerConnectionThread();

#endif /* FILESYSTEM_H_ */
