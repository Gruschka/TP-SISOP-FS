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

/********* INCLUDES **********/
#include "filesystem.h"
#include <pthread.h>
#include <semaphore.h>
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
#include <ipc/ipc.h>
#include <ipc/serialization.h>

/********* GLOBAL RESOURCES AND HANDSHAKE**********/
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

/********* ENUMS **********/

enum flags {
	EMPTY = 100,
	DIRECTORY_TABLE_MAX_AMOUNT = 100,
	DATANODE_ALREADY_CONNECTED = 1,
	DATANODE_IS_FROM_PREVIOUS_SESSION =1,
	BLOCK_DOES_NOT_EXIST = -1,
	TRUE = 1,
	FALSE = 0
};



/********* MAIN **********/
void main() {
	serialization_initialize();
	char *logFile = tmpnam(NULL);
	logger = log_create(logFile, "FS", 1, LOG_LEVEL_DEBUG);
	connectedNodes = list_create(); //Lista con los DataNodes conectados. Arranca siempre vacia y en caso de corresponder se llena con el estado anterior
	previouslyConnectedNodesNames = list_create();
	fs_mount(&myFS); //Crea los directorios del FS

	//fs_restorePreviousStatus();

	//t_fileBlockTuple *test = fs_getFileBlockTuple("/mnt/FS/metadata/archivos/1/ejemplo.txt");


	fs_listenToDataNodesThread(); //Este hilo escucha conexiones entrantes. Podriamos hacerlo generico y segun el handshake crear un hilo de DataNode o de YAMA

	while (fs_isStable()) { //Placeholder hardcodeado durlock

		log_debug(logger,
				"FS not stable. Cant connect to YAMA, will try again in 5 seconds-");
		sleep(5);
	}

	fs_yamaConnectionThread();

	fs_console_launch();
}

/************************************* CONSOLE COMMANDS *************************************/
int fs_format() {

	fclose(myFS.directoryTableFile);
	char *command = string_from_format("rm -r %s",myFS.mountDirectoryPath);
	int result = system(command);
	free(command);
	myFS._directoryIndexAutonumber = 0; // ▁ ▂ ▄ ▅ ▆ ▇ █ ŴÃŘŇĮŇĞ █ ▇ ▆ ▅ ▄ ▂ ▁
	myFS.usePreviousStatus = FALSE;
	fs_mount(&myFS);
	fs_wipeDirectoryTable();
    fs_wipeAllConnectedDataNodes();
    fs_createBitmapsOfAllConnectedNodes();
	fs_updateAllConnectedNodesOnTable();

	log_debug(logger,"Format successful");
	return EXIT_SUCCESS;

}
int fs_rm(char *filePath) {
	printf("removing %s\n", filePath);

	// get parent path
	char **splitPath = string_split(filePath,"/");
	int splitPathElementCount = fs_amountOfElementsInArray(splitPath);
	int fileNameLength = strlen(splitPath[splitPathElementCount-1]);

	char *parentPath = strdup(filePath);
	parentPath[strlen(parentPath)-fileNameLength] = 0;

	//check parent path exists and get parent dir
	t_directory *parent = fs_directoryExists(parentPath);
	if(!parent){
		log_error(logger,"fs_rm: directory doesnt exist");
		return EXIT_FAILURE;
	}

	//check file exists
	//transform path to physical path
	char *physicalPath = string_from_format("%s/%d/%s",myFS.filesDirectoryPath,parent->index,splitPath[splitPathElementCount-1]);
	FILE *fileMetadata = fopen(physicalPath,"r+");
	if(!fileMetadata){
		log_error(logger,"fs_rm: file doesnt exist");
		return EXIT_FAILURE;
	}

	// release occupied blocks
	int amountOfBlocks = fs_getNumberOfBlocksOfAFile(physicalPath);
	int blockIterator = 0;
	ipc_struct_fs_get_file_info_response_entry *blockArray = fs_getFileBlockTuples(physicalPath);
	t_dataNode dataNode;
	t_dataNode dataNodeCopy;
	t_dataNode *targetDataNode;
	while(blockIterator < amountOfBlocks){
		dataNode.name = string_from_format("%s",blockArray[blockIterator].firstCopyNodeID);
		dataNodeCopy.name = string_from_format("%s",blockArray[blockIterator].secondCopyNodeID);
		if(fs_isDataNodeAlreadyConnected(dataNode)){
			targetDataNode = fs_getNodeFromNodeName(dataNode.name);
			fs_cleanBlockFromDataNode(targetDataNode,blockArray[blockIterator].firstCopyBlockID);
			free(dataNode.name);
		}
		if(fs_isDataNodeAlreadyConnected(dataNodeCopy)){
			targetDataNode = fs_getNodeFromNodeName(dataNodeCopy.name);
			fs_cleanBlockFromDataNode(targetDataNode,blockArray[blockIterator].secondCopyBlockID);
			free(dataNodeCopy.name);
		}
		blockIterator+=1;
	}

	// update file index
	fs_deleteFileFromIndex(physicalPath);

	// delete metadata file
	fclose(fileMetadata);
	remove(physicalPath);

	free(splitPath);
	free(parentPath);
	free(physicalPath);

	return EXIT_SUCCESS;
}
int fs_rm_dir(char *dirPath) {
	printf("removing directory %s \n", dirPath);

	t_directory *directory = fs_directoryExists(dirPath);
	if (directory && !fs_directoryIsParent(directory)) {
		//el directorio existe y no tiene childrens
		char *physicalPath = string_from_format("%s/%d",myFS.filesDirectoryPath,directory->index);
		if(!fs_directoryIsEmpty(physicalPath)){
			log_error(logger,"fs_rm_dir: directory is not empty, cant remove");
			return EXIT_FAILURE;
		}

		int iterator = directory->index;
		fseek(myFS.directoryTableFile, (sizeof(t_directory)) * directory->index,
		SEEK_SET);
		myFS.amountOfDirectories--;
		while (iterator < myFS.amountOfDirectories) {
			myFS.directoryTable[iterator] = myFS.directoryTable[iterator + 1];
			fwrite(&myFS.directoryTable[iterator], sizeof(t_directory), 1,
					myFS.directoryTableFile);
			iterator++;
		}
		fflush(myFS.directoryTableFile);
		ftruncate(myFS.directoryTableFile->_fileno,
				sizeof(t_directory) * myFS.amountOfDirectories);
	} else {
		log_error(logger, "fs_rm_dir: directory doesnt exist or is parent");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
int fs_rm_block(char *filePath, int blockNumberToRemove, int numberOfCopyBlock) {
	printf("removing block %d whose copy is block %d from file %s\n",
			blockNumberToRemove, numberOfCopyBlock, filePath);

	// get parent path
	char **splitPath = string_split(filePath,"/");
	int splitPathElementCount = fs_amountOfElementsInArray(splitPath);
	int fileNameLength = strlen(splitPath[splitPathElementCount-1]);

	char *parentPath = strdup(filePath);
	parentPath[strlen(parentPath)-fileNameLength] = 0;

	//check parent path exists and get parent dir
	t_directory *parent = fs_directoryExists(parentPath);
	if(!parent){
		log_error(logger,"fs_rm: directory doesnt exist");
		return EXIT_FAILURE;
	}

	//check file exists
	//transform path to physical path
	char *physicalPath = string_from_format("%s/%d/%s",myFS.filesDirectoryPath,parent->index,splitPath[splitPathElementCount-1]);
	FILE *fileMetadata = fopen(physicalPath,"r+");
	if(!fileMetadata){
		log_error(logger,"fs_rm: file doesnt exist");
		return EXIT_FAILURE;
	}

	int amountOfBlocks = fs_getNumberOfBlocksOfAFile(physicalPath);
	int blockIterator = 0;
	ipc_struct_fs_get_file_info_response_entry *blockArray = fs_getFileBlockTuples(physicalPath);

	if(blockNumberToRemove > amountOfBlocks){
		log_error(logger,"block number to remove does not exist");
		return EXIT_FAILURE;
	}

	t_dataNode dataNode;
	t_dataNode dataNodeCopy;
	t_dataNode *targetDataNode;
	int blockNumber;
	int blockCopyNumber;

	dataNode.name = string_from_format("%s",blockArray[blockNumberToRemove].firstCopyNodeID);
	blockNumber = blockArray[blockNumberToRemove].firstCopyBlockID;
	dataNodeCopy.name = string_from_format("%s",blockArray[blockNumberToRemove].secondCopyNodeID);
	blockCopyNumber = blockArray[blockNumberToRemove].secondCopyBlockID;


	if(fs_isDataNodeAlreadyConnected(dataNode) && fs_isDataNodeAlreadyConnected(dataNodeCopy)){
		if(numberOfCopyBlock == 0){
			targetDataNode = fs_getNodeFromNodeName(dataNode.name);
			fs_cleanBlockFromDataNode(targetDataNode,blockNumber);
		}else{
			targetDataNode = fs_getNodeFromNodeName(dataNodeCopy.name);
			fs_cleanBlockFromDataNode(targetDataNode,blockCopyNumber);
		}
	}else{
		log_error(logger,"not enough copies of the block to perform rm operation");
		return EXIT_FAILURE;
	}

	fs_deleteBlockFromMetadata(physicalPath,blockNumberToRemove,numberOfCopyBlock);

	free(dataNode.name);
	free(dataNodeCopy.name);

	return EXIT_SUCCESS;
}
int fs_rename(char *filePath, char *nombreFinal) {
	printf("Renaming %s as %s\n", filePath, nombreFinal);
	t_directory *toRename;

	if (toRename = fs_directoryExists(filePath)) {
		memset(toRename->name, 0, 255);
		strcpy(toRename->name, nombreFinal);
		fseek(myFS.directoryTableFile, sizeof(t_directory) * toRename->index,
		SEEK_SET);
		fwrite(toRename, sizeof(t_directory), 1, myFS.directoryTableFile);
		fflush(myFS.directoryTableFile);
		fseek(myFS.directoryTableFile,
				sizeof(t_directory) * myFS.amountOfDirectories, SEEK_SET);
	} else {
		// get parent path
		char **splitPath = string_split(filePath,"/");
		int splitPathElementCount = fs_amountOfElementsInArray(splitPath);
		int fileNameLength = strlen(splitPath[splitPathElementCount-1]);

		char *parentPath = strdup(filePath);
		parentPath[strlen(parentPath)-fileNameLength-1] = 0;

		//check parent path exists and get parent dir
		t_directory *parent = fs_directoryExists(parentPath);
		if(!parent){
			log_error(logger,"fs_rm: directory doesnt exist");
			return EXIT_FAILURE;
		}

		//check file exists
		//transform path to physical path
		char *physicalPath = string_from_format("%s/%d/%s",myFS.filesDirectoryPath,parent->index,splitPath[splitPathElementCount-1]);
		FILE *fileMetadata = fopen(physicalPath,"r+");
		if(!fileMetadata){
			log_error(logger,"fs_rm: file doesnt exist");
			return EXIT_FAILURE;
		}
		fclose(fileMetadata);
		char *newName = string_from_format("%s/%d/%s",myFS.filesDirectoryPath,parent->index,nombreFinal);
		rename(physicalPath,newName);
		fs_updateFileFromIndex(physicalPath,newName);
		free(physicalPath);
		free(newName);
		free(parentPath);

		return EXIT_SUCCESS;
	}

	return EXIT_SUCCESS;
}
int fs_mv(char *origFilePath, char *destFilePath) {
	log_debug(logger, "moving %s to %s\n", origFilePath, destFilePath);
	t_directory *originDirectory;
	t_directory *destinationDirectory;
	int isAFile = 0;
	char *physicalFilePath = NULL;




	//chequeo si origen existe
	if (!(originDirectory = fs_directoryExists(origFilePath))) {
		//origin not a dir
		physicalFilePath = fs_isAFile(origFilePath);
		if(physicalFilePath != NULL){
			//origin is a file
			isAFile++;
			FILE *fileMetadata = fopen(physicalFilePath,"r+");
		}else{
			log_error(logger, "fs_mv: aborting mv - origin directory/file doesnt exist");
			return EXIT_FAILURE;
		}
	}

	//chequeo si destino existe
	if (!(destinationDirectory = fs_directoryExists(destFilePath))) {
		log_error(logger, "fs_mv: aborting mv - destination directory doesnt exist");
		return EXIT_FAILURE;
	}

	char *fileName = basename(origFilePath);
	char *fullFilePathInYama = string_from_format("%s/%d/%s",myFS.filesDirectoryPath,destinationDirectory->index,fileName);

	FILE * fileInYama = fopen(fullFilePathInYama,"r");
	if(fileInYama){
		log_error(logger,"fs_mv: File '%s' already exists on yama directory '%s' whose index is '%d' - Aborting mv",fileName,destinationDirectory->name,destinationDirectory->index);
		free(fullFilePathInYama);
		fclose(fileInYama);
		return EXIT_FAILURE;
	}


	char *originName = strrchr(origFilePath, '/');
	char *destinationFullName = strdup(destFilePath);
	string_append(&destinationFullName, originName);

	if (fs_directoryExists(destinationFullName) != NULL) {
		log_error(logger,
				"fs_mv: aborting mv - origin directory already exists in destination");
		return EXIT_FAILURE;
	}else{
		//check if file is contained in destination
		if(isAFile){
			if(fs_isFileContainedBy(physicalFilePath,destinationDirectory)){
				log_error(logger,
						"fs_mv: aborting mv - origin file already exists in destination");
				return EXIT_FAILURE;
			}else{
				fs_moveFileTo(physicalFilePath,destinationDirectory);
				return EXIT_SUCCESS;
			}
		}
	}

	originDirectory->parent = destinationDirectory->index;
	fseek(myFS.directoryTableFile, fs_getOffsetFromDirectory(originDirectory),
	SEEK_SET);
	fwrite(originDirectory, sizeof(t_directory), 1, myFS.directoryTableFile);
	fflush(myFS.directoryTableFile);

	return EXIT_SUCCESS;

}
int fs_cat(char *filePath) {
	printf("Showing file %s\n", filePath);

	char *fileContent = fs_readFile(filePath);

	log_info(logger,"fileContent: %s",filePath);

	return EXIT_SUCCESS;

}
int fs_mkdir(char *directoryPath) {

	if(myFS.amountOfDirectories == DIRECTORY_TABLE_MAX_AMOUNT){
		log_error(logger,"fs_mkdir: Directory table max amount reached. Aborting mkdir for directory %s", directoryPath);
		return EXIT_FAILURE;
	}

	log_info(logger,"fs_mkdir: Creating directory c %s\n", directoryPath);

	if(!fs_directoryStartsWithSlash(directoryPath)){
		log_error(logger,"fs_mkdir: Directory must start with '/'");
		return EXIT_FAILURE;
	}

	t_directory *root = malloc(sizeof(t_directory));
	root->index = 0;
	strcpy(root->name, "root");
	root->parent = -1;

	char **splitDirectory = string_split(directoryPath, "/");
	int amountOfDirectories = fs_amountOfElementsInArray(splitDirectory);

	int iterator = 0;
	t_directory *directoryReference = &myFS.directoryTable[0];
	t_directory *lastDirectoryReference;
	while (iterator < amountOfDirectories) {
		lastDirectoryReference = directoryReference;
		directoryReference = fs_childOfParentExists(splitDirectory[iterator],
				directoryReference);
		iterator++;
		if (directoryReference == NULL) {
			//no existe child para ese parent
			//ver si es el ultimo dir
			if (iterator == amountOfDirectories) {
				// no existe para ese parent, hay que crearlo
				myFS.directoryTable[myFS.amountOfDirectories].index =
						fs_getDirectoryIndex();
				strcpy(myFS.directoryTable[myFS.amountOfDirectories].name,
						splitDirectory[iterator - 1]);
				myFS.directoryTable[myFS.amountOfDirectories].parent =
						lastDirectoryReference->index;
				fwrite(&myFS.directoryTable[myFS.amountOfDirectories],
						sizeof(t_directory), 1, myFS.directoryTableFile);
				fflush(myFS.directoryTableFile);
				myFS.amountOfDirectories++;
			} else {
				//no existe y no es el ultimo, es decir no existe el parent
				// se aborta la creacion
				log_error(logger,
						"fs_mkdir: parent directory for directory to create doesnt exist");
				return EXIT_FAILURE;
			}
		} else {
			if (iterator == amountOfDirectories) {
				// dir exists, abort
				log_error(logger, "fs_mkdir: directory already exists");
				return EXIT_FAILURE;
			}
		}
	}

	return EXIT_SUCCESS;

}
int fs_cpfrom(char *origFilePath, char *yama_directory, char *fileType) {


	log_debug(logger,"fs_cpfrom:Copying %s to yama directory %s\n", origFilePath, yama_directory);
	FILE *originalFile = fopen(origFilePath, "r+");
	if (!originalFile) {
		log_error(logger, "fs_cpfrom: original file couldnt be opened");
		return EXIT_FAILURE;
	}

	t_directory *destinationDirectory = fs_directoryExists(yama_directory);
	if (!destinationDirectory) {
		log_error(logger, "fs_cpfrom: destination yama directory error");
		return EXIT_FAILURE;
	}

	if (!fileType) {
		log_error(logger, "fs_cpfrom: missing filetype");
		return EXIT_FAILURE;
	}


	char *fileName = basename(origFilePath);
	char *fullFilePathInYama = string_from_format("%s/%d/%s",myFS.filesDirectoryPath,destinationDirectory->index,fileName);


	FILE * fileInYama = fopen(fullFilePathInYama,"r");
	if(fileInYama){
		log_error(logger,"fs_cpfrom: File '%s' already exists on yama directory '%s' whose index is '%d' - Aborting cpfrom",fileName,yama_directory,destinationDirectory->index);
		free(fullFilePathInYama);
		fclose(fileInYama);
		return EXIT_FAILURE;

	}
	free(fullFilePathInYama);

	struct stat originalFileStats;
	fstat(originalFile->_fileno, &originalFileStats);

	t_fileType typeFile = !strcmp(fileType, "-b") ? T_BINARY : T_TEXT;
	void *buffer = fs_serializeFile(originalFile, originalFileStats.st_size);

	int result = fs_storeFile(yama_directory, fileName, typeFile, buffer,
			originalFileStats.st_size);
	if(result == EXIT_SUCCESS) log_debug(logger,"fs_cpfrom: Stored filed successfully");
	return EXIT_SUCCESS;

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

	if(directoryPath == NULL) return EXIT_FAILURE;

	log_info(logger,"Showing directory %s\n", directoryPath);
	int iterator = 0;
	if (!strcmp(directoryPath, "-fs")) {
		printf("\n[index]    [name]    [parent]\n");
		while (iterator < myFS.amountOfDirectories) {
			printf("   %d    |", myFS.directoryTable[iterator].index);
			printf("   %s    ", myFS.directoryTable[iterator].name);
			printf("   %d    \n", myFS.directoryTable[iterator].parent);
			iterator++;
		}
	}else{
		t_directory *directory = fs_directoryExists(directoryPath);
		if(!directory){
			log_error(logger,"fs_rm: directory doesnt exist");
			return EXIT_FAILURE;
		}
		char *physicalPath = string_from_format("%s/%d",myFS.filesDirectoryPath,directory->index);
		DIR *dir = opendir(physicalPath);
		if (!dir)
		{
		    /* Directory doesnt exist. */
			free(physicalPath);
		    closedir(dir);
		    printf(" \n");
		    return 0;
		}

		char *command = string_from_format("ls %s",physicalPath);
		int result = system(command);
		free(physicalPath);
		free(command);
		return 0;
	}

	return 0;

}
int fs_info(char *filePath) {

	int amountOfBlocks = fs_getNumberOfBlocksOfAFile(filePath);


	t_fileBlockTuple *arrayOfBlockTuples = fs_getFileBlockTuples(filePath);
	printf("Showing info of file file %s\n", filePath);


	int i = 0;

	for(i = 0; i < amountOfBlocks; i++){

		fs_dumpBlockTuple(arrayOfBlockTuples[i]);
	}

	fs_destroyNodeTupleArray(arrayOfBlockTuples, amountOfBlocks);
	return EXIT_SUCCESS;

}

/************************************* SOCKET CONNECTION FUNCTIONS *************************************/

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

		log_debug(logger,"fs_waitForDataNodes: New connection accepted!");
		pthread_t newDataNodeThread;

		// Copy the value of the accepted socket, in order to pass to the thread
		int new_sock = new_dataNode_socket;

		if (pthread_create(&newDataNodeThread, NULL,
				fs_dataNodeConnectionHandler, (void*) new_dataNode_socket)
				< 0) {
			log_error(logger,"fs_waitForDataNodes: Error creating thread after new connection!");
		}

		log_debug(logger,"fs_waitForDataNodes: Handler assigned");
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
	address.sin_port = htons(8081);

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
		ipc_struct_fs_get_file_info_request *request = ipc_recvMessage(new_socket, FS_GET_FILE_INFO_REQUEST);
		printf("Request: %s", request->filePath);
		printf("Hello message sent\n");

		ipc_struct_fs_get_file_info_response *response = fs_yamaFileBlockTupleResponse(request->filePath);
		ipc_sendMessage(new_socket, FS_GET_FILE_INFO_RESPONSE, response);
		int length = fs_getNumberOfBlocksOfAFile(request->filePath);
		fs_destroyNodeTupleArray(response->entries, length);
		sleep(5);
	}

}
int fs_isStable() {
	int amountOfConnectedNodes = list_size(connectedNodes);
	if (amountOfConnectedNodes <= 1) {
		log_error(logger, "fs_isStable: not enough datanodes connected");
		return EXIT_FAILURE;
	}

	FILE *fileListFileDescriptor = fopen(myFS.FSFileList, "r+");
	if (fileListFileDescriptor == NULL) {
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

		char *fullFilePath = string_from_format("%s", buffer);
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
			char **nodeBlockTupleAsArray = NULL;
			int blockNumber = -1;

			if(nodeBlockTupleAsString!=NULL){
				nodeBlockTupleAsArray = string_get_string_as_array(
									nodeBlockTupleAsString);
				blockNumber = atoi(nodeBlockTupleAsArray[1]);
			}

			char *nodeName = NULL;
			if(nodeBlockTupleAsArray != NULL){
				nodeName = nodeBlockTupleAsArray[0];
			}
			//si tenes el nodo y el nodo tiene el bloque esta OK, me chupa un huevo la data
			if (!fs_checkNodeBlockTupleConsistency(nodeName,
					blockNumber)) {
				free(blockToSearch);
				iterator++;
				copy = 0;
			} else {
				if (!copy) {
					copy = 1;
				} else {
					//no pudo armar el bloque
					log_debug(logger, "fs_isStable:consistency error for file %s", buffer);
					log_debug(logger, "fs_isStable:missing block no. %s", blockToSearch);
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
	sem_t *mutex = malloc(sizeof(sem_t));
	sem_t *results = malloc(sizeof(sem_t));
	int valread, cant;
	char buffer[1024] = { 0 };
	char *hello = "You are connected to the FS";
	int new_socket = (int *) dataNodeSocket;

	t_dataNode *newDataNode = malloc(sizeof(t_dataNode));

	//Read Node name
	valread = read(new_socket, buffer, 1024);

	newDataNode->name = malloc(sizeof(buffer));
	memset(newDataNode->name, 0, sizeof(newDataNode));
	strcpy(newDataNode->name, buffer);
	log_debug(logger,"fs_dataNodeConnectionHandler: New node name: %s", newDataNode->name);

	if(fs_isDataNodeAlreadyConnected(*newDataNode)){//Dont accept another node with same ID a
		log_error(logger, "fs_connectionHandler: Node %s already connected - Aborting connection", newDataNode->name);
		int closeResult = close(new_socket);
		if(closeResult < 0) log_error(logger,"fs_connectionHandler: Couldnt close socket fd when trying to restore from previous session");
		pthread_cancel(pthread_self());
		return;

	}
	//If the Node isnt already connected->include it to the global lists of connected nodes
	sem_init(mutex,0,0);
	sem_init(results,0,0);
	newDataNode->threadSemaphore = mutex;
	newDataNode->resultSemaphore = results;
	newDataNode->operationsQueue = queue_create();
	newDataNode->resultsQueue = queue_create();
	list_add(connectedNodes, newDataNode);


	/*if(myFS.usePreviousStatus){ //Only accepts nodes with IDs from the Node Table
		log_info(logger,"FS will restore previous session");

		if(!fs_isNodeFromPreviousSession(newDataNode)){ //If the ID isnt in the Node Table the connection thread will be aborted

			if(!(fs_amountOfConnectedNodesFromPreviousStatus() == list_size(previouslyConnectedNodesNames))){//If all the previously connected nodes didnt connect, do not allow any other node
				log_error(logger, "fs_connectionHandler: Node %s isnt from previous session - Aborting connection", newDataNode.name);
				int closeResult = close(new_socket);
				if(closeResult < 0) log_error(logger,"fs_connectionHandler: Couldnt close socket fd when trying to restore from previous session");
				pthread_cancel(pthread_self());
				return;
			}

		}


	}

*/

	//Send connection confirmation
	send(new_socket, hello, strlen(hello), 0);

	//Read amount of blocks
	read(new_socket, &cant, sizeof(int));
	cant = ntohl(cant);
	newDataNode->amountOfBlocks = cant;
	printf("Amount of blocks:%d\n", newDataNode->amountOfBlocks);


	fs_openOrCreateBitmap(myFS, newDataNode);

	myFS.totalAmountOfBlocks += newDataNode->amountOfBlocks;
	newDataNode->freeBlocks = fs_getAmountOfFreeBlocksOfADataNode(newDataNode);
	newDataNode->occupiedBlocks = newDataNode->amountOfBlocks - newDataNode->freeBlocks;

	log_info(logger,"fs_connectionHandler: Node: [%s] connected / Total:[%d], Free:[%d], Occupied:[%d]", newDataNode->name, newDataNode->amountOfBlocks, newDataNode->freeBlocks, newDataNode->occupiedBlocks);



	int nodeTableUpdate = fs_updateNodeTable(*newDataNode);

	/*if (nodeTableUpdate == DATANODE_ALREADY_CONNECTED) {
	 log_error(logger,
	 "Node table update failed.\n");

	 //Matar hilo

	 return;
	 }*/

	while (1) {

		//printf("DataNode %s en FS ala espera de pedidos\n", newDataNode.name);
		sem_wait(mutex);

		//pop operation
		t_threadOperation *operation = queue_pop(newDataNode->operationsQueue);
		int serializedOperationSize;
		//serializo
		if(operation->operationId){
			serializedOperationSize = sizeof(uint32_t)*2 + BLOCK_SIZE;
		}else{
			serializedOperationSize = sizeof(uint32_t)*2;
		}

		void *serializedOperation = malloc(serializedOperationSize);
		memset(serializedOperation,0,serializedOperationSize);
		int offset = 0;

		memcpy(serializedOperation+offset,&operation->operationId,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		memcpy(serializedOperation+offset,&operation->blockNumber,sizeof(uint32_t));
		offset+=sizeof(uint32_t);

		if(operation->operationId){
			memcpy(serializedOperation+offset,operation->buffer,BLOCK_SIZE);
		}

		//send operation
		send(new_socket, serializedOperation, serializedOperationSize, 0);


		void *result;
		if(!operation->operationId){
			result = malloc(BLOCK_SIZE);
			recv(new_socket, result, BLOCK_SIZE,MSG_WAITALL);
			//todo agregar a la queue de resultados
			queue_push(newDataNode->resultsQueue,result);
			sem_post(newDataNode->resultSemaphore);
		}

		free(operation);

		sleep(5);

	}
}

/************************************* FILE SYSTEM FUNCTIONS *************************************/
//Functions to mount FS structures
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
			log_debug(logger, "fs_openOrCreateDirectory: Directory: %s created",
					myFS.mountDirectoryPath);
		} else {
			log_error(logger, "fs_openOrCreateDirectory: error creating path %s", strerror(errno));
			return -1;
		}
	}
	log_debug(logger, "fs_openOrCreateDirectory: Found directory %s",directory);
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
		log_debug(logger, "fs_openOrCreateNodeTableFile: Found node table file");
		myFS.nodeTableFileDescriptor = output->_fileno;
		return 0;
	} else { //No puede abrirlo => Lo crea
		log_debug(logger, "fs_openOrCreateNodeTableFile: Node table file not found - creating from scratch");
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

	int isAlreadyInTable = 0;
	if (fs_arrayContainsString(listaNodosArray, aDataNode.name) == 0
			&& fs_amountOfElementsInArray(listaNodosArray) > 0) { //Si esta en la lista, no lo agrego

		log_debug(logger, "fs_updateNodeTable: DataNode is already in DataNode Table\n");

		isAlreadyInTable = 1;
		//config_destroy(nodeTableConfig);
		//return DATANODE_ALREADY_CONNECTED;

	}
	/*************** ACTUALIZA TAMANIO ***************/

	if (!isAlreadyInTable) {
		char * tamanioOriginal = config_get_string_value(nodeTableConfig,
				"TAMANIO"); //Stores original value of TAMANIO
		int tamanioAcumulado = atoi(tamanioOriginal) + aDataNode.amountOfBlocks; //Suma tamanio original al del nuevo dataNode
		char *tamanioFinal = string_from_format("%d", tamanioAcumulado); // Pasa el valor a string
		config_set_value(nodeTableConfig, "TAMANIO", tamanioFinal); //Toma el valor
		myFS.totalAmountOfBlocks = tamanioAcumulado;

	}

	/*************** ACTUALIZA LIBRE ***************/
	int libresFinal = fs_getTotalFreeBlocksOfConnectedDatanodes(connectedNodes); //Suma todos los bloques libres usando la lista de bloques conectados

	if (libresFinal == -1) { //Si la lista esta vacia osea que es el primer data node en conectarse
		char *libreTotalFinal = string_from_format("%d", aDataNode.freeBlocks); //La pasa a string
		config_set_value(nodeTableConfig, "LIBRE", libreTotalFinal);
		myFS.freeBlocks = aDataNode.freeBlocks;
	} else {
		//int bloquesLibresTotales = myFS.freeBlocks - libresFinal;
		char *libreTotalFinal = string_from_format("%d", libresFinal); //La pasa a string
		config_set_value(nodeTableConfig, "LIBRE", libreTotalFinal);
		myFS.freeBlocks = libresFinal;
	}

	myFS.occupiedBlocks = myFS.totalAmountOfBlocks - myFS.freeBlocks;

	/*************** ACTUALIZA LISTA NODOS EN NODE TABLE ***************/

	if (!isAlreadyInTable) {
		char *listaNodosFinal;
		int tamanioArrayNuevo =
				(fs_amountOfElementsInArray(listaNodosArray) + 1)
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

				log_debug(logger, "fs_updateNodeTable: DataNode is already in DataNode Table\n");

			} else { //Si no esta en la lista lo agrego
				//listaNodosArray = ["NODOA","NODOB"] y quiero meter "NODOC"

				for (i = 0; i < fs_amountOfElementsInArray(listaNodosArray);
						i++) { //Vuelco a listaNodosAux todos los nodos del array
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

	}

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

		//cada array en [0] tiene el string a usar sin el =
		char**  libresArray = string_split(libres, "=");
		char**  totalesArray = string_split(total, "=");

		config_set_value(nodeTableConfig, libresArray[0], bloquesLibresFinal);
		config_set_value(nodeTableConfig, totalesArray[0], bloquesTotalesFinal);
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
int fs_openOrCreateBitmap(t_FS FS, t_dataNode *aDataNode) {

	char *bitmapRootPath = "/mnt/FS/metadata/bitmaps/";
	char *bitmapFullPath = malloc(
			strlen(bitmapRootPath) + strlen(aDataNode->name) + strlen(".dat")
					+ 1);
	strcpy(bitmapFullPath, bitmapRootPath);
	strcat(bitmapFullPath, aDataNode->name);
	strcat(bitmapFullPath, ".dat");

	if (aDataNode->bitmapFile = fopen(bitmapFullPath, "r+")) { //Existe el archivo de bitmap del dataNode
		log_debug(logger,
				"fs_openOrCreateBitmap: bitmap of data node %s found. Wont create from scratch",
				aDataNode->name);
		log_debug(logger, "fs_openOrCreateBitmap: Mapping bitmap to memory");

		//TODO: hacer que tome la informacion anterior

		fs_mmapDataNodeBitmap(bitmapFullPath, aDataNode);

		aDataNode->bitmap = bitarray_create_with_mode(
				aDataNode->bitmapMapedPointer, aDataNode->amountOfBlocks / 8,
				LSB_FIRST);

		log_debug(logger, "Bitmap of datanode %s opened sucessfully",
				aDataNode->name);

		return EXIT_SUCCESS;

	} else { //No puede abrirlo => Lo crea
		log_debug(logger,
				"fs_openOrCreateBitmap: Bitmap file for datanode %s not found. Creating with parameters of config file",
				aDataNode->name);
		aDataNode->bitmapFile = fopen(bitmapFullPath, "w+");
		float amountOfBlocks = aDataNode->amountOfBlocks;
		fs_writeNBytesOfXToFile(aDataNode->bitmapFile,
				ceilf(amountOfBlocks / 8), 0);

		log_debug(logger, "fs_openOrCreateBitmap: Maping bitmap of datanode %s to memory",
				aDataNode->name);

		int mmapResult = fs_mmapDataNodeBitmap(bitmapFullPath, aDataNode);

		if (mmapResult == EXIT_FAILURE) {
			log_error(logger, "fs_openOrCreateBitmap: Maping  of datanode %s  bitmap to memory failed",
					aDataNode->name);

			return EXIT_FAILURE;
		}

		aDataNode->bitmap = bitarray_create_with_mode(
				aDataNode->bitmapMapedPointer, aDataNode->amountOfBlocks / 8,
				LSB_FIRST);
		log_debug(logger, "fs_openOrCreateBitmap: bitmap created with parameters");

		return EXIT_SUCCESS;

	}

	log_error(logger, "fs_openOrCreateBitmap: Bitmap of datanode %s could not be opened or created",
			aDataNode->name);

	return EXIT_FAILURE;

}
int fs_getTotalFreeBlocksOfConnectedDatanodes(t_list *connectedDataNodes) {

	int listSize = list_size(connectedNodes);
	int i;
	int totalFreeBlocks = 0;
	t_dataNode * aux;
	if (listSize == 0) {
		log_error(logger,
				"fs_getTotalFreeBlocks:No data nodes connected. Cant get total amount of free b\n");
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
	myFS.directoryTableFile = fopen(myFS.directoryTablePath, "rb+");
	int iterator = 0;
	t_directory *directoryRecord;
	int bytesRead;

	if (myFS.directoryTableFile == NULL) {
		//hay que crearlo
		myFS.amountOfDirectories = 0;
		myFS.directoryTableFile = fopen(myFS.directoryTablePath, "wb+");

		//cargo root
		myFS.directoryTable[0].index = 0;
		strcpy(myFS.directoryTable[0].name, "root");
		myFS.directoryTable[0].parent = -1;

		//escribo root
		int test;
		test = fwrite(&myFS.directoryTable[0], sizeof(t_directory), 1,
				myFS.directoryTableFile);
		fflush(myFS.directoryTableFile);

		myFS.amountOfDirectories++;
		myFS._directoryIndexAutonumber =
				myFS.directoryTable[myFS.amountOfDirectories - 1].index + 1;
		return EXIT_SUCCESS;
	} else {
		//hay que cargarlo
		myFS.amountOfDirectories = 0;
		while (bytesRead != 0) {
			bytesRead = fread(&myFS.directoryTable[iterator],
					sizeof(t_directory), 1, myFS.directoryTableFile);
			if (bytesRead) {
				myFS.amountOfDirectories++;
			}
			iterator++;
		}
		myFS._directoryIndexAutonumber =
				myFS.directoryTable[myFS.amountOfDirectories - 1].index + 1;
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
				"fs_includeDirectory: Limit of directory table reached. Aborting directory table update\n");
		return -1;
	}

	for (i = 0; i < amountOfDirectoriesToInclude; i++) { //Por cada subdirectorio del directorio pasado como parametro

		if (!fs_isDirectoryIncludedInDirectoryTable(subDirectories[i],
				directoryTable)) { //No esta incluido en la tabla
			//Me fijo cual es la primer posicion libre del array
			index = fs_getFirstFreeIndexOfDirectoryTable(directoryTable); //Guardo en index la posicion a actualizar en la tabla
			if (index == -1) {
				log_error(logger,
						"fs_includeDirectory: Limit of directory table reached. Aborting directory table update\n");
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

	log_error(logger, "fs_getFirstFreeIndex: Limit of directory table reached\n");
	return EXIT_FAILURE;

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
int fs_writeNBytesOfXToFile(FILE *fileDescriptor, int N, int C) { //El tamanio del archivo antes del mmap matchea con el tamanio del del archivo
	char *buffer = malloc(N);
	memset(buffer, C, N);
	fwrite(buffer, N, 1, fileDescriptor);
	fflush(fileDescriptor);
	return EXIT_SUCCESS;
}
int fs_checkNodeBlockTupleConsistency(char *dataNodeName, int blockNumber) { //No puede abrirlo => Lo crea
	if(dataNodeName == NULL || blockNumber == -1){
		return EXIT_FAILURE;
	}
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
int fs_getPreviouslyConnectedNodesNames() {

	nodeTableConfig = config_create(myFS.nodeTablePath);

	char *listaNodosOriginal = config_get_string_value(nodeTableConfig,
			"NODOS"); //"[Nodo1, Nodo2]"
	char **listaNodosArray = string_get_string_as_array(listaNodosOriginal); //["Nodo1","Nodo2"]

	if (fs_amountOfElementsInArray(listaNodosArray) == 0) {
		log_debug(logger,
				"No previously connected nodes - FS is starting from scratch");
		return NULL;
	}

	int i = 0;

	for (i = 0; i < fs_amountOfElementsInArray(listaNodosArray); i++) {
		list_add(previouslyConnectedNodesNames, listaNodosArray[i]);
	}

	config_destroy(nodeTableConfig);
	return EXIT_SUCCESS;

}
float fs_bytesToMegaBytes(int bytes) {
	float bytesInFloat = bytes;
	return bytesInFloat / 1024 / 1024;
}
t_directory *fs_childOfParentExists(char *child, t_directory *parent) {
	int iterator = 0;
	while (iterator < 100) {
		if (!strcmp(myFS.directoryTable[iterator].name, child)) {
			if (myFS.directoryTable[iterator].parent == parent->index) {
				return &myFS.directoryTable[iterator];
			}
		}
		iterator++;
	}
	return NULL;
}
t_directory *fs_directoryExists(char *directory) {

//	t_directory *root = malloc(sizeof(t_directory));
//	root->index = 0;
//	strcpy(root->name, "root");
//	root->parent = -1;

	char **splitDirectory = string_split(directory, "/");
	int amountOfDirectories = fs_amountOfElementsInArray(splitDirectory);

	int iterator = 0;
	t_directory *directoryReference = &myFS.directoryTable[0];
	t_directory *lastDirectoryReference;
	while (iterator < amountOfDirectories) {
		lastDirectoryReference = directoryReference;
		directoryReference = fs_childOfParentExists(splitDirectory[iterator],
				directoryReference);
		if (directoryReference == NULL) {
			//no existe child para ese parent
			log_error(logger,
					"fs_directoryExists: parent directory for directory to create doesnt exist");
			return NULL;
		} else {
			iterator++;

			if (iterator == amountOfDirectories) {
				// dir exists, abort
				//log_error(logger,"directory already exists puto");
				return directoryReference;
			}
		}
	}

	return NULL;
}
int fs_directoryIsParent(t_directory *directory) {
	int iterator = 0;

	while (iterator < 100) {
		if (myFS.directoryTable[iterator].parent == directory->index) {
			return 1;
		}
		iterator++;
	}
	return 0;
}
int fs_directoryIsEmpty(char *dirname) {
  int n = 0;
  struct dirent *d;
  DIR *dir = opendir(dirname);
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
int fs_getDirectoryIndex() {
	int returnNumber = myFS._directoryIndexAutonumber;
	myFS._directoryIndexAutonumber++;
	return returnNumber;
}
int fs_getOffsetFromDirectory(t_directory *directory) {
	int iterator = 0;

	while (iterator < myFS.amountOfDirectories) {
		if (myFS.directoryTable[iterator].index == directory->index) {
			return sizeof(t_directory) * iterator;
		}
		iterator++;
	}

	return EXIT_FAILURE;
}
int fs_storeFile(char *fullFilePath, char *fileName, t_fileType fileType,
		void *buffer, int fileSize) {
	//check if there's enough space in system (filesize * 2), else abort

	int totalFileSize = 2 * fileSize;
	int amountOfBlocks =
			(totalFileSize % BLOCK_SIZE) ?
					(totalFileSize / BLOCK_SIZE) + 2 :
					totalFileSize / BLOCK_SIZE;



	if(myFS.freeBlocks < amountOfBlocks){
		log_error(logger,"fs_storeFile: Cant store file '%s' as it requires [%d] blocks and there are currently [%d] free blocks on FS ",fileName,amountOfBlocks,myFS.freeBlocks);
		return EXIT_FAILURE;
	}


	//check if parent dir exists
	t_directory *destinationDirectory = fs_directoryExists(fullFilePath);
	if (!destinationDirectory) {
		log_error(logger, "fs_storeFile: destination directory doesnt exist");
		return EXIT_FAILURE;
	}

	//generate metadata folder --> according to file type
	char *fileMetadataDirectory = string_from_format("%s/%d",
			myFS.filesDirectoryPath, destinationDirectory->index);
	fs_openOrCreateDirectory(fileMetadataDirectory, 0);

	//create metadata file in metadata folder
	char *filePathWithName = string_from_format("%s/%s", fileMetadataDirectory,
			fileName);
	FILE *metadataFile = fopen(filePathWithName, "w+");
	if (!metadataFile) {
		log_error(logger, "fs_storeFile: Couldnt create metadata file");
		return EXIT_FAILURE;
	}
	t_config *metadataFileConfig = config_create(filePathWithName);

	//split buffer in packages --> according to file type
	t_list *packageList = list_create();

	void *bufferSplit;
	int offset = 0;
	int splitSize = 0;
	int blockNumber = 0;
	int copy = 0;
	t_blockPackage *package;

	int remainingSizeToSplit = fileSize;
	while (remainingSizeToSplit) {
		if (remainingSizeToSplit < BLOCK_SIZE) {
			//single block remaining
			splitSize = remainingSizeToSplit;
			remainingSizeToSplit = 0;
		} else {
			splitSize = BLOCK_SIZE;
			remainingSizeToSplit -= BLOCK_SIZE;
		}

		bufferSplit = malloc(BLOCK_SIZE);
		memset(bufferSplit, 0, BLOCK_SIZE);
		memcpy(bufferSplit, buffer + offset, splitSize);
		offset += splitSize;

		t_dataNode *exclude = NULL;
		for (copy = 0; copy < 2; copy++) {
			package = malloc(sizeof(t_blockPackage));
			package->blockCopyNumber = copy;
			package->blockNumber = blockNumber;
			package->blockSize = BLOCK_SIZE;
			package->buffer = bufferSplit;
			//decide nodes to deliver original blocks and copy
			package->destinationNode = fs_getDataNodeWithMostFreeSpace(exclude);
			exclude = package->destinationNode;
			package->destinationBlock = fs_getFirstFreeBlockFromNode(
					package->destinationNode);
			list_add(packageList, package);
		}
		blockNumber++;
	}

	//wait for receipt confirmation of all packages, update admin structures
	//list failed packages and resend to 2nd alternative, else abort.
	int operationStatus;

	if (operationStatus = fs_sendPackagesToCorrespondingNodes(packageList)) {
		log_error(logger, "fs_storeFile: Error with data transmition to nodes");
		return EXIT_FAILURE;
	}

	//set metadata values
	char *fileSizeString = string_from_format("%d", fileSize);
	config_set_value(metadataFileConfig, "TAMANIO", fileSizeString);

	if (fileType == T_BINARY) {
		config_set_value(metadataFileConfig, "TIPO", "BINARIO");
	} else {
		if (fileType == T_TEXT) {
			config_set_value(metadataFileConfig, "TIPO", "TEXT");
		} else {
			log_error(logger, "fs_storeFile: Unrecognized filetype");
			return EXIT_FAILURE;
		}
	}

	int iterator = 0;
	char *blockString;
	char *blockSizeString;
	char *blockNumberString;
	char *blockSizeValueString;
	char *nodeBlockString;
	int oldBlock = 0;

	while (iterator < list_size(packageList)) {
		t_blockPackage *currentBlock = list_get(packageList, iterator);

		blockString = string_from_format("BLOQUE%dCOPIA%d",
				currentBlock->blockNumber, currentBlock->blockCopyNumber);
		nodeBlockString = string_from_format("[%s, %d]",
				currentBlock->destinationNode->name,
				currentBlock->destinationBlock);

		config_set_value(metadataFileConfig, blockString, nodeBlockString);
		free(blockString);
		free(nodeBlockString);

		if (oldBlock == currentBlock->blockNumber) {
			blockSizeString = string_from_format("%d", currentBlock->blockSize);
			blockNumberString = string_from_format("BLOQUE%dBYTES", oldBlock);
			config_set_value(metadataFileConfig, blockNumberString,
					blockSizeString);
			free(blockSizeString);
			free(blockNumberString);
			oldBlock++;
		}

		iterator++;
	}


	FILE *fileIndex = fopen(myFS.FSFileList,"a+");
	if(!fileIndex){
		fileIndex = fopen(myFS.FSFileList,"w+");
	}
	char *filePathWithNameAndNewline = string_from_format("%s\n",filePathWithName);
	fseek(fileIndex,0,SEEK_END);
	fwrite(filePathWithNameAndNewline,strlen(filePathWithNameAndNewline),1,fileIndex);
	fclose(fileIndex);

	config_save(metadataFileConfig);
	config_destroy(metadataFileConfig);
	fclose(metadataFile);
	free(filePathWithName);
	free(filePathWithNameAndNewline);
	return EXIT_SUCCESS;


}
int *fs_sendPackagesToCorrespondingNodes(t_list *packageList) {
	//todo: testear
	int listSize = list_size(packageList);
	int iterator = 0;
	t_threadOperation *operation;

	// mando paquetes
	while(iterator < listSize){
		//armo operacion global
		t_blockPackage *package = list_get(packageList,iterator);
		operation = malloc(sizeof(t_threadOperation));
		operation->operationId = 1; //todo: reemplazar por define 0=read 1=write
		operation->size = package->blockSize;
		operation->buffer = malloc(BLOCK_SIZE);
		memcpy(operation->buffer,package->buffer,BLOCK_SIZE);
		operation->blockNumber = package->destinationBlock;

		//pusheo al datanode
		queue_push(package->destinationNode->operationsQueue,operation);

		//signaleo al thread
		sem_post(package->destinationNode->threadSemaphore);
		iterator++;
	}
	return EXIT_SUCCESS; //hardcodeado durlock ad handshake
}
int fs_getFirstFreeBlockFromNode(t_dataNode *dataNode) {

	int counter = 0;
	while (counter < dataNode->amountOfBlocks) {
		if (!bitarray_test_bit(dataNode->bitmap, counter)) {
			fs_setDataNodeBlock(dataNode, counter);
			log_debug(logger,"fs_getFirstFreeBlock: First free block of Node: [%s] is block: [%d]", dataNode->name, counter);
			return counter;
		}
		counter++;
	}
	return NULL;
}
t_dataNode *fs_getDataNodeWithMostFreeSpace(t_dataNode *excluding) {
	//todo: implementar recorriendo lista de nodos conectados

	int listSize = list_size(connectedNodes);
	int i;
	int maxFreeSpace = 0;
	t_dataNode * aux;
	t_dataNode * output = NULL;

	if (listSize == 0) {
		log_error(logger,
				"fs_getDataNodeWithMostFreeSpace: No Data Nodes connected - cant get node with most free space");
		return NULL;
	}

	for (i = 0; i < listSize; i++) {
		aux = list_get(connectedNodes, i);
		if (aux->freeBlocks > maxFreeSpace) {
			if (excluding != NULL) {
				if (strcmp(aux->name, excluding->name)) {
					maxFreeSpace = aux->freeBlocks;
					output = aux;
				}
			} else {
				maxFreeSpace = aux->freeBlocks;
				output = aux;
			}

		}
	}

	if (output != NULL) {
		log_debug(logger, "fs_getDataNodeWithMostFreeSpace: The dataNode with most free space is %s",
				output->name);
		return output;
	} else {

		log_error(logger,
				"fs_getDataNodeWithMostFreeSpace: All dataNodes are full - cant get node with most free space");
		return NULL;
	}

}
void *fs_serializeFile(FILE *file, int fileSize) {
	void *buffer = malloc(fileSize);
	memset(buffer, 0, fileSize);

	int result = fread(buffer, fileSize, 1, file);
	return buffer;
}
int fs_mmapDataNodeBitmap(char * bitmapPath, t_dataNode *aDataNode) {

	struct stat mystat;

	//aDataNode->bitmapFileDescriptor = open(bitmapPath, O_RDWR);

	if (aDataNode->bitmapFile->_fileno == -1) {
		log_error(logger,
				"fs_mmapDataNodeBitmap: Error opening bitmap file of datanode %s in order to map to memory",
				aDataNode->name);
		return EXIT_FAILURE;
	}

	if (fstat(aDataNode->bitmapFile->_fileno, &mystat) < 0) {
		log_error(logger,
				"fs_mmapDataNodeBitmap: Error at fstat of data node %s in order to map to memory",
				aDataNode->name);
		return EXIT_FAILURE;

	}

	aDataNode->bitmapMapedPointer = mmap(0, mystat.st_size,
	PROT_READ | PROT_WRITE, MAP_SHARED, aDataNode->bitmapFile->_fileno, 0);

	if (aDataNode->bitmapMapedPointer == MAP_FAILED) {
		log_error(logger,
				"fs_mmapDataNodeBitmap: Error creating mmap pointer to bitmap file of datanode %s",
				aDataNode->name);
		return EXIT_FAILURE;

	}

	return EXIT_SUCCESS;

}
void fs_dumpDataNodeBitmap(t_dataNode aDataNode) {
	int i = 0;
	int unBit = 0;

	while (i < aDataNode.amountOfBlocks) {
		unBit = bitarray_test_bit(aDataNode.bitmap, i);
		log_info(logger, "bit %d: %d", i, unBit);
		i++;
	}
}
int fs_getAmountOfFreeBlocksOfADataNode(t_dataNode *aDataNode) {


	int i;
	int output = 0;

	//Comienzo a recorrer el bitmap
	for (i = 0; i < aDataNode->amountOfBlocks; i++) {
		if (!bitarray_test_bit(aDataNode->bitmap, i)) {
			output++;
		}
	}

	return output;

}

int fs_setDataNodeBlock(t_dataNode *aDataNode, int blockNumber) {

	if(aDataNode->occupiedBlocks == aDataNode->amountOfBlocks){ //No hay bloques libres
		log_error(logger,"fs_setDataNodeBlock: Cant Set block number %d of datanode %s - not enough space", blockNumber, aDataNode->name);
		return EXIT_FAILURE;
	}

	if(bitarray_test_bit(aDataNode->bitmap, blockNumber)){
		log_error(logger,"fs_setDataNodeBlock: Cant Set block number %d of datanode %s - block already set", blockNumber, aDataNode->name);
		return EXIT_FAILURE;
	}

	bitarray_set_bit(aDataNode->bitmap, blockNumber);
	aDataNode->freeBlocks--;
	aDataNode->occupiedBlocks++;
	fs_updateNodeTable(*aDataNode);

	log_debug(logger,"fs_setDataNodeBlock: Set block number %d of datanode %s", blockNumber, aDataNode->name);

	return EXIT_SUCCESS;

}

int fs_cleanBlockFromDataNode(t_dataNode *aDataNode, int blockNumber) {


	if(aDataNode->freeBlocks  == aDataNode->amountOfBlocks){ //Estan todos libres
		log_error(logger,"fs_cleanBlockFromDataNode: Cant Set block number %d of datanode %s - not enough space", blockNumber, aDataNode->name);
		return EXIT_FAILURE;
	}

	if(!bitarray_test_bit(aDataNode->bitmap, blockNumber)){
		log_error(logger,"fs_cleanBlockFromDataNode: Cant Set block number %d of datanode %s - block already clean", blockNumber, aDataNode->name);
		return EXIT_FAILURE;
	}

	bitarray_clean_bit(aDataNode->bitmap, blockNumber);
	aDataNode->freeBlocks++;
	aDataNode->occupiedBlocks--;
	fs_updateNodeTable(*aDataNode);

	log_debug(logger,"fs_cleanBlockFromDataNode: Cleaned block number %d of datanode %s", blockNumber, aDataNode->name);

	return EXIT_SUCCESS;


}

int fs_restorePreviousStatus(){

	log_info(logger,"Restoring FS from previous status");
	myFS.usePreviousStatus = 1;

	fs_getPreviouslyConnectedNodesNames(); //Updates previouslyConnectedNodesNames global list

	return EXIT_SUCCESS;

}

int fs_isNodeFromPreviousSession(t_dataNode aDataNode){

	int amountOfNodes = list_size(previouslyConnectedNodesNames);
	int i;


	for(i = 0; i < amountOfNodes ; i++){
		if(!strcmp(aDataNode.name, list_get(previouslyConnectedNodesNames, i))){
			log_info(logger, "Node:[%s] was in previous session", aDataNode.name);
			return DATANODE_IS_FROM_PREVIOUS_SESSION;
		}
	}
	log_error(logger, "Node:[%s] was not in previous session", aDataNode.name);

	return 0;

}
int fs_isDataNodeAlreadyConnected(t_dataNode aDataNode){

	int amountOfNodes = list_size(connectedNodes);
	int i;
	t_dataNode *aux = NULL;
	for(i = 0; i < amountOfNodes ; i++){
		aux = list_get(connectedNodes, i);
		if(!strcmp(aDataNode.name, aux->name)){
			log_error(logger, "Node:[%s] is already connected previous session. Aborting connection", aDataNode.name);
			return DATANODE_ALREADY_CONNECTED;
		}
	}
	log_info(logger, "Node:[%s] isnt already connected to FS", aDataNode.name);

	return 0;


}

ipc_struct_fs_get_file_info_response_entry *fs_getFileBlockTuples(char *filePath){


	FILE * fileToOpen = fopen(filePath,"r");
		if(fileToOpen == NULL){
			log_error(logger,"fs_getAmountOfBlocks: File %s does not exist",filePath);
			close(fileToOpen);
			return EXIT_FAILURE;
		}

	close(fileToOpen);



	int amountOfBlocks = fs_getNumberOfBlocksOfAFile(filePath);
	ipc_struct_fs_get_file_info_response_entry *output = malloc(sizeof(ipc_struct_fs_get_file_info_response_entry) * amountOfBlocks);


	int i = 0;
	int j = 0;
	int copy = 0;

	for(i = 0; i < amountOfBlocks; i++){

		copy = 0;

		for(j = 0; j < 2; j++){

			t_config *fileConfig = config_create(filePath);//Creo el config
			ipc_struct_fs_get_file_info_response_entry *currentTuple = output + i;

			char *blockToSearch = string_from_format("BLOQUE%dCOPIA%d",
					i, copy);
			char *nodeBlockTupleAsString = config_get_string_value(fileConfig,
									blockToSearch);
			if(nodeBlockTupleAsString == NULL){
				log_error(logger,"fs_getFileBlockTuples: Block tuple: %s not found",blockToSearch);
				config_destroy(fileConfig);

				if(j==0){
					currentTuple->firstCopyNodeID = NULL;
					currentTuple->firstCopyBlockID = BLOCK_DOES_NOT_EXIST;
				}

				if(j==1){
					currentTuple->secondCopyNodeID = NULL;
					currentTuple->secondCopyBlockID = BLOCK_DOES_NOT_EXIST;
				}

				if(copy==1)copy=0;
				if(copy==0)copy=1;
				continue;
			}

			char *blockSizeString = string_from_format("BLOQUE%dBYTES",	i);
			char * blockSize = config_get_string_value(fileConfig,blockSizeString);
			currentTuple->blockSize = atoi(blockSize);
			free(blockSizeString);

			char **nodeBlockTupleAsArray = string_get_string_as_array(
										nodeBlockTupleAsString);
			if(j == 0){ //es el primer bloque
				currentTuple->firstCopyNodeID = string_from_format("%s",nodeBlockTupleAsArray[0]);
				currentTuple->firstCopyBlockID = atoi(nodeBlockTupleAsArray[1]);
			}else{// es el copia
				currentTuple->secondCopyNodeID = string_from_format("%s",nodeBlockTupleAsArray[0]);
				currentTuple->secondCopyBlockID = atoi(nodeBlockTupleAsArray[1]);
			}

			copy = 1;
			free(blockToSearch);
			config_destroy(fileConfig);

		}


	}

	return output;

}

int fs_getAmountOfBlocksAndCopiesOfAFile(char *file){

	FILE * fileToOpen = fopen(file,"r");
		if(fileToOpen == NULL){
			log_error(logger,"fs_getAmountOfBlocksAndCopiesOfAFile: File %s does not exist",file);
			close(fileToOpen);
			return EXIT_FAILURE;
		}

	close(fileToOpen);

	t_config *fileConfig = config_create(file);

	int totalKeys = config_keys_amount(fileConfig); //Tamanio y tipo de archivo estan siempre y no importan en este caso
	int totalBlockKeys = totalKeys - 2;
	int i = 0;
	int j = 0;
	int copy = 0;
	int totalAmountOfBlocks = 0;



	for(i = 0; i < totalBlockKeys; i++){

			copy = 0;

			for(j = 0; j < 2; j++){

				t_config *fileConfig = config_create(file);//Creo el config
				char *blockToSearch = string_from_format("BLOQUE%dCOPIA%d",
						i, copy);
				char *nodeBlockTupleAsString = config_get_string_value(fileConfig,
										blockToSearch);

				if(nodeBlockTupleAsString == NULL){
					log_error(logger,"fs_getAmountOfBlocksAndCopiesOfAFile: Block tuple: %s not found",blockToSearch);
					config_destroy(fileConfig);
					free(blockToSearch);

					if(copy==1)copy=0;
					if(copy==0)copy=1;
					continue;
				}

				totalAmountOfBlocks++;
				copy = 1;
				free(blockToSearch);
				config_destroy(fileConfig);

			}
	}


	return totalAmountOfBlocks;


}

void fs_dumpBlockTuple(t_fileBlockTuple blockTuple){


	printf("BLOCK:[%d] COPY:[0] NODE:[%s]\n", blockTuple.firstCopyBlockID, blockTuple.firstCopyNodeID);
	printf("BLOCK:[%d] COPY:[1] NODE:[%s]\n", blockTuple.secondCopyBlockID, blockTuple.secondCopyNodeID);
	printf("BLOCK SIZE: %d\n", blockTuple.blockSize);

}

ipc_struct_fs_get_file_info_response *fs_yamaFileBlockTupleResponse(char *file){

	int amountOfBlockTuples = fs_getNumberOfBlocksOfAFile(file);
	ipc_struct_fs_get_file_info_response *response = malloc(sizeof(ipc_struct_fs_get_file_info_response));

	response->entriesCount = amountOfBlockTuples;
	response->entries = fs_getFileBlockTuples(file);

	return response;


}

int fs_amountOfConnectedNodesFromPreviousStatus(){
	int listSize = list_size(connectedNodes);
	int i;
	int output = 0;
	t_dataNode * aux;

	if (listSize == 0){
		log_error(logger,"fs_amountOfPreviousStatus: No DataNodes connected");
		return -1;
	}

	for (i = 0; i < listSize; i++) {
		aux = list_get(connectedNodes, i);
		if(fs_isDataNodeIncludedInPreviouslyConnectedNodes(aux->name)){
			output++;
		}

	}

	if(output == 0){
		log_error(logger,"fs_amountOfPreviousStatus: No dataNode from Previous session");
		return 0;
	}

	log_debug(logger,"fs_amountOfPreviousStatus: Currently %d out of %d connected nodes are from previous session",output,listSize);
	return output;

}

int fs_isDataNodeIncludedInPreviouslyConnectedNodes(char *nodeName){
	int listSize = list_size(previouslyConnectedNodesNames);
	int i;
	int output = 0;
	t_dataNode * aux;

	if (listSize == 0)
		printf("No data nodes connected\n");

	for (i = 0; i < listSize; i++) {
		aux = list_get(previouslyConnectedNodesNames, i);
		if(!strcmp(aux->name, nodeName)) return 1;

	}

	return 0;

}

int fs_deleteFileFromIndex(char *path){
	char* inFileName = myFS.FSFileList;
	char* outFileName = string_from_format("%s.tmp",myFS.FSFileList);
	FILE* inFile = fopen(inFileName, "r");
	FILE* outFile = fopen(outFileName, "w+");
	char line [1024]; // maybe you have to user better value here
	memset(line,0,1024);
	int lineCount = 0;

	if( inFile == NULL )
	{
	    printf("Open Error");
	}

	while( fgets(line, sizeof(line), inFile) != NULL )
	{
		line[strlen(line)-1] = '\0';
	    if(strcmp(line,path))
	    {
	        fprintf(outFile, "%s\n", line);
	    }

	    lineCount++;
	}


	fclose(outFile);

	// possible you have to remove old file here before
	fclose(inFile);
	remove(inFileName);
	if( !rename(outFileName,inFileName ) )
	{
	    log_error(logger,"Rename Error");
	    return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int fs_deleteBlockFromMetadata(char *path,int block, int copy){

	char* inFileName = path;
	char* outFileName = string_from_format("%s.tmp",path);
	FILE* inFile = fopen(inFileName, "r");
	FILE* outFile = fopen(outFileName, "w+");
	char line [1024]; // maybe you have to user better value here
	memset(line,0,1024);
	int lineCount = 0;

	if( inFile == NULL )
	{
	    printf("Open Error");
	}


	char *blockAndCopyString = string_from_format("BLOQUE%dCOPIA%d",block,copy);
	int blockAndCopyStringLength = strlen(blockAndCopyString);
	char *auxiliaryString;

	while( fgets(line, sizeof(line), inFile) != NULL )
	{
		auxiliaryString = strdup(line);
		if(strlen(auxiliaryString)>blockAndCopyStringLength+1){
			auxiliaryString[blockAndCopyStringLength] = '\0';
		}
	    if(strcmp(blockAndCopyString,auxiliaryString))
	    {
	        fprintf(outFile, "%s", line);
	    }

	    lineCount++;
		free(auxiliaryString);
	}


	fclose(outFile);

	// possible you have to remove old file here before
	fclose(inFile);
	remove(inFileName);
	if( !rename(outFileName,inFileName ) )
	{
	    log_error(logger,"Rename Error");
	    return EXIT_FAILURE;
	}

	free(blockAndCopyString);

	return EXIT_SUCCESS;
}

int fs_getNumberOfBlocksOfAFile(char *file){

	FILE * fileToOpen = fopen(file,"r");
		if(fileToOpen == NULL){
			log_error(logger,"fs_getNumberOfBlocksOfAFile: File %s does not exist",file);
			close(fileToOpen);
			return EXIT_FAILURE;
		}

	close(fileToOpen);

	t_config *fileConfig = config_create(file);

	int totalKeys = config_keys_amount(fileConfig); //Tamanio y tipo de archivo estan siempre y no importan en este caso
	int totalBlockKeys = totalKeys - 2;
	int i = 0;
	int j = 0;
	int copy = 0;
	int totalAmountOfBlocks = 0;


	for(i = 0; i < totalBlockKeys; i++){

			copy = 0;

			for(j = 0; j < 2; j++){

				t_config *fileConfig = config_create(file);//Creo el config
				char *blockToSearch = string_from_format("BLOQUE%dCOPIA%d",
						i, copy);
				char *nodeBlockTupleAsString = config_get_string_value(fileConfig,
										blockToSearch);

				if(nodeBlockTupleAsString == NULL){
					log_error(logger,"fs_getNumberOfBlocksOfAFile: Block tuple: %s not found",blockToSearch);
					config_destroy(fileConfig);
					free(blockToSearch);

					if(copy==1)copy=0;
					if(copy==0)copy=1;
					continue;
				}

				totalAmountOfBlocks++;
				copy = 1;
				free(blockToSearch);
				config_destroy(fileConfig);
				break;

			}
	}


	return totalAmountOfBlocks;




}

void fs_freeTuple(ipc_struct_fs_get_file_info_response_entry *tuple){


	return;

}

void fs_destroyNodeTupleArray(ipc_struct_fs_get_file_info_response_entry *tuple, int arrayLength){

	int i = 0;

	for(i = 0; i < arrayLength ; i++){
		log_info(logger,"fs_FreeTuple: Freeing tuple: [%s - %d] | [%s  - %d]",tuple[i].firstCopyNodeID, tuple[i].firstCopyBlockID, tuple[i].secondCopyNodeID, tuple[i].secondCopyBlockID);
		free(tuple[i].firstCopyNodeID);
		free(tuple[i].secondCopyNodeID);
	}

	free(tuple);


	return;

}

int fs_updateFileFromIndex(char *old, char *new){
	fs_deleteFileFromIndex(old);
	char *newName = string_from_format("%s\n",new);
	FILE *fileIndex = fopen(myFS.FSFileList,"a+");
	fseek(fileIndex,0,SEEK_END);
	fwrite(newName,strlen(newName),1,fileIndex);
	free(newName);
	fclose(fileIndex);
	return EXIT_SUCCESS;
}

char *fs_isAFile(char *path){
	char **splitPath = string_split(path,"/");
	int splitPathElementCount = fs_amountOfElementsInArray(splitPath);
	int fileNameLength = strlen(splitPath[splitPathElementCount-1]);

	char *parentPath = strdup(path);
	parentPath[strlen(parentPath)-fileNameLength-1] = 0;

	//check parent path exists and get parent dir
	t_directory *parent = fs_directoryExists(parentPath);
	if(!parent){
		log_error(logger,"parent directory doesnt exist");
		return EXIT_FAILURE;
	}

	//check file exists
	//transform path to physical path
	char *physicalPath = string_from_format("%s/%d/%s",myFS.filesDirectoryPath,parent->index,splitPath[splitPathElementCount-1]);
	FILE *fileMetadata = fopen(physicalPath,"r+");
	if(!fileMetadata){
		log_error(logger,"physical file doesnt exist");
		return NULL;
	}
	fclose(fileMetadata);
	int iterator = 0;
	while(iterator < splitPathElementCount){
		free(splitPath[iterator]);
		iterator++;
	}
	free(parentPath);
	return physicalPath;
}

char *fs_isFileContainedBy(char *filePhysicalPath, t_directory *parent){
	char **splitPath = string_split(filePhysicalPath,"/");
	int splitPathElementCount = fs_amountOfElementsInArray(splitPath);
	int parentIndex = strlen(splitPath[splitPathElementCount-2]);
	int returnValue = 0;

	if(parentIndex == parent->index){
		returnValue++;
	}

	int iterator = 0;
	while(iterator < splitPathElementCount){
		free(splitPath[iterator]);
		iterator++;
	}

	return returnValue;
}

int *fs_moveFileTo(char *filePhysicalPath, t_directory *parent){
	char **splitPath = string_split(filePhysicalPath,"/");
	int splitPathElementCount = fs_amountOfElementsInArray(splitPath);

	char *newFileName = string_from_format("%s/%d/%s",myFS.filesDirectoryPath,parent->index,basename(filePhysicalPath));
	char *parentPath = string_from_format("%s/%d",myFS.filesDirectoryPath,parent->index,basename(filePhysicalPath));

	fs_openOrCreateDirectory(parentPath,0);
	fs_updateFileFromIndex(filePhysicalPath,newFileName);

	char *command = string_from_format("mv %s %s",filePhysicalPath,newFileName);

	system(command);

	int iterator = 0;
	while(iterator < splitPathElementCount){
		free(splitPath[iterator]);
		iterator++;
	}
	free(newFileName);
	free(parentPath);
	free(command);

	return 1;

}

int fs_updateAllConnectedNodesOnTable(){

	int listSize = list_size(connectedNodes);
	int i;
	t_dataNode * aux;
	if (listSize == 0){
		log_error(logger,"fs_updateAllConnectedNodes: No data nodes connected\n");
		return EXIT_FAILURE;

	}

	for (i = 0; i < listSize; i++) {
		aux = list_get(connectedNodes, i);
		fs_updateNodeTable(*aux);

	}

	return EXIT_SUCCESS;

}

int fs_wipeAllConnectedDataNodes(){

	int listSize = list_size(connectedNodes);
		int i;
		t_dataNode * aux;
		if (listSize == 0){
			log_error(logger,"fs_wipeAllConnectedNodes: No data nodes connected\n");
			return EXIT_FAILURE;

		}
		int result;
		for (i = 0; i < listSize; i++) {
			aux = list_get(connectedNodes, i);
			result = fs_wipeDataNode(aux);
			if(result == EXIT_FAILURE){
				log_error(logger,"fs_wipeAllConnectedNodes: Could not wipe all connected nodes\n");
				return EXIT_FAILURE;
			}

		}

		log_info(logger,"fs_wipeAllConnectedNodes: All connected nodes have been wiped successfully");

		return EXIT_SUCCESS;

}

int fs_wipeDataNode(t_dataNode *aDataNode){


	struct stat mystat;

	if (fstat(aDataNode->bitmapFile->_fileno, &mystat) < 0) {
			log_error(logger,
					"fs_mmapDataNodeBitmap: Error at fstat of data node %s in order to map to memory",
					aDataNode->name);
			return EXIT_FAILURE;

	}

	if(aDataNode == NULL){
		log_error(logger,"fs_wipeDataNode: Node %s isnt valid- wipe aborted", aDataNode->name);
		return EXIT_FAILURE;
	}
	int iterator = 0;

	while(iterator < aDataNode->amountOfBlocks){

		bitarray_clean_bit(aDataNode->bitmap, iterator);
			aDataNode->occupiedBlocks--;

			iterator++;
	}

	aDataNode->freeBlocks = aDataNode->amountOfBlocks;
	aDataNode->occupiedBlocks = 0;

	int munmapResult = munmap(aDataNode->bitmapMapedPointer, mystat.st_size);

	if(munmapResult == -1){
		log_error(logger,"fs_wipeDataNode: error on munmap");
		return EXIT_FAILURE;
	}

	log_debug(logger,"fs_wipeDataNode: munmapped bitmap of DataNode %s successfully", aDataNode->name);

	log_debug(logger,"fs_wipeDataNode: Node: %s wiped successfully", aDataNode->name);
	return EXIT_SUCCESS;

}

int fs_wipeDirectoryTable(){

	log_debug(logger,"fs_wipeDirectoryTable: Wiping directory table");
	int i = 1;//Arranca de uno porque el root siempre queda

	for(i = 1; i < DIRECTORY_TABLE_MAX_AMOUNT ; i++){

		myFS.directoryTable[i].index = 0;
		myFS.directoryTable[i].name[0] = '\0';
		myFS.directoryTable[i].parent = 0;

	}


	return EXIT_SUCCESS;
}

int fs_directoryStartsWithSlash(char *directory){

	if(directory[0] == '/'){
		log_debug(logger,"fs_directoryStartsWithFlash: TRUE");
		return TRUE;
	}
	log_error(logger,"fs_directoryStartsWithFlash: FALSE");
	return FALSE;

}

t_dataNode *fs_pickNodeToSendRead(t_dataNode *first, t_dataNode *copy){
	if(first == NULL && copy != NULL){
		return copy;
	}else{
		if(copy == NULL && first != NULL){
			return first;
		}else{
			if(copy == NULL && first == NULL){
				return NULL;
			}
		}
	}
	return (queue_size(first->operationsQueue) > queue_size(copy->operationsQueue)) ? copy : first;
}

void *fs_readFile(char *filePath){
	char *physicalPath = fs_isAFile(filePath);

	ipc_struct_fs_get_file_info_response_entry *blockArray = fs_getFileBlockTuples(physicalPath);

	int amountOfBlocks = fs_getNumberOfBlocksOfAFile(physicalPath);

	int iterator = 0;
	t_dataNode *first = NULL;
	t_dataNode *copy = NULL;
	t_dataNode *target = NULL;

	t_threadOperation *operation;
	t_dataNode **readOrder = malloc(sizeof(t_dataNode*)*amountOfBlocks);

	while(iterator<amountOfBlocks){
		if(blockArray[iterator].firstCopyNodeID != NULL){
			first = fs_getNodeFromNodeName(blockArray[iterator].firstCopyNodeID);
		}else{
			first = NULL;
		}
		if(blockArray[iterator].secondCopyNodeID != NULL){
			copy = fs_getNodeFromNodeName(blockArray[iterator].secondCopyNodeID);
		}else{
			copy = NULL;
		}

		target = fs_pickNodeToSendRead(first,copy);

		if(target == NULL){
			log_error(logger,"no nodes to send read operation");
			return NULL;
		}

		operation = malloc(sizeof(t_threadOperation));
		operation->operationId = 0; // reada
		if(target == first){
			operation->blockNumber = blockArray[iterator].firstCopyBlockID;
		}else{
			operation->blockNumber = blockArray[iterator].secondCopyBlockID;
		}

		queue_push(target->operationsQueue,operation);
		sem_post(target->threadSemaphore);
		readOrder[iterator] = target;


		iterator++;
	}

	//borrar a la mierda
	void *buffer = BLOCK_SIZE; // arre
	void *result = malloc(BLOCK_SIZE * amountOfBlocks);
	memset(result,0,BLOCK_SIZE * amountOfBlocks);

	// termine de mandar los pedidos ahora a leer
	iterator = 0;
	int offset = 0;
	while(iterator < amountOfBlocks){
		target = readOrder[iterator];

		sem_wait(target->resultSemaphore);
		void *buffer = queue_pop(target->resultsQueue);

		memcpy(result+offset,buffer,BLOCK_SIZE);
		free(buffer);
		offset+= BLOCK_SIZE;
		iterator++;
	}
	//log_info(logger,"file: %s",result);
	return result;
}

int fs_createBitmapsOfAllConnectedNodes(){

	int listSize = list_size(connectedNodes);
			int i;
			t_dataNode * aux;
			if (listSize == 0){
				log_error(logger,"fs_createBitmapsOfAllConnectedNodes: No data nodes connected\n");
				return EXIT_FAILURE;

			}
			int result;
			for (i = 0; i < listSize; i++) {
				aux = list_get(connectedNodes, i);

				result = fs_openOrCreateBitmap(myFS, aux);

				if(result == EXIT_FAILURE){
					log_error(logger,"fs_createBitmapsOfAllConnectedNodes: Could not wipe all connected nodes\n");
					return EXIT_FAILURE;
				}

			}

			log_info(logger,"fs_wipeAllConnectedNodes: All connected nodes have been wiped successfully");

			return EXIT_SUCCESS;
}
int fs_getFileSize(char *filePath){
	t_config *fileMetadata = config_create(filePath);
	char *stringSize = config_get_string_value(fileMetadata,"TAMANIO");
	int fileSize = atoi(stringSize);
	config_destroy(fileMetadata);
	free(stringSize);

	return fileSize;
}
