#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_


typedef struct dataNode {

	char *name;
	int amountOfBlocks;
	int freeBlocks;
	int occupiedBlocks;

} t_dataNode;

int fs_format();
int fs_rm(char *filePath);
int fs_rm_dir(char *dirPath);
int fs_rm_block(char *filePath, int blockNumberToRemove, int numberOfCopyBlock);
int fs_rename (char *filePath, char *nombreFinal);
int fs_mv(char *origFilePath, char *destFilePath);
int fs_cat(char *filePath);
int fs_mkdir(char *filePath);
int fs_cpfrom(char *origFilePath, char *yama_directory);
int fs_cpto(char *origFilePath, char *yama_directory);
int fs_cpblock(char *origFilePath, int blockNumberToCopy, int nodeNumberToCopy);
int fs_md5(char *filePath);
int fs_ls(char *filePath);
int fs_info(char *filePath);
void fs_dataNodeConnectionThread();
void fs_waitForDataNodes();
void fs_yamaConnectionThread();
void fs_waitForYama();
int fs_isStable();


#endif /* FILESYSTEM_H_ */
