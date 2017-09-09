/*
 * filesystem.h
 *
<<<<<<< HEAD
 *  Created on: Sep 8, 2017
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

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
=======
 *  Created on: 9/9/2017
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_
>>>>>>> branch 'master' of https://github.com/sisoputnfrba/tp-2017-2c-Deus-Vult.git



#endif /* FILESYSTEM_H_ */
