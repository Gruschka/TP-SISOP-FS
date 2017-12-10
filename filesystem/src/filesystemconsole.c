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
#include <commons/log.h>

char* fs_console_getOpFromInput(char *userInput);
void fs_console_launch();
int fs_console_validateOp(char * newLine);
char *fs_console_getNParameterFromUserInput(int paramNumber, char *userInput);
int fs_console_destroyAnArrayOfCharPointers(char **array);

int fs_console_operationEndedSuccessfully(int);
t_log *logger;


void fs_console_launch() {

	char *logFile = tmpnam(NULL);
	logger = log_create(logFile, "FS", 1, LOG_LEVEL_DEBUG);
	char *userInput;
	int operationResult;
	log_debug(logger,"fs_console_launch: Console initiated");

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
int fs_console_destroyAnArrayOfCharPointers(char **array){

	int length = fs_amountOfElementsInArray(array);
	int i = 0;
	for(i = 0; i < length; i++){
		free(array[i]);
	}
	free(array);
	return EXIT_SUCCESS;


}
int fs_console_validateOp(char * newLine) {

	if(strlen(newLine) == 0){
		printf("No input provided\n");
		return EXIT_FAILURE;
	}

    char **parameter_list = string_split(newLine," ");
    int sizeof_parameter_list = cantidadDe(parameter_list);

//	char *operation = malloc(sizeof(parameter_list[0]));
//	memset(operation, 0, sizeof(parameter_list[0]));
//	strcpy(operation, parameter_list[0]);

	char *operation = malloc(strlen(parameter_list[0])+1);
	strcpy(operation, parameter_list[0]);
	//operation = fs_console_getOpFromInput(newLine);

	int opResult; //0 if success -1 if fails

	/************ format **************/
	if (strcmp(operation, "format") == 0) {
		log_debug(logger,"Console operation: [%s]", operation);
		fs_format();

	}

	/************ rm **************/
	if (!strcmp(operation, "rm")) {
		log_debug(logger,"Console operation: [%s]", operation);

		//-d
		if (!strcmp(parameter_list[1], "-d")) {
			opResult = fs_rm_dir(parameter_list[2]);
			if (!fs_console_operationEndedSuccessfully(opResult))
				log_error(logger,"fs_console_validateOp:rm -d operation failed");
		}

		//-b
		if (!strcmp(parameter_list[1], "-b")) {

			opResult = fs_rm_block(parameter_list[2], atoi(parameter_list[3]),
					atoi(parameter_list[4]));

			if (!fs_console_operationEndedSuccessfully(opResult))
				log_error(logger,"fs_console_validateOp:rm -b operation failed");

		} else {

			if(sizeof_parameter_list == 2){
				//If not -b or -d
				opResult = fs_rm(parameter_list[1]);
				if (!fs_console_operationEndedSuccessfully(opResult))
					log_error(logger,"fs_console_validateOp:rm operation failed");
			}
		}
	}

	if (!strcmp(operation, "rename")) {
		log_debug(logger,"Console operation: [%s]", operation);
		opResult = fs_rename(parameter_list[1], parameter_list[2]);

		if (!fs_console_operationEndedSuccessfully(opResult))
			log_error(logger,"fs_console_validateOp: rename operation failed");

	}

	if (!strcmp(operation, "mv")) {
		log_debug(logger,"Console operation: [%s]", operation);

		opResult = fs_mv(parameter_list[1], parameter_list[2]);

		if (!fs_console_operationEndedSuccessfully(opResult))
			log_error(logger,"fs_console_validateOp:mv operation failed");
	}

	if (!strcmp(operation, "cat")) {
		log_debug(logger,"Console operation: [%s]", operation);
		opResult = fs_cat(parameter_list[1]);

		if (!fs_console_operationEndedSuccessfully(opResult))
			log_error(logger,"fs_console_validateOp:cat operation failed");

	}

	if (!strcmp(operation, "mkdir")) {
		log_debug(logger,"Console operation: [%s]", operation);
		opResult = fs_mkdir(parameter_list[1]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			log_error(logger,"fs_console_validateOp:mkdir operation failed");
	}
	if (!strcmp(operation, "cpfrom")) {
		log_debug(logger,"Console operation: [%s]", operation);
		opResult = fs_cpfrom(parameter_list[1], parameter_list[2], parameter_list[3]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			log_error(logger,"fs_console_validateOp:cpfrom operation failed");

	}

	if (!strcmp(operation, "cpto")) {
		log_debug(logger,"Console operation: [%s]", operation);
		opResult = fs_cpto(parameter_list[1], parameter_list[2]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			log_error(logger,"fs_console_validateOp:cpto operation failed");

	}

	if (!strcmp(operation, "cpblock")) {
		log_debug(logger,"Console operation: [%s]", operation);
		opResult = fs_cpblock(parameter_list[1], atoi(parameter_list[2]),
				parameter_list[3]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			log_error(logger,"fs_console_validateOp:cpblock operation failed");

	}

	if (!strcmp(operation, "md5")) {
		log_debug(logger,"Console operation: [%s]", operation);
		opResult = fs_md5(parameter_list[1]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			log_error(logger,"fs_console_validateOp:md5 operation failed");
	}

	if (!strcmp(operation, "ls")) {
		log_debug(logger,"Console operation: [%s]", operation);
		opResult = fs_ls(parameter_list[1]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			log_error(logger,"fs_console_validateOp:ls operation failed");
	}

	if (!strcmp(operation, "info")) {
		log_debug(logger,"Console operation: [%s]", operation);
		opResult = fs_info(parameter_list[1]);
		if (!fs_console_operationEndedSuccessfully(opResult))
			log_error(logger,"fs_console_validateOp:info operation failed");

	}

	if (!strcmp(operation, "shownodes")) {
		log_debug(logger,"Console operation: [%s]", operation);
		fs_show_connected_nodes();

		}

	if (!strncmp(operation, "exit", 4)) {
		log_debug(logger,"Console operation: [%s]", operation);
		free(parameter_list);
		free(operation);
		return NULL;
	}


	free(operation);
	fs_console_destroyAnArrayOfCharPointers(parameter_list);
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

int cantidadDe(char** algo)
{
    int i=0;
    while(algo[i])
    {
        i++;
    }
    return i;
}


