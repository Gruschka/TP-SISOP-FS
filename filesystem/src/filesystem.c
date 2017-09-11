#include "filesystem.h"
#include <pthread.h>
#include <netinet/in.h>

//Return 0 if success 1 if fails

int fs_format() {

	//Do stuff
	printf("format\n");
	return 0;

}

int fs_rm(char *filePath) {
	printf("removing %s\n", filePath);

	return 0;
}
int fs_rm_dir(char *dirPath) {
	printf("removing directory %s \n", dirPath);

	return 0;
}
int fs_rm_block(char *filePath, int blockNumberToRemove, int numberOfCopyBlock) {
	printf("removing block %d whose copy is block %d from file %s\n",
			blockNumberToRemove, numberOfCopyBlock, filePath);

	return 0;
}

int fs_rename(char *filePath, char *nombreFinal) {
	printf("Renaming %s as %s\n", filePath, nombreFinal);

	return 0;
}

int fs_mv(char *origFilePath, char *destFilePath) {
	printf("moving %s to %s\n", origFilePath, destFilePath);
	return 0;

}
int fs_cat(char *filePath) {
	printf("Showing file %s\n", filePath);
	return -1;

}

int fs_mkdir(char *directoryPath) {
	printf("Creating directory c %s\n", directoryPath);
	return 0;

}
int fs_cpfrom(char *origFilePath, char *yama_directory) {
	printf("Copying %s to yama directory %s\n", origFilePath, yama_directory);
	return 0;

}
int fs_cpto(char *origFilePath, char *yama_directory) {
	printf("Copying %s to yama directory %s\n", origFilePath, yama_directory);

	return 0;

}
int fs_cpblock(char *origFilePath, int blockNumberToCopy, int nodeNumberToCopy) {
	printf("Copying block %d from %s to node %d\n", blockNumberToCopy,
			origFilePath, nodeNumberToCopy);
	return 0;

}
int fs_md5(char *filePath) {
	printf("Showing file %s\n", filePath);
	return 0;

}
int fs_ls(char *directoryPath) {
	printf("Showing directory %s\n", directoryPath);
	return 0;

}
int fs_info(char *filePath) {
	printf("Showing info of file file %s\n", filePath);
	return 0;

}

void fs_dataNodeConnectionThread() {

	//Thread ID
	pthread_t threadId;

	//Create thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	//Set to detached
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	//Create thread
	pthread_create(&threadId, &attr, fs_waitForDataNodes, NULL);
}

void fs_waitForDataNodes() {

	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[1024] = { 0 };
	char *hello = "Hello from server";

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(-1);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(8080);

	// Forcefully attaching socket to the port 8080
	if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(-1);
	}
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(-1);
	}
	if ((new_socket = accept(server_fd, (struct sockaddr *) &address,
			(socklen_t*) &addrlen)) < 0) {
		perror("accept");
		exit(-1);
	}

	while (1) {
		valread = read(new_socket, buffer, 1024);
		printf("%s\n", buffer);
		send(new_socket, hello, strlen(hello), 0);
		printf("Hello message sent\n");
		sleep(5);
	}

}

void fs_yamaConnectionThread() {

	//Thread ID
	pthread_t threadId;

	//Create thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	//Set to detached
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	//Create thread
	pthread_create(&threadId, &attr, fs_waitForYama, NULL);
}


void fs_waitForYama() {

	int server_fd, new_socket, valread;
		struct sockaddr_in address;
		int opt = 1;
		int addrlen = sizeof(address);
		char buffer[1024] = { 0 };
		char *hello = "Hello from server";

		// Creating socket file descriptor
		if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
			perror("socket failed");
			exit(-1);
		}

		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(8080);

		// Forcefully attaching socket to the port 8080
		if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
			perror("bind failed");
			exit(-1);
		}
		if (listen(server_fd, 3) < 0) {
			perror("listen");
			exit(-1);
		}
		if ((new_socket = accept(server_fd, (struct sockaddr *) &address,
				(socklen_t*) &addrlen)) < 0) {
			perror("accept");
			exit(-1);
		}

		while (1) {
			valread = read(new_socket, buffer, 1024);
			printf("%s\n", buffer);
			send(new_socket, hello, strlen(hello), 0);
			printf("Hello message sent\n");
			sleep(5);
		}

}
