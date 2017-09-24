#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <commons/bitarray.h>

typedef struct dataNode {

	char *name;
	int amountOfBlocks;
	int freeBlocks;
	int occupiedBlocks;

} t_dataNode;

typedef struct FS {
	char *mountDirectoryPath;
	char *MetadataDirectoryPath;
	char *FSMetadataFileName;
	char *bitmapFileName;
	char *filesDirectoryPath;
	char *dataDirectoryPath;
	t_bitarray *bitmap;
	int bitmapFileDescriptor;
} t_FS;

struct t_directory {
	int index;
	char name[255];
	int parent;
};

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
void fs_listenToDataNodesThread();
void fs_waitForDataNodes();
void fs_yamaConnectionThread();
void fs_waitForYama();
int fs_isStable();
void fs_show_connected_nodes();
void fs_print_connected_node_info(t_dataNode *aDataNode);
void fs_dataNodeConnectionHandler(void *dataNodeSocket);
#endif /* FILESYSTEM_H_ */
