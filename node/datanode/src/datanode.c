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

	FILE * dataBinFile = fopen(myDataNode.config.databinPath,"w+");

	dataNode_setBlockInformation(&myDataNode);
	char *test = "ESCRIBIESTOCONLAFUNCION";
	dataNode_setBlock(0,test);
	void *prueba = malloc(BLOCK_SIZE);
	memset(prueba, 0, BLOCK_SIZE);
	prueba = dataNode_getBlock(0);

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
		return EXIT_FAILURE;

	} else { //No puede abrirlo => Lo crea
		printf("NoExiste el data.bin");

		log_debug(logger,
				"Data.bin file not found. Creating with parameters of config file");
		dataBinFileDescriptor = fopen(dataBinPath, "w+");
		ftruncate(fileno(dataBinFileDescriptor), sizeInMb * BLOCK_SIZE);
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
	aDataNode->blockInfo.freeBlocks = 5;
	aDataNode->blockInfo.occupiedBlocks = aDataNode->blockInfo.amountOfBlocks
			- aDataNode->blockInfo.freeBlocks;

}

void *dataNode_getBlock(int blockNumber) {

	FILE * dataBinFileDescriptor;
	int result = 0;
	void *blockInformation = malloc(BLOCK_SIZE);
	memset(blockInformation, 0, BLOCK_SIZE);

	dataBinFileDescriptor = fopen(myDataNode.config.databinPath, "r+"); //Abro el archivo

	int positionInBytesOfTheBlock = blockNumber * BLOCK_SIZE;

	//Me posiciono en el bloque correspondiente partiendo desde el principio
	result = fseek(dataBinFileDescriptor, positionInBytesOfTheBlock, SEEK_SET);

	if(result == -1){
			log_error(logger, "Error al intentar posicionar el bloque %d del databin",blockNumber);
			return NULL;
		}

	//Leo 1 BLOCK_SIZE (1024*1024)
	result = fread(blockInformation, 1, BLOCK_SIZE, dataBinFileDescriptor);

	if(result == 0){
		log_error(logger, "Error al intentar leer bloque %d",blockNumber);
		return NULL;
	}

	fclose(dataBinFileDescriptor);

	return blockInformation;

}

int dataNode_setBlock(int blockNumber, void *dataToWrite) {

	FILE * dataBinFileDescriptor;
	int result = 0;

	//Abro el archivo
	dataBinFileDescriptor = fopen(myDataNode.config.databinPath, "w+");

	int positionInBytesOfTheBlock = blockNumber * BLOCK_SIZE;

	//Me posiciono en el bloque correspondiente partiendo desde el principio
	result = fseek(dataBinFileDescriptor, positionInBytesOfTheBlock, SEEK_SET);

	if(result == -1){
			log_error(logger, "Error al intentar posicionar el bloque %d del databin",blockNumber);
			return EXIT_FAILURE;
		}

	//Leo 1 BLOCK_SIZE (1024*1024)
	result = fwrite(dataToWrite, 1, BLOCK_SIZE, dataBinFileDescriptor);

	if(result == 0){
		log_error(logger, "Error al intentar leer bloque %d",blockNumber);
		return EXIT_FAILURE;
	}

	fclose(dataBinFileDescriptor);

	return EXIT_SUCCESS;

}
