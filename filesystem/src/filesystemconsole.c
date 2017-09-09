#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include "filesystem.h"

char* fs_console_getOpFromInput(char *userInput);
void fs_console();
int fs_console_validateOp(char * newLine);
char *fs_console_getFirstParameterFromLine(char *userInput);
char *fs_console_getSecondParameterFromLine(char *userInput);
char *fs_console_getThirdParameterFromLine(char *userInput);

int fs_console_operationEndedSuccessfully(int);

void main() {

	fs_console();
}

void fs_console() {

	char *userInput;
	int operationResult;

	while (1) {

		userInput = readline(">");

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

	if (strcmp(operation, "format") == 0) {
		printf("%s\n", newLine);
	}

	if (!strcmp(operation, "rm")) {
		printf("%s\n", newLine);

	}

	if (!strcmp(operation, "rename")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getFirstParameterFromLine(newLine);
		secondParameter = fs_console_getSecondParameterFromLine(newLine);

		opResult = fs_rename(firstParameter, secondParameter);

		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("rename operation failed\n");

	}

	if (!strcmp(operation, "mv")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getFirstParameterFromLine(newLine);
		secondParameter = fs_console_getSecondParameterFromLine(newLine);
		opResult = fs_mv(firstParameter, secondParameter);

		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mv operation failed\n");
	}

	if (!strcmp(operation, "cat")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getFirstParameterFromLine(newLine);
		opResult = fs_cat(firstParameter);

		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mv operation failed\n");

	}

	if (!strcmp(operation, "mkdir")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getFirstParameterFromLine(newLine);
		opResult = fs_mkdir(firstParameter);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mkdir operation failed\n");
	}
	if (!strcmp(operation, "cpfrom")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getFirstParameterFromLine(newLine);
		secondParameter = fs_console_getSecondParameterFromLine(newLine);

		opResult = fs_cpfrom(firstParameter, secondParameter);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("mkdir operation failed\n");

	}

	if (!strcmp(operation, "cpto")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getFirstParameterFromLine(newLine);
		secondParameter = fs_console_getSecondParameterFromLine(newLine);
		opResult = fs_cpfrom(firstParameter, secondParameter);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("cpto operation failed\n");

	}

	if (!strcmp(operation, "cpblock")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getFirstParameterFromLine(newLine);
		secondParameter = fs_console_getSecondParameterFromLine(newLine);
		thirdParameter = fs_console_getThirdParameterFromLine(newLine);
		opResult = fs_cpblock(firstParameter, atoi(secondParameter), atoi(thirdParameter));
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("cpblock operation failed\n");

	}

	if (!strcmp(operation, "md5")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getFirstParameterFromLine(newLine);
		opResult = fs_md5(firstParameter);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("md5 operation failed\n");
	}

	if (!strcmp(operation, "ls")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getFirstParameterFromLine(newLine);
		opResult = fs_ls(firstParameter);
		if (!fs_console_operationEndedSuccessfully(opResult))
			printf("ls operation failed\n");
	}

	if (!strcmp(operation, "info")) {
		printf("OPERATION: %s\n", operation);
		firstParameter = fs_console_getFirstParameterFromLine(newLine);
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

char *fs_console_getFirstParameterFromLine(char *userInput) {

	char * userInputCopy = malloc(strlen(userInput) + 1);
	strcpy(userInputCopy, userInput);
	char * output = strtok(userInputCopy, " ");

	output = strtok(NULL, " \n");

	return output;

}

char *fs_console_getSecondParameterFromLine(char *userInput) {

	char * userInputCopy = malloc(strlen(userInput) + 1);
	strcpy(userInputCopy, userInput);
	char * output = strtok(userInputCopy, " ");

	output = strtok(NULL, " \n");
	output = strtok(NULL, " \n");

	return output;

}

char *fs_console_getThirdParameterFromLine(char *userInput) {

	char * userInputCopy = malloc(strlen(userInput) + 1);
	strcpy(userInputCopy, userInput);
	char * output = strtok(userInputCopy, " ");

	output = strtok(NULL, " \n");
	output = strtok(NULL, " \n");
	output = strtok(NULL, " \n");

	return output;

}

int fs_console_operationEndedSuccessfully(int operationResult) {

	if (operationResult == 0)
		return 1;
	return 0;
}
