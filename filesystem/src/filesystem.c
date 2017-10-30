/*
 * TODO: Restaurar FS de un estado anterior (Si no falla el update Node Table)
 * TODO: El ingresar directorio a la tabla esta mal por el tema del parent
 * TODO: Bitarray no se esta escribiendo bien al archivo (por la cantidad de elementos que devuelve fs_AmountOfElements
 TODO: Cambiar disenio de los hilos de conexion de yama y datanodes
 TODO:Validar existencia file paths en cada funcion del fs que los utilice
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
 */

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
#include <sys/stat.h> //para bitmap
#include <sys/types.h> //para bitmap
#include <fcntl.h> //para bitmap
#include <sys/mman.h> //para bitmap
#include <stdlib.h> //Para EXIT_SUCCESS y EXIT_FAILURE
#include <math.h> //Para redondear los bits

//Global resources
t_list *connectedNodes; //Every time a new node is connected to the FS its included in this list
t_list *previouslyConnectedNodesNames; //Only the names
t_FS myFS = { .mountDirectoryPath = "/mnt/FS/", .MetadataDirectoryPath =
		"/mnt/FS/metadata", .filesDirectoryPath = "/mnt/FS/metadata/archivos",
		.directoryPath = "/mnt/FS/metadata/directorios", .bitmapFilePath =
				"/mnt/FS/metadata/bitmaps/", .nodeTablePath =
				"/mnt/FS/metadata/nodos.bin", .directoryTablePath =
				"/mnt/FS/metadata/directorios.dat", .FSFileList =
				"/mnt/FS/metadata/archivos/archivos.txt", .totalAmountOfBlocks =
				0, .freeBlocks = 0, .occupiedBlocks = 0 }; //Global struct containing the information of the FS

t_log *logger;
t_config *nodeTableConfig; //Para levantar la tabla de nodos como un archivo de config

//Enums
enum flags {
	EMPTY = 100,
	DIRECTORY_TABLE_MAX_AMOUNT = 100,
	DATANODE_ALREADY_CONNECTED = -1
};
void main() {

	char *logFile = tmpnam(NULL);
	logger = log_create(logFile, "FS", 1, LOG_LEVEL_DEBUG);
	connectedNodes = list_create(); //Lista con los DataNodes conectados. Arranca siempre vacia y en caso de corresponder se llena con el estado anterior

	fs_mount(&myFS); //Crea los directorios del FS

	fs_listenToDataNodesThread(); //Este hilo escucha conexiones entrantes. Podriamos hacerlo generico y segun el handshake crear un hilo de DataNode o de YAMA

	while (fs_isStable()) { //Placeholder hardcodeado durlock

		log_debug(logger,
				"FS not stable. Cant connect to YAMA, will try again in 5 seconds-");
		sleep(5);
	}

	//fs_yamaConnectionThread();

	fs_console_launch();
}
int fs_format() {

	fclose(myFS.directoryTableFile);
	remove(myFS.directoryTablePath);

	fs_openOrCreateDirectoryTableFile(myFS.directoryTablePath);
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

	t_directory *directory = fs_directoryExists(dirPath);
	fs_directoryIsEmpty(directory);
	if(directory && !fs_directoryIsParent(directory)){
		//el directorio existe y no tiene childrens
		//todo: validar que no tenga archivos

		int iterator = directory->index;
		fseek(myFS.directoryTableFile,(sizeof(t_directory))*directory->index,SEEK_SET);
		myFS.amountOfDirectories--;
		while(iterator<myFS.amountOfDirectories){
			myFS.directoryTable[iterator] = myFS.directoryTable[iterator+1];
			fwrite(&myFS.directoryTable[iterator],sizeof(t_directory),1,myFS.directoryTableFile);
			iterator++;
		}
		fflush(myFS.directoryTableFile);
		ftruncate(myFS.directoryTableFile->_fileno,sizeof(t_directory)*myFS.amountOfDirectories);
	}else{
		log_debug(logger,"directory doesnt exist or is parent");
	}

	return 0;
}
int fs_rm_block(char *filePath, int blockNumberToRemove, int numberOfCopyBlock) {
	printf("removing block %d whose copy is block %d from file %s\n",
			blockNumberToRemove, numberOfCopyBlock, filePath);

	return 0;
}
int fs_rename(char *filePath, char *nombreFinal) {
	printf("Renaming %s as %s\n", filePath, nombreFinal);
	t_directory *toRename;
	//todo: chequear si es un archivo
	if(toRename = fs_directoryExists(filePath)){
		memset(toRename->name,0,255);
		strcpy(toRename->name,nombreFinal);
		fseek(myFS.directoryTableFile,sizeof(t_directory)*toRename->index,SEEK_SET);
		fwrite(toRename,sizeof(t_directory),1,myFS.directoryTableFile);
		fflush(myFS.directoryTableFile);
		fseek(myFS.directoryTableFile,sizeof(t_directory)*myFS.amountOfDirectories,SEEK_SET);
	}else{
		log_error(logger,"dir to rename doesnt exist");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
int fs_mv(char *origFilePath, char *destFilePath) {
	printf("moving %s to %s\n", origFilePath, destFilePath);
	t_directory *originDirectory;
	t_directory *destinationDirectory;

	//chequeo si origen existe
	//todo: chequear si es un archivo
	if(!(originDirectory = fs_directoryExists(origFilePath))){
		log_error(logger,"aborting mv: origin directory doesnt exist");
		return EXIT_FAILURE;
	}

	//chequeo si destino existe
	if(!(destinationDirectory = fs_directoryExists(destFilePath))){
		log_error(logger,"aborting mv: destination directory doesnt exist");
		return EXIT_FAILURE;
	}


	//chequeo si original ya existe en destino
	char *originName = strrchr(origFilePath,'/');
	char *destinationFullName = strdup(destFilePath);
	string_append(&destinationFullName,originName);

	if(!fs_directoryExists(destinationFullName)){
		log_error(logger,"aborting mv: origin directory already exists in destination");
		return EXIT_FAILURE;
	}

	originDirectory->parent = destinationDirectory->index;
	fseek(myFS.directoryTableFile,fs_getOffsetFromDirectory(originDirectory),SEEK_SET);
	fwrite(originDirectory,sizeof(t_directory),1,myFS.directoryTableFile);
	fflush(myFS.directoryTableFile);


	return EXIT_SUCCESS;

}
int fs_cat(char *filePath) {
	printf("Showing file %s\n", filePath);
	return -1;

}
int fs_mkdir(char *directoryPath) {
	printf("Creating directory c %s\n", directoryPath);
	t_directory *root = malloc(sizeof(t_directory));
	root->index = 0;
	strcpy(root->name,"root");
	root->parent = -1;

	char **splitDirectory = string_split(directoryPath,"/");
	int amountOfDirectories = fs_amountOfElementsInArray(splitDirectory);

	int iterator = 0;
	t_directory *directoryReference = &myFS.directoryTable[0];
	t_directory *lastDirectoryReference;
	while(iterator < amountOfDirectories){
		lastDirectoryReference = directoryReference;
		directoryReference = fs_childOfParentExists(splitDirectory[iterator],directoryReference);
		iterator++;
		if(directoryReference == NULL){
			//no existe child para ese parent
			//ver si es el ultimo dir
			if(iterator == amountOfDirectories){
				// no existe para ese parent, hay que crearlo
				myFS.directoryTable[myFS.amountOfDirectories].index = fs_getDirectoryIndex();
				strcpy(myFS.directoryTable[myFS.amountOfDirectories].name,splitDirectory[iterator-1]);
				myFS.directoryTable[myFS.amountOfDirectories].parent = lastDirectoryReference->index;
				fwrite(&myFS.directoryTable[myFS.amountOfDirectories],sizeof(t_directory),1,myFS.directoryTableFile);
				fflush(myFS.directoryTableFile);
				myFS.amountOfDirectories++;
			}else{
				//no existe y no es el ultimo, es decir no existe el parent
				// se aborta la creacion
				log_error(logger,"parent directory for directory to create doesnt exist");
				return EXIT_FAILURE;
			}
		}else{
			if(iterator == amountOfDirectories){
				// dir exists, abort
				log_error(logger,"directory already exists");
				return EXIT_FAILURE;
			}
		}
	}


	return EXIT_SUCCESS;

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
	int iterator = 0;
	if(!strcmp(directoryPath,"-fs")){
		printf("i    n    p\n");
		while(iterator < myFS.amountOfDirectories){
			printf("%d    ",myFS.directoryTable[iterator].index);
			printf("%s    ",myFS.directoryTable[iterator].name);
			printf("%d    \n",myFS.directoryTable[iterator].parent);
			iterator++;
		}
	}
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
	FILE *fileListFileDescriptor = fopen(myFS.FSFileList, "r+");
	if(fileListFileDescriptor == NULL){
		fileListFileDescriptor = fopen(myFS.FSFileList, "w+");
		fclose(fileListFileDescriptor);
		return EXIT_SUCCESS;
	}
	char buffer[255];
	memset(buffer, 0, 255);
	int size;
	int blockCount;
	int iterator;
	int copy;
	t_config *fileMetadata;
	while (fgets(buffer, 255, fileListFileDescriptor)) {
		if (buffer[strlen(buffer) - 1] == '\n') {
			buffer[strlen(buffer) - 1] = '\0';
		}

		char *fullFilePath = string_from_format("/mnt/FS/%s", buffer);
		fileMetadata = config_create(fullFilePath);
		char *sizeString = config_get_string_value(fileMetadata, "TAMANIO");
		size = atoi(sizeString);

		blockCount = ceilf(fs_bytesToMegaBytes(size)); //Convierto a MB y como cada bloque es de 1MB la relacion es 1 a 1 (no hace falta dividir por block/size)
		//Uso ceilf por si usa una parte de otro bloque (ej 1,17 son 2 bloques).
		iterator = 0;
		copy = 0;

		while (iterator < blockCount) { //Si blockCount es 1 => son 2 bloques (0,1) por lo que debe ser menor o igual
			char *blockToSearch = string_from_format("BLOQUE%dCOPIA%d",
					iterator, copy);
			char *nodeBlockTupleAsString = config_get_string_value(fileMetadata,
					blockToSearch);
			char **nodeBlockTupleAsArray = string_get_string_as_array(
					nodeBlockTupleAsString);

			int blockNumber = atoi(nodeBlockTupleAsArray[1]);

			//si tenes el nodo y el nodo tiene el bloque esta OK, me chupa un huevo la data
			if (!fs_checkNodeBlockTupleConsistency(nodeBlockTupleAsArray[0],
					blockNumber)) {
				free(blockToSearch);
				iterator++;
				copy = 0;
			} else {
				if (!copy) {
					copy = 1;
				} else {
					//no pudo armar el bloque
					log_debug(logger, "consistency error for file %s", buffer);
					log_debug(logger, "missing block no. %s", blockToSearch);
					config_destroy(fileMetadata);
					free(fullFilePath);
					return EXIT_FAILURE;
				}
			}
		}
		config_destroy(fileMetadata);
		free(fullFilePath);
	}

	fclose(fileListFileDescriptor);

	return EXIT_SUCCESS;
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
	char *hello = "You are connected to the FS";
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

	int nodeTableUpdate = fs_updateNodeTable(newDataNode);

	/*if (nodeTableUpdate == DATANODE_ALREADY_CONNECTED) {
	 log_error(logger,
	 "Node table update failed.\n");

	 //Matar hilo

	 return;
	 }*/

	//newDataNode.bitmap = fs_openOrCreateBitmap(myFS, newDataNode);
//	fs_dumpDataNodeBitmap(newDataNode);
	list_add(connectedNodes, &newDataNode);

	while (1) {

		printf("DataNode %s en FS ala espera de pedidos\n", newDataNode.name);

		sleep(5);

	}
}
int fs_mount(t_FS *FS) {

	/****************************    MOUNT DIRECTORY ****************************/
	fs_openOrCreateDirectory(myFS.mountDirectoryPath, 0);

	/****************************    METADATA DIRECTORY ****************************/
	fs_openOrCreateDirectory(myFS.MetadataDirectoryPath, 0);

	/****************************    FILES DIRECTORY ****************************/
	fs_openOrCreateDirectory(myFS.filesDirectoryPath, 0);

	/****************************    BITMAP DIRECTORY ****************************/
	fs_openOrCreateDirectory(myFS.bitmapFilePath, 0);

	/****************************    DIRECTORY TABLE PATH ****************************/
	fs_openOrCreateDirectoryTableFile(myFS.directoryTablePath);

	int openNodeTable = fs_openOrCreateNodeTableFile(myFS.nodeTablePath);
	if (openNodeTable == -1)
		log_error(logger, "Error when opening Node Table");

	return 0;

}

int fs_openOrCreateDirectory(char * directory, int includeInTable) {

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
	log_debug(logger, "found path");
	closedir(newDirectory);

	if (includeInTable)
		fs_includeDirectoryOnDirectoryFileTable(directory,
				myFS.directoryTablePath);

	return 0;

}

int fs_openOrCreateNodeTableFile(char *directory) {

	FILE *output;
	char buffer[50];

	//Intenta abrir
	if (output = fopen(directory, "r+")) { //Existe el nodes.bin
		log_debug(logger, "found node table file");
		myFS.nodeTableFileDescriptor = output->_fileno;
		return 0;
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

	//Crea archivo config para levantar la node table
	nodeTableConfig = config_create(myFS.nodeTablePath);

	char *listaNodosOriginal = config_get_string_value(nodeTableConfig,
			"NODOS"); //"[Nodo1, Nodo2]"
	char **listaNodosArray = string_get_string_as_array(listaNodosOriginal); //["Nodo1,","Nodo2"]

	if (fs_arrayContainsString(listaNodosArray, aDataNode.name) == 0
			&& fs_amountOfElementsInArray(listaNodosArray) > 0) { //Si esta en la lista, no lo agrego

		log_debug(logger, "DataNode is already in DataNode Table\n");

		config_destroy(nodeTableConfig);
		return DATANODE_ALREADY_CONNECTED;

	}
	/*************** ACTUALIZA TAMANIO ***************/
	char * tamanioOriginal = config_get_string_value(nodeTableConfig,
			"TAMANIO"); //Stores original value of TAMANIO

	int tamanioAcumulado = atoi(tamanioOriginal) + aDataNode.amountOfBlocks; //Suma tamanio original al del nuevo dataNode
	char *tamanioFinal = string_from_format("%d", tamanioAcumulado); // Pasa el valor a string
	config_set_value(nodeTableConfig, "TAMANIO", tamanioFinal); //Toma el valor
	myFS.totalAmountOfBlocks = tamanioAcumulado;

	/*************** ACTUALIZA LIBRE ***************/
	int libresFinal = fs_getTotalFreeBlocksOfConnectedDatanodes(connectedNodes); //Suma todos los bloques libres usando la lista de bloques conectados

	if (libresFinal == -1) { //Si la lista esta vacia osea que es el primer data node en conectarse
		char *libreTotalFinal = string_from_format("%d", aDataNode.freeBlocks); //La pasa a string
		config_set_value(nodeTableConfig, "LIBRE", libreTotalFinal);
		myFS.freeBlocks = aDataNode.freeBlocks;
	} else {
		int bloquesLibresTotales = aDataNode.freeBlocks + libresFinal;
		char *libreTotalFinal = string_from_format("%d", bloquesLibresTotales); //La pasa a string
		config_set_value(nodeTableConfig, "LIBRE", libreTotalFinal);
		myFS.freeBlocks = libresFinal;
	}

	myFS.occupiedBlocks = myFS.totalAmountOfBlocks - myFS.freeBlocks;

	/*************** ACTUALIZA LISTA NODOS EN NODE TABLE ***************/

	char *listaNodosFinal;
	int tamanioArrayNuevo = (fs_amountOfElementsInArray(listaNodosArray) + 1)
			* sizeof(listaNodosArray);
	char *listaNodosAux = malloc(200);
	memset(listaNodosAux, 0, 200);

	int i = 0;

	if (fs_amountOfElementsInArray(listaNodosArray) == 0) { //Si el array de nodos esta vacio, no hace falta fijarse si ya esta adentro del archivo node table

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

	/*************** ACTUALIZA INFORMACION DE BLOQUES DEL NODO EN NODE TABLE ***************/

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

	/*
	 free(nombreAux1);
	 free(nombreAux2);
	 free(libres);
	 free(total);
	 */
	config_destroy(nodeTableConfig);
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
		if (!strcmp(array[i], string))	//Si lo encuentra
			return 0;
		i++;
	}
	return -1;
}

int fs_openOrCreateDirectoryTableFile(char *directory) {
	myFS.directoryTableFile = fopen(myFS.directoryTablePath,"rb+");
	int iterator = 0;
	t_directory *directoryRecord;
	int bytesRead;

	if(myFS.directoryTableFile ==NULL){
		//hay que crearlo
		myFS.directoryTableFile = fopen(myFS.directoryTablePath,"wb+");

		//cargo root
		myFS.directoryTable[0].index = 0;
		strcpy(myFS.directoryTable[0].name,"root");
		myFS.directoryTable[0].parent = -1;

		//escribo root
		int test;
		test = fwrite(&myFS.directoryTable[0],sizeof(t_directory),1,myFS.directoryTableFile);
		fflush(myFS.directoryTableFile);

		myFS.amountOfDirectories++;
		myFS._directoryIndexAutonumber = myFS.directoryTable[myFS.amountOfDirectories-1].index + 1;
		return EXIT_SUCCESS;
	}else{
		//hay que cargarlo
		myFS.amountOfDirectories = 0;
		while(bytesRead!=0){
			bytesRead = fread(&myFS.directoryTable[iterator],sizeof(t_directory),1,myFS.directoryTableFile);
			if(bytesRead){
				myFS.amountOfDirectories++;
			}
			iterator++;
		}
		myFS._directoryIndexAutonumber = myFS.directoryTable[myFS.amountOfDirectories-1].index + 1;
		return EXIT_SUCCESS;

	}

	return EXIT_FAILURE;
}

int fs_includeDirectoryOnDirectoryFileTable(char *directory,
		t_directory *directoryTable) {

	char ** subDirectories = string_split(directory, "/");
	int amountOfDirectoriesToInclude = fs_amountOfElementsInArray(
			subDirectories);
	int i = 0;
	int index;
	int firstFreeIndex = fs_getFirstFreeIndexOfDirectoryTable(directoryTable);
	int availableDirectories = 100 - firstFreeIndex; //100 es el maximo de directorios. First free index es el primer indice donde puede escribirse un nuevo directorio
	if (amountOfDirectoriesToInclude > availableDirectories) {
		log_error(logger,
				"Limit of directory table reached. Aborting directory table update\n");
		return -1;
	}

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

			fs_updateDirectoryTableArrayElement(index, i, subDirectories[i],
					directoryTable);

		}

	}

	i = 0;
	FILE *directoryTableFile = fopen(myFS.directoryTablePath, "w+");
	char *buffer = malloc(sizeof(t_directory));
	memset(buffer, 0, sizeof(buffer));

	while (directoryTable[i].index != 100) {
		sprintf(buffer, "%d %s %d\n", directoryTable[i].index,
				directoryTable[i].name, directoryTable[i].parent);
		fputs(buffer, directoryTableFile);
		memset(buffer, 0, sizeof(buffer));
		i++;
	}

	fclose(directoryTableFile);
	return 0;

}

int fs_isDirectoryIncludedInDirectoryTable(char *directory,
		t_directory *directoryTable) {

	int i = 0;

	while (directoryTable[i].index != EMPTY) {
		if (!strcmp(directory, directoryTable[i].name)) { //Lo encontro
			log_debug(logger, "Directory is included in Directory Table\n");
			return 1;

		}
		i++;
	}

	return 0; // no lo encontro
}

int fs_getFirstFreeIndexOfDirectoryTable(t_directory *directoryTable) {

	int i = 0;
	for (i = 0; i < DIRECTORY_TABLE_MAX_AMOUNT; i++) {
		if (directoryTable[i].index == EMPTY) {

			log_debug(logger, "Theres a free space on directory table\n");

			return i;

		}
	}

	log_error(logger, "Limit of directory table reached\n");
	return -1;

}

int fs_updateDirectoryTableArrayElement(int indexToUpdate, int parent,
		char *directory, t_directory *directoryTable) {

	//Si se invoca esta funcion ya se valido que no se haya excedido el limite de la tabla. No hace falta error checking

	directoryTable[indexToUpdate].index = indexToUpdate;
	strcpy(directoryTable[indexToUpdate].name, directory);
	directoryTable[indexToUpdate].parent = parent;

	return 0;

}

int fs_wipeDirectoryTableFromIndex(t_directory *directoryTable, int index) {

	int i = 0;
	for (i = index; i < DIRECTORY_TABLE_MAX_AMOUNT; i++) {
		directoryTable[i].index = EMPTY;
		directoryTable[i].name[0] = '\0';
		directoryTable[i].parent = EMPTY;
	}

	return 0;

}

int fs_writeNBytesOfXToFile(FILE *fileDescriptor, int N, int C) { //El tamanio del archivo antes del mmap matchea con el tamanio del bitmap
	char *buffer = malloc(N);

	memset(buffer, C, N);
	fwrite(buffer, N, 1, fileDescriptor);
	return 0;
}
t_bitarray *fs_openOrCreateBitmap(t_FS FS, t_dataNode aDataNode) {

	char * fullBitmapPath = malloc(
			strlen(myFS.bitmapFilePath) + strlen(aDataNode.name)
					+ strlen(".dat"));
	memset(fullBitmapPath, 0, sizeof(fullBitmapPath));
	char * fullBitmapPathBuffer = string_from_format("%s%s.dat",
			myFS.bitmapFilePath, aDataNode.name);
	strcpy(fullBitmapPath, fullBitmapPathBuffer);

	FILE * bitmapPath;

	t_bitarray *output;
	int bitmapFileDescriptor = 0;
	if (bitmapPath = fopen(fullBitmapPath, "rb+")) { //Abre bitmap

		fclose(bitmapPath);

		bitmapFileDescriptor = open(fullBitmapPath, O_RDWR);

		struct stat fileStats;
		fstat(bitmapFileDescriptor, &fileStats);

		//Mappea el bitmap a memoria para que se actualice automaticamente al modificar la memoria
		char *mapPointer = mmap(0, fileStats.st_size, PROT_WRITE, MAP_SHARED,
				bitmapFileDescriptor, 0);

		//Actualiza el bitarray pasando el puntero que devuelve el map
		output = bitarray_create_with_mode(mapPointer,
				aDataNode.amountOfBlocks / 8, LSB_FIRST);

		log_debug(logger, "opened bitmap from disk");
		free(fullBitmapPath);
		return output;

	} else { //Si no pudo abrir bit map (no lo encuentra) lo crea
		log_debug(logger, "bitmap file not found creating with parameters");
		bitmapFileDescriptor = fopen(fullBitmapPath, "wb+");
		chmod(fullBitmapPath, 511);

		int error = fs_writeNBytesOfXToFile(bitmapFileDescriptor,
				aDataNode.amountOfBlocks, 0);

		if (error == -1) {
			fclose(bitmapFileDescriptor);
			log_error(logger, "Error calling lseek() to 'stretch' the file: %s",
					strerror(errno));
			return -1;
		}
		fclose(bitmapFileDescriptor);

		int mmapFileDescriptor = open(fullBitmapPath, O_RDWR);
		bitmapFileDescriptor = mmapFileDescriptor;
		struct stat scriptMap;
		fstat(mmapFileDescriptor, &scriptMap);

		char* mapPointer = mmap(0, scriptMap.st_size, PROT_WRITE, MAP_SHARED,
				mmapFileDescriptor, 0);

		output = bitarray_create_with_mode(mapPointer,
				aDataNode.amountOfBlocks / 8, LSB_FIRST);

		log_debug(logger, "bitmap created with parameters");
		return output;
	}

	return NULL;

}

void fs_dumpDataNodeBitmap(t_dataNode aDataNode) {

	int i = 0;
	int unBit = 0;

	while (i < myFS.totalAmountOfBlocks) {
		unBit = bitarray_test_bit(aDataNode.bitmap, i);
		printf("%d.[%d]", i, unBit);
		i++;
	}
	printf("\n");

	fflush(stdout);

}

int fs_checkNodeBlockTupleConsistency(char *dataNodeName, int blockNumber) { //No puede abrirlo => Lo crea
	t_dataNode *connectedNode;
	connectedNode = fs_getNodeFromNodeName(dataNodeName);
	if (connectedNode == NULL) {
		return EXIT_FAILURE;
	}

	if (connectedNode->amountOfBlocks >= blockNumber) {
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}

t_dataNode *fs_getNodeFromNodeName(char *nodeName) {
	int listSize = list_size(connectedNodes);
	if (!listSize) {
		return NULL;
	}

	int iterator = 0;
	t_dataNode *nodeToReturn;

	while (iterator < listSize) {
		nodeToReturn = list_get(connectedNodes, iterator);
		if (!strcmp(nodeName, nodeToReturn->name)) {
			return nodeToReturn;
		}
		iterator++;
	}

	return NULL;
}

t_list *fs_getPreviouslyConnectedNodesNames() {

	nodeTableConfig = config_create(myFS.nodeTablePath);

	char *listaNodosOriginal = config_get_string_value(nodeTableConfig,
			"NODOS"); //"[Nodo1, Nodo2]"
	char **listaNodosArray = string_get_string_as_array(listaNodosOriginal); //["Nodo1","Nodo2"]

	if (fs_amountOfElementsInArray(listaNodosArray) == 0) {
		log_debug(logger,
				"No previously connected nodes - FS is starting from scratch");
		return EXIT_FAILURE;
	}

	int i = 0;

	for (i = 0; i < fs_amountOfElementsInArray(listaNodosArray); i++) {
		list_add(previouslyConnectedNodesNames, listaNodosArray[i]);
	}

	char * primerNodo = list_get(previouslyConnectedNodesNames, 0);
	char * segundoNodo = list_get(previouslyConnectedNodesNames, 1);

	config_destroy(nodeTableConfig);
	return EXIT_SUCCESS;

}

float fs_bytesToMegaBytes(int bytes) {
	float bytesInFloat = bytes;
	return bytesInFloat / 1024 / 1024;
}

t_directory *fs_childOfParentExists(char *child, t_directory *parent){
	int iterator = 0;
	while(iterator < 100){
		if(!strcmp(myFS.directoryTable[iterator].name,child)){
			if(myFS.directoryTable[iterator].parent == parent->index){
				return &myFS.directoryTable[iterator];
			}
		}
		iterator++;
	}
	return NULL;
}

t_directory *fs_directoryExists(char *directory){

	t_directory *root = malloc(sizeof(t_directory));
	root->index = 0;
	strcpy(root->name,"root");
	root->parent = -1;

	char **splitDirectory = string_split(directory,"/");
	int amountOfDirectories = fs_amountOfElementsInArray(splitDirectory);

	int iterator = 0;
	t_directory *directoryReference = &myFS.directoryTable[0];
	t_directory *lastDirectoryReference;
	while(iterator < amountOfDirectories){
		lastDirectoryReference = directoryReference;
		directoryReference = fs_childOfParentExists(splitDirectory[iterator],directoryReference);
		iterator++;
		if(directoryReference == NULL){
			//no existe child para ese parent
			//ver si es el ultimo dir
			if(iterator == amountOfDirectories){
			}else{
				//no existe y no es el ultimo, es decir no existe el parent
				// se aborta la creacion
				log_error(logger,"parent directory for directory to create doesnt exist puto");
				return NULL;
			}
		}else{
			if(iterator == amountOfDirectories){
				// dir exists, abort
				//log_error(logger,"directory already exists puto");
				return directoryReference;
			}
		}
	}


	return EXIT_FAILURE;
}

int fs_directoryIsParent(t_directory *directory){
	int iterator = 0;

	while(iterator < 100){
		if(myFS.directoryTable[iterator].parent == directory->index){
			return 1;
		}
		iterator++;
	}
	return 0;
}

int fs_directoryIsEmpty(t_directory *directory){
	int n = 0;
	struct dirent *d;
	char stringIndex[10];
	sprintf(stringIndex,"%d",directory->index);

	int directoryNameLenght = strlen(myFS.filesDirectoryPath) + strlen(stringIndex) + 2;
	char *directoryName = malloc(directoryNameLenght);
	sprintf(directoryName,"%s/%s\0",myFS.filesDirectoryPath,stringIndex);


	DIR *dir = opendir(directoryName);
	if (dir == NULL) //Not a directory or doesn't exist
		return 1;
	while ((d = readdir(dir)) != NULL) {
		if(++n > 2)
		  break;
	}
	closedir(dir);
	if (n <= 2) //Directory Empty
		return 1;
	else
		return 0;
}

int fs_getDirectoryIndex(){
	int returnNumber = myFS._directoryIndexAutonumber;
	myFS._directoryIndexAutonumber++;
	return returnNumber;
}

int fs_getOffsetFromDirectory(t_directory *directory){
	int iterator = 0;

	while(iterator<myFS.amountOfDirectories){
		if(myFS.directoryTable[iterator].index == directory->index){
			return sizeof(t_directory)*iterator;
		}
		iterator++;
	}

	return EXIT_FAILURE;
}
