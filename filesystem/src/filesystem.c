#include "filesystem.h"

int fs_rename (char *filePath, char *nombreFinal){
	printf("Renaming %s as %s\n", filePath, nombreFinal);

	return 0;
}

int fs_mv(char *origFilePath, char *destFilePath){
	printf("Renaming %s as %s\n", origFilePath, destFilePath);
	return 0;

}
int fs_cat(char *filePath){
	printf("Showing file %s\n", filePath);
	return -1;

}

int fs_mkdir(char *directoryPath){
	printf("Creating directory c %s\n", directoryPath);
	return 0;

}
int fs_cpfrom(char *origFilePath, char *yama_directory){
	printf("Copying %s to yama directory %s\n", origFilePath, yama_directory);
	return 0;


}
int fs_cpto(char *origFilePath, char *yama_directory){
	printf("Copying %s to yama directory %s\n", origFilePath, yama_directory);

	return 0;

}
int fs_cpblock(char *origFilePath, int blockNumberToCopy, int nodeNumberToCopy){
	printf("Copying block %d from %s to node %d\n", blockNumberToCopy, origFilePath, nodeNumberToCopy);
	return 0;

}
int fs_md5(char *filePath){
	printf("Showing file %s\n", filePath);
	return 0;

}
int fs_ls(char *directoryPath){
	printf("Showing directory %s\n", directoryPath);
	return 0;

}
int fs_info(char *filePath){
	printf("Showing info of file file %s\n", filePath);
	return 0;

}
