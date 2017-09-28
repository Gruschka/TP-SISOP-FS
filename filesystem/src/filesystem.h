#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <stdio.h>

typedef struct dataNode {

	char *name;
	int amountOfBlocks;
	int freeBlocks;
	int occupiedBlocks;

} t_dataNode;
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
	char *FSMetadataFileName;
	char *bitmapFileName;
	t_bitarray *bitmap;
	t_directory directoryTable[100];
	int bitmapFileDescriptor;
} t_FS;


//FS commands
int fs_mount(t_FS *FS);
int fs_openOrCreateDirectory(char * directory);
int fs_openOrCreateNodeTableFile(char *directory);
int fs_updateNodeTable(t_dataNode aDataNode);
int fs_getTotalFreeBlocksOfConnectedDatanodes(t_list *connectedDataNodes);
int fs_amountOfElementsInArray(char** array);
int fs_arrayContainsString(char **array, char *string);
int fs_openOrCreateDirectoryTableFile(char *path);
int fs_includeDirectoryOnDirectoryFileTable(char *directory, t_directory *directoryFileTable);
int fs_isDirectoryIncludedInDirectoryTable(char *directory, t_directory *directoryFileTable);
int fs_getFirstFreeIndexOfDirectoryTable(t_directory *directoryTable);
int fs_updateDirectoryTableElement(int indexToUpdate, int parent, char *directory, t_directory *directoryTable);
int fs_wipeDirectoryTableFromIndex(t_directory *directoryTable, int index);

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
