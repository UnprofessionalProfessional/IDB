#include "idb.h"

#include <shlobj_core.h>
#include <Shlwapi.h>
#include <Windows.h>
#include <winerror.h>

#include <libpq-fe.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sha-256.h>


char *appData[2];
size_t appDataLen[1]; // TODO: Get rid of this


static void getAppDataPath(void);


extern bool checkFileExists(const char *path) {
	return PathFileExistsA(path) ? true : false;
} // End of Function


extern int dirInit(void) {
	DWORD appDataAttrib;
	size_t pathLen;
	char *dbName;
	int ret;

	dbName = PQdb(conn);
	if (!dbName)
		term(ERROR_OUTOFMEMORY);

	getAppDataPath();
	pathLen = strlen(appData[0]) + strlen(dbName) + 8;
	appData[1] = (char*) malloc(pathLen);

	if (!(appData[1]))
		term(ERROR_OUTOFMEMORY);

	sprintf_s(appData[1], pathLen, "%s\\%s\\media", appData[0], dbName);
	//free(dbName);	// FIXME: We have a memory leak, because if we free() dbName, it crashes
//	printf("'%s'\n", appData[1]);
	dbName = NULL;

	appDataAttrib = GetFileAttributesA(appData[1]);
	if (appDataAttrib == INVALID_FILE_ATTRIBUTES && (appDataAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
		printf("directory does not exist\n");
		if ((ret = SHCreateDirectoryExA(NULL, appData[1], NULL)) != 0)
			return ret;
	} // End of If


	return 0;
} // End of Function


static void getAppDataPath(void) {
	char *str;
	size_t strLen;

	_dupenv_s(appData, appDataLen, "LOCALAPPDATA");
	strLen = strlen(appData[0]) + 17;
	str = (char*) malloc(strLen);
	if (str == NULL) {
		dbDisconnect();
		exit(ERROR_OUTOFMEMORY);
	}

	sprintf_s(str, strLen, "%s\\idb\\Databases", appData[0]);

	free(appData[0]);
	appData[0] = NULL;

	appData[0] = (char*) malloc(strLen);
	if (appData[0] == NULL) {
		dbDisconnect();
		exit(ERROR_OUTOFMEMORY);
	}

	memmove_s(appData[0], strLen, str, strLen);
	free(str);
	str = NULL;
}


extern int getFileExtension(char **buf, const char *fileName) {
	const char *dot = strrchr(fileName, '.');
	size_t len;
	int ret;

	len = strlen(dot) + 1;
	*buf = (char*) malloc(len);
	if (*buf == NULL)
		return 0;

	ret = sprintf_s(*buf, len, "%s", dot);
	return ret;
} // End of Function


extern void getSHA256(char **dest, char *dat, size_t len) {
	uint8_t raw[32]; // Raw SHA256 data

	calc_sha_256(raw, dat, len);
	hash_to_string(dest, raw);
} // End of Function
