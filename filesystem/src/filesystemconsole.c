/**
 TODO: Validar existencia file paths en cada funcion del fs que los utilice
 TODO: Liberar memoria dentro de validateOp
 TODO: Controlar que solo se conecte un YAMA
 TODO: Controlar que se conecte un YAMA solo despues que se conecte un DataNode
 TODO: Validar estado seguros
 TODO: Levantar archivo de configuracion
 **/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include "filesystem.h"
#include <netinet/in.h>
#include <pthread.h>

char* fs_console_getOpFromInput(char *userInput);
void fs_console();
int fs_console_validateOp(char * newLine);
char *fs_console_getNParameterFromUserInput(int paramNumber, char *userInput);

int fs_console_operationEndedSuccessfully(int);

void main() {

	fs_dataNodeConnectionThread();

	while(!fs_isStable()){

		printf("FS not stable. Cant connect to YAMA");

	}

	//fs_yamaConnectionThread();

	fs_console();
}

void fs_console() {

	char *userInput;
	int operationResult;

	while (1) {

		userInput = readline(">");

		//fs_console_waitForDataNodes();
		operationResult = fs_console_validateOp(userInput);

		if (operationResult == NULL)
			break;

		free(userInput);

	}
}

int fs_console_validateOp(char * newLine) {

	char *operation = malloc(25);
	memset(operation, 0, 25);
	operation = fs_console_getOpFromInput(newLine);

	char *firstParameter = malloc(255);
	memset(firstParameter, 0, 255);

	char *secondParameter = malloc(255);
	memset(secondParameter, 0, 255);

	char *thirdParameter = malloc(255);
	memset(thirdParameter, 0, 255);

	int opResult; //0 if success -1 if fails

	/************ format **************/
	if (strcmp(operation, "format") == 0) {
		fs_format();

	}

	/************ rm **************/
	if (!strcmp(operation, "rm")) {
		firstParameter = fs_console_getNParameterFromUserInput(1, newLine);

		//-d
		if (!strcmp(firstParameter, "-d")) {
			secondParameter = fs_console_getNParameterFromUserInput(2, newLine);
			opResult = fs_rm_dir(secondParameter);
			if (!fs_console_operationEndedSuccessfully(opResult))
				printf("remove operation failed\n");
		}

		//-b
		if (!strcmp(firstParameter, "-b")) {
			char *fourthParameter = malloc(255);
			memset(fourthParameter, 0, 255);
			secondParameter = fs_console_getNParameterFromUserInput(2, newLine);
			thirdParameter = fs_console_getNParameterFromUserInput(3, newLine);
			fourthParameter = fs_console_getNParameterFromUserInput(4, newLine);

			opResult = fs_rm_block(secondParameter, atoi(thirdParameter),
					atoi(fourthParameter));

			if (!fs_console_operationEndedSuccessfully(opResult))
				printf("remove operation failed\n");

		} else {
			//If not -b or -d
			opResult = fs_rm(firstParameter);
			if (!fs_console_operationEndedSuccessfully(opResult))
				printf("remove operation failed\n");

		}

	}

	if (!strcmp(operation, "rename")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getNParameterFromUserInput(1, newLine);
		secondParameter = fs_console_getNParameterFromUserInput(2, newLine);
		opResult = fs_rename(firstParameter, secondParameter);

		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("rename operation failed\n");

	}

	if (!strcmp(operation, "mv")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getNParameterFromUserInput(1, newLine);
		secondParameter = fs_console_getNParameterFromUserInput(2, newLine);

		opResult = fs_mv(firstParameter, secondParameter);

		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mv operation failed\n");
	}

	if (!strcmp(operation, "cat")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getNParameterFromUserInput(1, newLine);
		opResult = fs_cat(firstParameter);

		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mv operation failed\n");

	}

	if (!strcmp(operation, "mkdir")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getNParameterFromUserInput(1, newLine);
		opResult = fs_mkdir(firstParameter);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mkdir operation failed\n");
	}
	if (!strcmp(operation, "cpfrom")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getNParameterFromUserInput(1, newLine);
		secondParameter = fs_console_getNParameterFromUserInput(2, newLine);

		opResult = fs_cpfrom(firstParameter, secondParameter);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mkdir operation failed\n");

	}

	if (!strcmp(operation, "cpto")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getNParameterFromUserInput(1, newLine);
		secondParameter = fs_console_getNParameterFromUserInput(2, newLine);
		opResult = fs_cpfrom(firstParameter, secondParameter);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("cpto operation failed\n");

	}

	if (!strcmp(operation, "cpblock")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getNParameterFromUserInput(1, newLine);
		secondParameter = fs_console_getNParameterFromUserInput(2, newLine);
		thirdParameter = fs_console_getNParameterFromUserInput(3, newLine);
		opResult = fs_cpblock(firstParameter, atoi(secondParameter),
				atoi(thirdParameter));
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("cpblock operation failed\n");

	}

	if (!strcmp(operation, "md5")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getNParameterFromUserInput(1, newLine);
		opResult = fs_md5(firstParameter);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("md5 operation failed\n");
	}

	if (!strcmp(operation, "ls")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getNParameterFromUserInput(1, newLine);
		opResult = fs_ls(firstParameter);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("ls operation failed\n");
	}

	if (!strcmp(operation, "info")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getNParameterFromUserInput(1, newLine);
		opResult = fs_info(firstParameter);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("info operation failed\n");

	}

	if (!strncmp(operation, "exit", 4)) {
		/*
		 free(firstParameter);
		 free(secondParameter);
		 free(thirdParameter);*/

		return NULL;
	}

	/*free(firstParameter);
	 free(secondParameter);
	 free(thirdParameter);*/

	return 1;

}

char* fs_console_getOpFromInput(char *userInput) {

	char * userInputCopy = malloc(strlen(userInput) + 1);
	strcpy(userInputCopy, userInput);
	char *output = strtok(userInputCopy, " ");

	return output;

}

char *fs_console_getNParameterFromUserInput(int paramNumber, char *userInput) {

	char * userInputCopy = malloc(strlen(userInput) + 1);
	strcpy(userInputCopy, userInput);
	char * output = strtok(userInputCopy, " ");
	int iterator = 0;

	for (iterator = 0; iterator < paramNumber; iterator++) {
		output = strtok(NULL, " \n");
	}

	return output;

}

int fs_console_operationEndedSuccessfully(int operationResult) {

	if (operationResult == 0)
		return 1;
	return 0;
}

