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
#include <readline/history.h>

#include "filesystem.h"
#include <netinet/in.h>
#include <pthread.h>
#include <commons/string.h>


char* fs_console_getOpFromInput(char *userInput);
void fs_console_launch();
int fs_console_validateOp(char * newLine);
char *fs_console_getNParameterFromUserInput(int paramNumber, char *userInput);

int fs_console_operationEndedSuccessfully(int);



void fs_console_launch() {

	char *userInput;
	int operationResult;

	while (1) {

		userInput = readline(">");

	      if(userInput)
	        add_history(userInput);

		//fs_console_waitForDataNodes();
		operationResult = fs_console_validateOp(userInput);

		if (operationResult == NULL)
			break;

		free(userInput);

	}
}

int fs_console_validateOp(char * newLine) {

    char** parameter_list = string_split(newLine," ");
    int sizeof_parameter_list = cantidadDe(parameter_list);

	char *operation = malloc(sizeof(parameter_list[0]));
	memset(operation, 0, sizeof(parameter_list[0]));
	strcpy(operation, parameter_list[0]);
	//operation = fs_console_getOpFromInput(newLine);

	int opResult; //0 if success -1 if fails

	/************ format **************/
	if (strcmp(operation, "format") == 0) {
		fs_format();

	}

	/************ rm **************/
	if (!strcmp(operation, "rm")) {

		//-d
		if (!strcmp(parameter_list[1], "-d")) {
			opResult = fs_rm_dir(parameter_list[2]);
			if (!fs_console_operationEndedSuccessfully(opResult))
				printf("remove operation failed\n");
		}

		//-b
		if (!strcmp(parameter_list[1], "-b")) {

			opResult = fs_rm_block(parameter_list[2], atoi(parameter_list[3]),
					atoi(parameter_list[4]));

			if (!fs_console_operationEndedSuccessfully(opResult))
				printf("remove operation failed\n");

		} else {
			//If not -b or -d
			opResult = fs_rm(parameter_list[1]);
			if (!fs_console_operationEndedSuccessfully(opResult))
				printf("remove operation failed\n");

		}

	}

	if (!strcmp(operation, "rename")) {
		printf("OPERATION: %s\n", operation);

		opResult = fs_rename(parameter_list[1], parameter_list[2]);

		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("rename operation failed\n");

	}

	if (!strcmp(operation, "mv")) {
		printf("OPERATION: %s\n", operation);

		opResult = fs_mv(parameter_list[1], parameter_list[2]);

		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mv operation failed\n");
	}

	if (!strcmp(operation, "cat")) {
		printf("OPERATION: %s\n", operation);
		opResult = fs_cat(parameter_list[1]);

		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mv operation failed\n");

	}

	if (!strcmp(operation, "mkdir")) {
		printf("OPERATION: %s\n", operation);
		opResult = fs_mkdir(parameter_list[1]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mkdir operation failed\n");
	}
	if (!strcmp(operation, "cpfrom")) {
		printf("OPERATION: %s\n", operation);

		opResult = fs_cpfrom(parameter_list[1], parameter_list[2]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mkdir operation failed\n");

	}

	if (!strcmp(operation, "cpto")) {
		printf("OPERATION: %s\n", operation);

		opResult = fs_cpfrom(parameter_list[1], parameter_list[2]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("cpto operation failed\n");

	}

	if (!strcmp(operation, "cpblock")) {
		printf("OPERATION: %s\n", operation);

		opResult = fs_cpblock(parameter_list[1], atoi(parameter_list[2]),
				atoi(parameter_list[3]));
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("cpblock operation failed\n");

	}

	if (!strcmp(operation, "md5")) {
		printf("OPERATION: %s\n", operation);
		opResult = fs_md5(parameter_list[1]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("md5 operation failed\n");
	}

	if (!strcmp(operation, "ls")) {
		printf("OPERATION: %s\n", operation);
		opResult = fs_ls(parameter_list[1]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("ls operation failed\n");
	}

	if (!strcmp(operation, "info")) {
		printf("OPERATION: %s\n", operation);
		opResult = fs_info(parameter_list[1]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("info operation failed\n");

	}

	if (!strcmp(operation, "shownodes")) {
			printf("OPERATION: %s\n", operation);
			fs_show_connected_nodes();

		}

	if (!strncmp(operation, "exit", 4)) {

		free(parameter_list);
		return NULL;
	}


	free(operation);
	free(parameter_list);

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

char *fs_console_getNParameterFromList(int index, char **list) {



}
int fs_console_operationEndedSuccessfully(int operationResult) {

	if (operationResult == 0)
		return 1;
	return 0;
}

int cantidadDe(char** algo)
{
    int i=0;
    while(algo[i])
    {
        i++;
    }
    return i;
}

