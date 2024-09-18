#include "idb.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern void assertFlagArgProvided(void) {
	if (curArg == argc_ - 1) {
		printf("\n%s: Error: Import file name not specified.\n", (*argv_)[0]);
		term(ERROR_BAD_ARGUMENTS);
	} // End of If
} // End of Function


extern char *getFlagArg(void) {
	size_t bufLen;
	char *str;

	curArg++;
	if (curArg >= argc_) // Let's try this with greater than (was >=)
		return NULL;

	bufLen = strlen((*argv_)[curArg]) + 1;
	str = (char*) malloc(bufLen);

	if (!str)
		return NULL;

	sprintf_s(str, bufLen, "%s", (*argv_)[curArg]);
	printf("getFlagArg: '%s'\n", str);

	return str;
} // End of Function


extern int argParse(char *arg, char **fmt, const int numFlags) {
	int curFlag;
	char *usrFlag;
	bool match;


	match = false;
	usrFlag = NULL;


	if (curArg >= argc_ || !arg)
		return -1;

	// TODO: Use a regex to check for letters only

	if (arg[0] == FLAGCHAR) {
		size_t argLen;

		argLen = strlen(arg);
		usrFlag = (char*) malloc(argLen);
		if (usrFlag == NULL) {
			printf("argParse: Warning: Could not allocate memory.\n");
			return -1;
		}

		for (size_t i = 1; i <= argLen; i++)
//		for (size_t i = 1; i < argLen; i++)
			usrFlag[i - 1] = arg[i];
	} else {
		printf("argParse: '%s' is not a flag.\n", arg);
		return -1;
	} // End of If/Else


	for (curFlag = 0; curFlag < numFlags; curFlag++) {
		if (strcmp(usrFlag, fmt[curFlag]) == 0) {
			match = true;
			break;
		} // End of If
	} // End of For

	free(usrFlag);
	usrFlag = NULL;

	return match ? curFlag : -1;
} // End of Function
