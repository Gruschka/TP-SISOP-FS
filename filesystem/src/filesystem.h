#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <stdio.h>

#define BLOCK_SIZE 1024

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

typedef struct dataNode {

	char *name;
	int amountOfBlocks;
	int freeBlocks;
	int occupiedBlocks;
	t_bitarray *bitmap;

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
	int _directoryIndexAutonumber; //no modificar valor a mano!
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
t_bitarray *fs_openOrCreateBitmap(t_FS FS, t_dataNode aDataNode);
int fs_writeNBytesOfXToFile(FILE *fileDescriptor, int n, int c);
void fs_dumpDataNodeBitmap(t_dataNode aDataNode);
int fs_checkNodeBlockTupleConsistency(char *dataNodeName, int blockNumber);
t_dataNode *fs_getNodeFromNodeName(char *nodeName);
t_list *fs_getPreviouslyConnectedNodesNames();
float fs_bytesToMegaBytes(int bytes);
t_directory *fs_childOfParentExists(char *child, t_directory *parent);
t_directory *fs_directoryExists(char *directory);
int fs_directoryIsParent(t_directory *directory);
int fs_directoryIsEmpty(t_directory *directory);
int fs_getDirectoryIndex();
int fs_getOffsetFromDirectory(t_directory *directory);
int fs_storeFile(char *fullFilePath, char *fileName, t_fileType fileType, void *buffer, int fileSize);
void *fs_readFile(char *fullFilePath);
t_dataNode *fs_getDataNodeWithMostFreeSpace();
int *fs_sendPackagesToCorrespondingNodes(t_list *packageList);
int *fs_getFirstFreeBlockFromNode(t_dataNode *dataNode);

//Console commands
int fs_format();
int fs_rm(char *filePath);
int fs_rm_dir(char *dirPath);
int fs_rm_block(char *filePath, int blockNumberToRemove, int numberOfCopyBlock);
int fs_rename(char *filePath, char *nombreFinal);
int fs_mv(char *origFilePath, char *destFilePath);
int fs_cat(char *filePath);
int fs_mkdir(char *filePath);
int fs_cpfrom(char *origFilePath, char *yama_directory);
int fs_cpto(char *origFilePath, char *yama_directory);
int fs_cpblock(char *origFilePath, int blockNumberToCopy, int nodeNumberToCopy);
int fs_md5(char *filePath);
int fs_ls(char *filePath);
int fs_info(char *filePath);

//Connection functions
void fs_listenToDataNodesThread();
void fs_waitForDataNodes();
void fs_yamaConnectionThread();
void fs_waitForYama();
int fs_isStable();
void fs_show_connected_nodes();
void fs_print_connected_node_info(t_dataNode *aDataNode);
void fs_dataNodeConnectionHandler(void *dataNodeSocket);
#endif /* FILESYSTEM_H_ */
