/*TODO: Levantar info de bloques libres del bitmap
 TODO: Setear bien cantidad de nodos libres

 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "datanode.h"
#include <commons/log.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include<commons/config.h>
#include <sys/mman.h> //mmap
#include <fcntl.h> // O_RDRW

t_log *logger;
t_config *config;
t_dataNode myDataNode;
t_dataNodeConfig nodeConfig;

int dataNode_mmapDataBin(char* dataBinPath);

int main(int argc, char **argv) {
	logger = log_create("log.txt", "DataNode", 1, LOG_LEVEL_DEBUG);

	//Validar parametros de entrada
	if (argc < 2) {
		log_error(logger, "Falta pasar la ruta del archivo de configuracion");
		return EXIT_FAILURE;
	} else {
		//Si estan bien, cargo el archivo de configuracion
		config = config_create(argv[1]);
		dataNode_loadConfig(&myDataNode);
	}

	//Abro el archivo data.bin y si no lo creo con el tamanio pasado por config
	dataNode_openOrCreateDataBinFile(myDataNode.config.databinPath, myDataNode.config.sizeInMb);

	//FILE * dataBinFile = fopen(myDataNode.config.databinPath, "w+");


	dataNode_setBlockInformation(&myDataNode);



	dataNode_connectToFileSystem(myDataNode);

	return EXIT_SUCCESS;
}

int dataNode_loadConfig(t_dataNode *dataNode) {

	dataNode->config.DataNodePortno = config_get_int_value(config,
			"PUERTO_DATANODE");
	dataNode->config.workerPortno = config_get_int_value(config,
			"PUERTO_WORKER");
	dataNode->config.fsPortno = config_get_int_value(config,
			"PUERTO_FILESYSTEM");
	dataNode->config.fsIP = config_get_string_value(config, "IP_FILESYSTEM");
	dataNode->config.nodeName = config_get_string_value(config, "NOMBRE_NODO");
	dataNode->config.databinPath = config_get_string_value(config,
			"RUTA_DATABIN");
	dataNode->config.sizeInMb = config_get_int_value(config, "TAMANO_DATABIN");

	return 0;

}

int dataNode_openOrCreateDataBinFile(char *dataBinPath, int sizeInMb) {

	FILE * dataBinFileDescriptor;
	if ((dataBinFileDescriptor = fopen(dataBinPath, "r+"))) { //Existe el archivo de data.bin
		log_debug(logger, "Data.bin file  found. Wont create from scratch");
		log_debug(logger, "Maping Data.bin to memory");

		dataNode_mmapDataBin(dataBinPath);

		return EXIT_FAILURE;

	} else { //No puede abrirlo => Lo crea
		printf("NoExiste el data.bin");

		log_debug(logger,
				"Data.bin file not found. Creating with parameters of config file");
		dataBinFileDescriptor = fopen(dataBinPath, "w+");
		dataNode_writeNBytesOfXToFile(dataBinFileDescriptor,
				sizeInMb * BLOCK_SIZE, 0);
		//ftruncate(fileno(dataBinFileDescriptor), sizeInMb * BLOCK_SIZE);

		log_debug(logger, "Maping Data.bin to memory");

		dataNode_mmapDataBin(dataBinPath);
		return EXIT_SUCCESS;
	}
}

void dataNode_connectToFileSystem(t_dataNode dataNode) {

	//Declare variables used in function
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	printf("\nIP FS: %s Portno: %d\n", dataNode.config.fsIP, dataNode.config.fsPortno);
	printf("\nConnecting to FS\n");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	server = gethostbyname(dataNode.config.fsIP);
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(dataNode.config.fsPortno);

	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		exit(1);
	}

	char buffer[1024] = { 0 };

	//Send block name
	send(sockfd, dataNode.config.nodeName, strlen(dataNode.config.nodeName), 0);
	read(sockfd, buffer, 1024);

	//Send amount of blocks
	int myInt = dataNode.blockInfo.amountOfBlocks;
	int tmp = htonl((uint32_t) myInt);

	write(sockfd, &tmp, sizeof(tmp));

	int sendPortno = dataNode.config.workerPortno;
	tmp = htonl((uint32_t) sendPortno);
	write(sockfd, &tmp, sizeof(tmp));



	//wait for request from fs
	uint32_t operationType;
	uint32_t blockNumber;
	void *operationBuffer;
	while (1) {

		//Codigo para escuchar pedidos del fs aca;lk;lk
		//leo operation type
		//read(sockfd, &operationType, sizeof(uint32_t));
		recv(sockfd,&operationType,sizeof(uint32_t),MSG_WAITALL);
		int sending;
		if(operationType == 1){
			//write
			//leo size a escribir
			//read(sockfd, &size, sizeof(uint32_t));
			//recv(sockfd, &size, sizeof(uint32_t),MSG_WAITALL);
			//leo block a escribir
			//read(sockfd, &blockNumber, sizeof(uint32_t));
			recv(sockfd, &blockNumber, sizeof(uint32_t),MSG_WAITALL);
			//leo buffer
			operationBuffer = malloc(BLOCK_SIZE);
			memset(operationBuffer,0,BLOCK_SIZE);
			//read(sockfd, operationBuffer, size);
			recv(sockfd, operationBuffer, BLOCK_SIZE,MSG_WAITALL);
			dataNode_setBlock(blockNumber,operationBuffer);
		}else if(operationType == 0){
			recv(sockfd, &blockNumber, sizeof(uint32_t),MSG_WAITALL);
			void *blockRead = malloc(BLOCK_SIZE);
			blockRead = dataNode_getBlock(blockNumber);
			send(sockfd, blockRead, BLOCK_SIZE, 0);
			free(blockRead);
		}else if(operationType == 2){
			sending = 1;
			send(sockfd,&sending,sizeof(int),0);
		}

	}

}

void dataNode_setBlockInformation(t_dataNode *aDataNode) {

	//TODO: Levantar info de bloques libres del bitmap
	aDataNode->blockInfo.amountOfBlocks = aDataNode->config.sizeInMb;


}

void *dataNode_getBlock(int blockNumber) {

	void *blockInformation = malloc(BLOCK_SIZE);
	memset(blockInformation, 0, BLOCK_SIZE);

	int positionInBytesOfTheBlock = blockNumber * BLOCK_SIZE;

	memcpy(blockInformation,
			myDataNode.dataBinMMapedPointer + positionInBytesOfTheBlock,
			BLOCK_SIZE);

	return blockInformation;

}

int dataNode_setBlock(int blockNumber, void *dataToWrite) {

	int positionInBytesOfTheBlock = blockNumber * BLOCK_SIZE;

	memcpy(myDataNode.dataBinMMapedPointer + positionInBytesOfTheBlock,
			dataToWrite, BLOCK_SIZE);

	return EXIT_SUCCESS;

}

int dataNode_mmapDataBin(char* dataBinPath) {

	struct stat mystat;

	myDataNode.dataBinFileDescriptor = open(dataBinPath, O_RDWR);

	if (myDataNode.dataBinFileDescriptor == -1) {
		log_error(logger, "Error opening Data.bin in order to map to memory");
		return EXIT_FAILURE;
	}

	if (fstat(myDataNode.dataBinFileDescriptor, &mystat) < 0) {
		log_error(logger,
				"Error at fstat of Data.bin in order to map to memory");
		return EXIT_FAILURE;

	}

	myDataNode.dataBinMMapedPointer = mmap(0, mystat.st_size,
	PROT_READ | PROT_WRITE, MAP_SHARED, myDataNode.dataBinFileDescriptor, 0);

	if (myDataNode.dataBinMMapedPointer == MAP_FAILED) {
		log_error(logger, "Error creating mmap pointer to Data.bin file");
		return EXIT_FAILURE;

	}

	return EXIT_SUCCESS;
}

int dataNode_writeNBytesOfXToFile(FILE *fileDescriptor, int N, int C) { //El tamanio del archivo antes del mmap matchea con el tamanio del del archivo
	char *buffer = malloc(N);
	memset(buffer, C, N);
	fwrite(buffer, N, 1, fileDescriptor);
	fflush(fileDescriptor);
	return EXIT_SUCCESS;
}

void dataNode_dumpDataBin() {

}

