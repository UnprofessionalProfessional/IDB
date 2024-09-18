#include "idb.h"

#include <Windows.h>
#include <winerror.h>

#include <libpq-fe.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uri_encode.h>

PGconn *conn;

/* Database Name
 * Username
 * Password
 * Address
 * Port
 * Profile?
 */
extern void dbConnect(char *username, char *dbName, char *password, char *addr, uint16_t port) {
	char *dbURI = NULL;
	char *encDbName = NULL;
	char *encUsername = NULL;
	char *encPassword = NULL;
	size_t uriLen;
	size_t encDbNameLen;
	size_t encUsernameLen;
	size_t encPasswordLen;
	size_t ret = 0;

	size_t dbNameLen;
	size_t usernameLen;
	size_t passwordLen;


/*
#ifdef DBG
	printf("Username:\t'%s'\n", username);
	printf("DB Name:\t'%s'\n", dbName);
	printf("Password:\t'%s'\n", password);
	printf("Address:\t'%s'\n", addr);
	printf("Port:\t\t%lu\n\n", port);
#endif
*/

	/* Default Settings */
	if (username == NULL) {
		char *uname[1];
		size_t s[1];
#ifdef DBG
		printf("Username not supplied; auto-generating.\n");
#endif // DBG
		_dupenv_s(uname, s, "USERNAME");
		username = (char *) malloc(s[0]);

		if (username != NULL) {
			sprintf_s(username, s[0], "%s", uname[0]);
		} else {
			printf("No memory\n");
			exit(DNS_ERROR_NO_MEMORY);
		}
	}

	if (dbName == NULL) {
#ifdef DBG
		printf("DB name not supplied; auto-generating from username: '%s'\n", username);
#endif // DBG
		if (username != NULL) {
			size_t dbNameLen;

			dbNameLen = 5 + strlen(username);
			dbName = (char *) malloc(dbNameLen);

			if (dbName != NULL) {
				sprintf_s(dbName, dbNameLen, "idb_%s", username);
			} else {
				exit(ERROR_OUTOFMEMORY);
			}
		} else {
			exit(ERROR_OUTOFMEMORY);
		}
	}

	// Remember: win32 needs an extra byte for NULL.
	if (addr == NULL) {
#ifdef DBG
		printf("DB address not supplied; assuming localhost.\n");
#endif // DBG

		addr = (char *) malloc(10);
		if (addr != NULL)
			sprintf_s(addr, 10, "%s", "localhost");
		else
			exit(ERROR_OUTOFMEMORY);
	}

	/// Port 0 is reserved, so this value is safe for this purpose.
	if (port == 0)
		port = 5432;

	/* Get Credential Lengths */
	usernameLen = strlen(username);
	encUsernameLen = usernameLen * 3 + 1;

	dbNameLen = strlen(dbName);
	encDbNameLen = dbNameLen * 3 + 1;

	if (password != NULL) {
		passwordLen = strlen(password);
		encPasswordLen = passwordLen * 3 + 1;
	} else {
		passwordLen = 0;
		encPasswordLen = 0;
	}

/*
#ifdef DBG
	printf("Automatic generation successful.\n\n");
	printf("usernameLen: %zu\n", usernameLen);
	printf("dbName: %zu\n", dbNameLen);
	printf("passwordLen: %zu\n\n", passwordLen);
	printf("Encoded usernameLen: %zu\n", encUsernameLen);
	printf("Encoded dbName: %zu\n", encDbNameLen);
	printf("Encoded passwordLen: %zu\n", encPasswordLen);

	printf("\nUpdated Credentials\n");
	printf("Username:\t'%s'\n", username);
	printf("DB Name:\t'%s'\n", dbName);
	printf("Password:\t'%s'\n", password);
	printf("Address:\t'%s'\n", addr);
	printf("Port:\t\t%lu\n\n", port);
#endif
*/
	/* Percent-Encode Credentials */
	encDbName = (char *) malloc(encDbNameLen);

	if (encDbName != NULL)
		encDbName[0] = '\0';
	else
		exit(ERROR_OUTOFMEMORY);

	encUsername = (char *) malloc(encUsernameLen);

	if (encUsername != NULL)
		encUsername[0] = '\0';
	else
		exit(ERROR_OUTOFMEMORY);

	ret = uri_encode(encDbName, dbNameLen, dbName);
	ret = uri_encode(encUsername, usernameLen, username);

	if (password != NULL) {
		encPasswordLen = strlen(password) * 3 + 1;
		encPassword = (char *) malloc(encPasswordLen);

		if (encPassword == NULL)
			exit(ERROR_OUTOFMEMORY);

		encPassword[0] = '\0';
		ret = uri_encode(encPassword, strlen(password), password);

//		printf("%zu Bytes written\n", ret);
//		printf("Buffer is %zu bytes\n\n", encPasswordLen);
	}

	/// postgresql://username:password@addr/dbName:port
	/// 19 excluding escape characters
	if (encPassword == NULL) {
		uriLen = 18 + encDbNameLen + encUsernameLen + strlen(addr) + (size_t) intLen(port);
		dbURI = (char *) malloc(uriLen);

		if (dbURI != NULL)
			sprintf_s(dbURI, uriLen, "postgresql://%s@%s:%lu/%s", encUsername, addr, port, encDbName);
		else
			exit(ERROR_OUTOFMEMORY);
	} else {
		uriLen = 19 + encDbNameLen + encUsernameLen + encPasswordLen + strlen(addr) + (size_t) intLen(port);
		dbURI = (char *) malloc(uriLen);

		if (dbURI != NULL)
			sprintf_s(dbURI, uriLen, "postgresql://%s:%s@%s:%lu/%s", encUsername, encPassword, addr, port, encDbName);
		else
			exit(ERROR_OUTOFMEMORY);
	}

//	printf("Generated: %s\n", dbURI);

	conn = PQconnectdb(dbURI);
	printf("%s", PQerrorMessage(conn));

	// TODO: Check the status of the connection.
//	if (PQstatus(conn) != PGRES_) {
//		printf("%s\n", PQerrorMessage(conn));
//		exit(ERROR_CONNECTION_ABORTED);
//	}
}

extern int cacheCredentials(bool passwd) {
	return 0;
}