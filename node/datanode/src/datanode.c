/*TODO: Levantar info de bloques libres del bitmap
 TODO: Setear bien cantidad de nodos libres

 */
#include <stdio.h>
#include <stdlib.h>
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

int main(int argc, char **argv) {

	char *logFile = tmpnam(NULL);

	logger = log_create(logFile, "DataNode", 1, LOG_LEVEL_DEBUG);

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
	int dataBinResult = dataNode_openOrCreateDataBinFile(
			myDataNode.config.databinPath, myDataNode.config.sizeInMb);

	//FILE * dataBinFile = fopen(myDataNode.config.databinPath, "w+");


	dataNode_setBlockInformation(&myDataNode);

	char *bitarray = malloc(myDataNode.blockInfo.amountOfBlocks/8);
	memset(bitarray,0,myDataNode.blockInfo.amountOfBlocks/8);

	myDataNode.bitmap = bitarray_create_with_mode(bitarray,myDataNode.blockInfo.amountOfBlocks/8, LSB_FIRST);

	char * test = "test";
	void *prueba = malloc(BLOCK_SIZE);
	memset(prueba, 0, BLOCK_SIZE);
	memcpy(prueba, test, strlen(test));
	dataNode_setBlock(0, prueba);
	prueba = dataNode_getBlock(0);

	memset(prueba, 0, BLOCK_SIZE);
	char * test2 = "estoy en el tercer bloque";
	memcpy(prueba, test2, strlen(test2));
	dataNode_setBlock(3, prueba);
	prueba = dataNode_getBlock(3);


	dataNode_setBitmapInformation();

	dataNode_dumpBitmap();

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
	if (dataBinFileDescriptor = fopen(dataBinPath, "r+")) { //Existe el archivo de data.bin
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

	return NULL;
}

void dataNode_connectToFileSystem(t_dataNode dataNode) {

	//Declare variables used in function
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int programLength;

	printf("\IP FS: %s Portno: %d\n", dataNode.config.fsIP,
			dataNode.config.fsPortno);
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

	//Send amount of free blocks
	myInt = dataNode.blockInfo.freeBlocks;
	tmp = htonl((uint32_t) myInt);
	write(sockfd, &tmp, sizeof(tmp));

	//Send amount of occupied blocks
	myInt = dataNode.blockInfo.occupiedBlocks;
	tmp = htonl((uint32_t) myInt);
	write(sockfd, &tmp, sizeof(tmp));

	printf("%s\n", buffer);

	//wait for request from fs

	while (1) {

		//Codigo para escuchar pedidos del fs aca;lk;lk
		//printf("DataNode %s esperando pedidos del FS\n",dataNode.config.nodeName);
		sleep(5);

	}

}

void dataNode_setBlockInformation(t_dataNode *aDataNode) {

	//TODO: Levantar info de bloques libres del bitmap
	aDataNode->blockInfo.amountOfBlocks = aDataNode->config.sizeInMb;
	aDataNode->blockInfo.freeBlocks = 11;
	aDataNode->blockInfo.occupiedBlocks = aDataNode->blockInfo.amountOfBlocks
			- aDataNode->blockInfo.freeBlocks;

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
	return EXIT_SUCCESS;
}

void dataNode_dumpDataBin() {

}

int dataNode_setBitmapInformation() {

	int i;
	char *aux = malloc(BLOCK_SIZE);
	char testblock [BLOCK_SIZE];
	memset(testblock, 0, sizeof(testblock));

	//Comienzo a recorrer el dataBin que ya esta mmapeado
	for (i = 0; i < myDataNode.blockInfo.amountOfBlocks; i++) {

		aux = dataNode_getBlock(i);
		//Si adentro no tiene nada
		if (!memcmp(testblock,aux,BLOCK_SIZE)) {
			//CLEAN BIT
			bitarray_clean_bit(myDataNode.bitmap,i);
		} else {
			//SET BIT
			bitarray_set_bit(myDataNode.bitmap,i);

		}

		free(aux);

	}

}

void dataNode_dumpBitmap() {
	int i = 0;
	int unBit = 0;

	while (i < myDataNode.blockInfo.amountOfBlocks) {
		unBit = bitarray_test_bit(myDataNode.bitmap, i);
		log_info(logger,"bit %d: %d",i,unBit);
		i++;
	}
}
