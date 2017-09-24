/**
 TODO: Validar existencia file paths en cada funcion del fs que los utilice
 TODO: Controlar que solo se conecte un YAMA
 TODO: Controlar que se conecte un YAMA solo despues que se conecte un DataNode
 TODO: Validar estado seguros
 TODO: Levantar archivo de configuracion
TODO: Implementar bitmap
TODO: Implementar montaje de FS
TODO: Implementar tabla de directorios
TODO:Implementar tabla de archivos
TODO: Implementar tabla de nodos
TODO: Logear actividad del FS sin mostrar por pantalla


 **/

#include "filesystem.h"
#include <pthread.h>
#include <netinet/in.h>
#include <commons/collections/list.h>
//Return 0 if success 1 if fails

//Global resources
t_list *connectedNodes; //Every time a new node is connected to the FS its included in this list
t_FS myFS; //Global struct containing the information of the FS

void main() {

	connectedNodes = list_create();

	fs_listenToDataNodesThread();

	while (!fs_isStable()) {

		printf("FS not stable. Cant connect to YAMA");

	}

	//fs_yamaConnectionThread();

	fs_console_launch();
}

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
void fs_listenToDataNodesThread() {

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

	//Socket connection variables
	int server_fd, new_dataNode_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[1024] = { 0 };
	char *hello = "You are connected to the FS";

	//Variable to include new DataNode in the list of currently connected data nodes
	t_dataNode newDataNode;

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(-1);
	}
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
			sizeof(opt))) {
		perror("setsockopt");
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

	while (new_dataNode_socket = accept(server_fd, (struct sockaddr*) &address,
			(socklen_t*) &addrlen)) {
		puts("New connection accepted\n");

		pthread_t newDataNodeThread;

		// Copy the value of the accepted socket, in order to pass to the thread
		int new_sock = new_dataNode_socket;

		if (pthread_create(&newDataNodeThread, NULL,
				fs_dataNodeConnectionHandler, (void*) new_dataNode_socket)
				< 0) {
			perror("could not create thread");
		}

		puts("Handler assigned");
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
int fs_isStable() {

	return 1;
}
void fs_show_connected_nodes() {

	int listSize = list_size(connectedNodes);
	int i;
	t_dataNode * aux;
	if (listSize == 0)
		printf("No data nodes connected\n");
	for (i = 0; i < listSize; i++) {
		aux = list_get(connectedNodes, i);
		fs_print_connected_node_info(aux);

	}
}

void fs_print_connected_node_info(t_dataNode *aDataNode) {

	printf(
			"Name:[%s] - Amount of blocks:[%d] - Free blocks:[%d] - Occupied blocks:[%d]\n",
			aDataNode->name, aDataNode->amountOfBlocks, aDataNode->freeBlocks,
			aDataNode->occupiedBlocks);

}

void fs_dataNodeConnectionHandler(void *dataNodeSocket) {

	int valread, cant;
	char buffer[1024] = { 0 };
	char *hello = "You are connected to the FS ss";
	int new_socket = (int *) dataNodeSocket;

	t_dataNode newDataNode;

//Read Node name
	valread = read(new_socket, buffer, 1024);
	printf("New node connected: %s\n", buffer);

	newDataNode.name = malloc(sizeof(buffer));
	memset(newDataNode.name, 0, sizeof(newDataNode));
	strcpy(newDataNode.name, buffer);
	printf("Node name:%s\n", newDataNode.name);

//Send connection confirmation
	send(new_socket, hello, strlen(hello), 0);

//Read amount of blocks
	read(new_socket, &cant, sizeof(int));
	cant = ntohl(cant);
	newDataNode.amountOfBlocks = cant;
	printf("Amount of blocks:%d\n", newDataNode.amountOfBlocks);

//Read amount of free blocks
	read(new_socket, &cant, sizeof(int));
	cant = ntohl(cant);
	newDataNode.freeBlocks = cant;
	printf("Free blocks: %d\n", newDataNode.freeBlocks);

//Read amount of occupied blocks
	read(new_socket, &cant, sizeof(int));
	cant = ntohl(cant);
	newDataNode.occupiedBlocks = cant;
	printf("Occupied blocks: %d\n", newDataNode.occupiedBlocks);

	list_add(connectedNodes, &newDataNode);

	while (1) {

		printf("DataNode %s en FS ala espera de pedidos\n", newDataNode.name);

		sleep(5);






	}
}
