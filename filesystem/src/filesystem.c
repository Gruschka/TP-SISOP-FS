/**
 TODO: Hacer que la tabla de directorios se actualice al levantar el FS para poder validar basura
 TODO: Validar existencia file paths en cada funcion del fs que los utilice
 TODO: Controlar que solo se conecte un YAMA
 TODO: Controlar que se conecte un YAMA solo despues que se conecte un DataNode
 TODO: Validar estado seguros
 TODO: Levantar archivo de configuracion
 TODO: Implementar bitmap
 TODO: Implementar tabla de directorios
 TODO: Implementar tabla de archivos
 TODO: Logear actividad del FS sin mostrar por pantalla
 TODO: Hacer que listaNodos de la funcion updateNodeTable reserve el tamanio justo y no un valor fijo
 TODO: Ejecutar filesystem con flags?
 **/

//Includes
#include "filesystem.h"
#include <pthread.h>
#include <netinet/in.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

//Global resources
t_list *connectedNodes; //Every time a new node is connected to the FS its included in this list
t_FS myFS = { "/mnt/FS/", "/mnt/FS/metadata", "/mnt/FS/metadata/archivos",
		"/mnt/FS/metadata/directorios", "/mnt/FS/metadata/bitmaps",
		"/mnt/FS/metadata/nodos.bin", "/mnt/FS/metadata/directorios.dat" }; //Global struct containing the information of the FS
t_log *logger;
t_config *nodeTableConfig; //Create pointer to t_config containing the nodeTable information
void main() {

	char *logFile = tmpnam(NULL);
	logger = log_create(logFile, "FS", 1, LOG_LEVEL_DEBUG);
	connectedNodes = list_create();
	fs_mount(&myFS);
	fs_includeDirectoryOnDirectoryFileTable("user/juan/datos",
			myFS.directoryTable);

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

	int openNodeTable = fs_openOrCreateNodeTableFile(myFS.nodeTablePath);

	if (openNodeTable == -1)
		log_error(logger, "Error when opening Node Table");

	list_add(connectedNodes, &newDataNode);

	fs_updateNodeTable(newDataNode);

	while (1) {

		printf("DataNode %s en FS ala espera de pedidos\n", newDataNode.name);

		sleep(5);

	}
}
int fs_mount(t_FS *FS) {

	/****************************    MOUNT DIRECTORY ****************************/
	fs_openOrCreateDirectory(myFS.mountDirectoryPath);

	/****************************    METADATA DIRECTORY ****************************/
	fs_openOrCreateDirectory(myFS.MetadataDirectoryPath);

	/****************************    ARCHIVOS DIRECTORY ****************************/
	fs_openOrCreateDirectory(myFS.filesDirectoryPath);

	/****************************    DIRECTORIES DIRECTORY ****************************/
	fs_openOrCreateDirectory(myFS.directoryPath);

	/****************************    DIRECTORY TABLE PATH ****************************/
	fs_openOrCreateDirectoryTableFile(myFS.directoryTablePath);
	/****************************    BITMAP DIRECTORY ****************************/
	fs_openOrCreateDirectory(myFS.bitmapFilePath);

	return 0;

}

int fs_openOrCreateDirectory(char * directory) {

	DIR *newDirectory = opendir(directory);

	//check if mount path exists then open
	if (newDirectory == NULL) {
		// doesnt exist then create
		int error = mkdir(directory, 511);
		// 511 is decimal for mode 777 (octal)
		chmod(directory, 511);
		if (!error) {
			log_debug(logger, "FS path created on directory %s",
					myFS.mountDirectoryPath);
		} else {
			log_error(logger, "error creating path %s", strerror(errno));
			return -1;
		}
	}
	log_debug(logger, "found FS mount path");
	closedir(newDirectory);

}

int fs_openOrCreateNodeTableFile(char *directory) {

	FILE *output;
	char buffer[50];

	//Intenta abrir
	if (output = fopen(directory, "r+")) { //Existe el nodes.bin
		log_debug(logger, "found node table file");
		return output;
	} else { //No puede abrirlo => Lo crea
		log_debug(logger, "node table file not found creating from scratch");
		output = fopen(directory, "w+");

		memset(buffer, 0, 50);
		sprintf(buffer, "TAMANIO=0\n");
		fputs(buffer, output);

		memset(buffer, 0, 50);
		sprintf(buffer, "LIBRE=0\n");
		fputs(buffer, output);

		memset(buffer, 0, 50);
		sprintf(buffer, "NODOS=[]\n");
		fputs(buffer, output);

		fflush(output);
		fclose(output);
		return 0;
	}

	return -1;
}

int fs_updateNodeTable(t_dataNode aDataNode) {

	//Crea archivo config
	nodeTableConfig = config_create(myFS.nodeTablePath);

	/*************** ACTUALIZA TAMANIO ***************/
	char * tamanioOriginal = config_get_string_value(nodeTableConfig,
			"TAMANIO"); //Stores original value of TAMANIO

	int tamanioAcumulado = atoi(tamanioOriginal) + aDataNode.amountOfBlocks; //Suma tamanio original al del nuevo dataNode
	char *tamanioFinal = string_from_format("%d", tamanioAcumulado); // Pasa el valor a string
	config_set_value(nodeTableConfig, "TAMANIO", tamanioFinal); //Toma el valor
//	config_save_in_file(nodeTableConfig, myFS.nodeTablePath); //Lo guarda en archivo

	/*************** ACTUALIZA LIBRE ***************/
	int libresFinal = fs_getTotalFreeBlocksOfConnectedDatanodes(connectedNodes); //Suma todos los bloques libres usando la lista de bloques conectados
	char *libreTotalFinal = string_from_format("%d", libresFinal); //La pasa a string
	config_set_value(nodeTableConfig, "LIBRE", libreTotalFinal);
//	config_save_in_file(nodeTableConfig, myFS.nodeTablePath);

	/*************** ACTUALIZA LISTA NODOS ***************/

	char *listaNodosOriginal = config_get_string_value(nodeTableConfig,
			"NODOS"); //"[Nodo1, Nodo2]"
	char **listaNodosArray = string_get_string_as_array(listaNodosOriginal); //["Nodo1,","Nodo2"]
	char *listaNodosFinal;
	int tamanioArrayNuevo = (fs_amountOfElementsInArray(listaNodosArray) + 1)
			* sizeof(listaNodosArray);
	char *listaNodosAux = malloc(200);
	memset(listaNodosAux, 0, 200);

	int i = 0;

	if (fs_amountOfElementsInArray(listaNodosArray) == 0) { //Si el array de nodos esta vacio, no hace falta fijarse si ya esta adentro.

		listaNodosFinal = string_from_format("[%s]", aDataNode.name);
		config_set_value(nodeTableConfig, "NODOS", listaNodosFinal);
		//config_save_in_file(nodeTableConfig, myFS.nodeTablePath);

	} else { //Si no esta vacio, tengo que fijarme si ya esta en la lista

		if (!fs_arrayContainsString(listaNodosArray, aDataNode.name)) { //Si esta en la lista, no lo agrego

			log_debug(logger, "DataNode is already in DataNode Table\n");

		} else { //Si no esta en la lista lo agrego
			//listaNodosArray = ["NODOA","NODOB"] y quiero meter "NODOC"

			for (i = 0; i < fs_amountOfElementsInArray(listaNodosArray); i++) { //Vuelco a listaNodosAux todos los nodos del array
				strcat(listaNodosAux, listaNodosArray[i]);
				strcat(listaNodosAux, ",");
			}

			strcat(listaNodosAux, aDataNode.name); //concatena el nuevo nodo con coma y espacio

			listaNodosFinal = string_from_format("[%s]", listaNodosAux);
			config_set_value(nodeTableConfig, "NODOS", listaNodosFinal);
			//config_save_in_file(nodeTableConfig, myFS.nodeTablePath);

		}

	}
	config_save_in_file(nodeTableConfig, myFS.nodeTablePath);

	/*************** ACTUALIZA INFORMACION DEL NODO ***************/

	char *nombreAux1 = malloc(sizeof(aDataNode.name)); //le reserva espacio para nombre + total o libre
	strcpy(nombreAux1, aDataNode.name);

	char *nombreAux2 = malloc(sizeof(aDataNode.name)); //le reserva espacio para nombre + total o libre
	strcpy(nombreAux2, aDataNode.name);
	//Crea el string Nodo+Libre=
	char *libres = malloc(strlen(nombreAux1) + strlen("Libre="));
	strcpy(libres, "Libre=");
	libres = strcat(nombreAux1, libres);
	char *bloquesLibresFinal = string_from_format("%d", aDataNode.freeBlocks); //Pasa la cantidad de bloques libres a string

	//Crea el string Nodo+Total=
	char *total = malloc(strlen(nombreAux2) + strlen("Total="));
	strcpy(total, "Total=");
	total = strcat(nombreAux2, total);
	char *bloquesTotalesFinal = string_from_format("%d",
			aDataNode.amountOfBlocks); //Pasa la cantidad de bloques libres a string

	char lineBuffer[255];

	if (!fs_arrayContainsString(listaNodosArray, aDataNode.name)) {	//Si esta en la lista, solo tengo que actualizar su informacion

		config_set_value(nodeTableConfig, libres, bloquesLibresFinal);
		config_set_value(nodeTableConfig, total, bloquesTotalesFinal);
		config_save_in_file(nodeTableConfig, myFS.nodeTablePath);

	} else { //No esta en la lista entonces tengo que crear nueva entrada

		FILE *nodeTableFile = fopen(myFS.nodeTablePath, "a"); //Abre el archivo para appendear al final del mismo. el stream esta posicionado en eof
		//Agrega libres
		memset(lineBuffer, 0, 255);
		sprintf(lineBuffer, "%s%s\n", libres, bloquesLibresFinal);
		fputs(lineBuffer, nodeTableFile);

		//Agrega totales
		memset(lineBuffer, 0, 255);
		sprintf(lineBuffer, "%s%s\n", total, bloquesTotalesFinal);
		fputs(lineBuffer, nodeTableFile);

		fclose(nodeTableFile);

	}

	return 0;

}

int fs_getTotalFreeBlocksOfConnectedDatanodes(t_list *connectedDataNodes) {

	int listSize = list_size(connectedNodes);
	int i;
	int totalFreeBlocks = 0;
	t_dataNode * aux;
	if (listSize == 0) {
		log_error(logger,
				"No data nodes connected. Cant get total amount of free b\n");
		return -1;

	}
	for (i = 0; i < listSize; i++) {
		aux = list_get(connectedNodes, i);
		totalFreeBlocks += aux->freeBlocks;
	}
	return totalFreeBlocks;

}

int fs_amountOfElementsInArray(char** array) {
	int i = 0;
	while (array[i]) {
		i++;
	}
	return i;
}

int fs_arrayContainsString(char **array, char *string) {
	int i = 0;
	while (array[i]) {
		if (!strcmp(array[i], string))			//Si lo encuentra
			return 0;
		i++;
	}
	return -1;
}

int fs_openOrCreateDirectoryTableFile(char *directory) {
	FILE *directoryTableFile;
	char buffer[50];

	//Intenta abrir
	if (directoryTableFile = fopen(directory, "r+")) { //Existe el directorios.dat
		log_debug(logger, "found directories table file");
		/*Si la tabla de directorios ya esta creada cuando levanto el FS
		 * => tengo que limpiar el vector de la directory table desde la ultima posicion*/

		return 0;
	} else { //No puede abrirlo => Lo crea
		log_debug(logger,
				"directory table file not found creating from scratch");
		directoryTableFile = fopen(directory, "w+");
		fclose(directoryTableFile);

		/*Si tengo que crear la tabla significa que es la primera vez que  levanto el FS
		 * => tengo que limpiar el vector de la directory table desde el principio*/
		fs_wipeDirectoryTableFromIndex(myFS.directoryTable, 0);
		return 0;
	}

	log_error(logger, "directory table file not found or created!");

	return -1;
}

int fs_includeDirectoryOnDirectoryFileTable(char *directory,
		t_directory *directoryTable) {

	char ** subDirectories = string_split(directory, "/");
	int amountOfDirectoriesToInclude = fs_amountOfElementsInArray(
			subDirectories);
	int i = 0;
	int index;
	for (i = 0; i < amountOfDirectoriesToInclude; i++) { //Por cada subdirectorio del directorio pasado como parametro

		if (!fs_isDirectoryIncludedInDirectoryTable(subDirectories[i],
				directoryTable)) { //No esta incluido en la tabla
			//Me fijo cual es la primer posicion libre del array
			index = fs_getFirstFreeIndexOfDirectoryTable(directoryTable); //Guardo en index la posicion a actualizar en la tabla
			if (index == -1) {
				log_error(logger,
						"Limit of directory table reached. Aborting directory table update\n");
				return -1;
			}

			fs_updateDirectoryTableElement(index, i, subDirectories[i],
					directoryTable);

		}

	}

}

int fs_isDirectoryIncludedInDirectoryTable(char *directory,
		t_directory *directoryTable) {

	int i = 0;

	while (directoryTable[i].index != 100) {
		if (!strcmp(directory, directoryTable[i].name)){//Lo encontro
			log_debug(logger,"Directory is included in Directory Table\n");
			return 1;

		}
		i++;
	}

	return 0; // no lo encontro
}

int fs_getFirstFreeIndexOfDirectoryTable(t_directory *directoryTable) {

	int i = 0;
	for (i = 0; i < 100; i++) {
		if (directoryTable[i].index == 100) {

			log_debug(logger, "Theres a free space on directory table\n");

			return i;

		}
	}

	log_error(logger, "Limit of directory table reached\n");
	return -1;

}

int fs_updateDirectoryTableElement(int indexToUpdate, int parent,
		char *directory, t_directory *directoryTable) {

	//Si se invoca esta funcion ya se valido que no se haya excedido el limite de la tabla. No hace falta error checking

	directoryTable[indexToUpdate].index = indexToUpdate;
	strcpy(directoryTable[indexToUpdate].name, directory);
	directoryTable[indexToUpdate].parent = parent;

	return 0;

}

int fs_wipeDirectoryTableFromIndex(t_directory *directoryTable, int index) {

	int i = 0;
	for (i = index; i < 100; i++) {
		directoryTable[i].index = 100;
		directoryTable[i].name[0] = '\0';
		directoryTable[i].parent = 100;
	}

	return 0;

}
