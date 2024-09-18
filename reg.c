#include "idb.h"

#include <Windows.h>
#include <winerror.h>
#include <winreg.h>

#include <stdio.h>

PHKEY idbKey;

extern int initReg(void) {
	LSTATUS regRet;
	int ret;

//	printf("Initializing registry...\n");

	/* Check if key exists */
	regRet = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\idb\\profiles", 0, KEY_READ | KEY_WOW64_64KEY, &idbKey);

	switch (regRet) {
		case ERROR_SUCCESS:
//			printf("Success\n");
			return 0;

			// FIXME: ERROR_FILE_NOT_FOUND is the wrong return code.
		case ERROR_FILE_NOT_FOUND:
			printf("Creating registry key...\n");
			regRet = RegCreateKeyA(HKEY_CURRENT_USER, "Software\\idb\\profiles", &idbKey);
			if (regRet == ERROR_SUCCESS)
				printf("Created registry key");
			ret = 0;
			break;

		default:
			printf("Defaulting\n");
			return regRet;
	}

	return ret;
}