#include "idb.h"

#include <Windows.h>
#include <winerror.h>

#include <libpq-fe.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern void favouriteInternal(char *id, bool favourite) {
	PGresult *res;
	size_t idLen;
	char *query;

	// TODO: Check type of ID
	idLen = strlen(id);
	if (favourite)
		printf("Favouriting post %s\n", id);
	else
		printf("Unfavouriting post %s\n", id);

	query = (char *) malloc(42 + idLen);
	if (query != NULL) {
		sprintf_s(query, 42 + idLen, "SELECT * FROM public.images WHERE id = %s;", id);
	} else {
		exit(ERROR_OUTOFMEMORY);
	}

	res = PQexec(conn, query);
	if (PQresultStatus(res) == PGRES_TUPLES_OK) {
		if (PQntuples(res) == 0) {
			if (verbose) {
				printf("No results\n");
				exit(ERROR_NOT_FOUND);
			}
		} else {
			printf("Returned: '%s'\n", PQgetvalue(res, 0, 0));
		}
	} else {
		printf("Error: %s", PQerrorMessage(conn));
		exit(ERROR_DATABASE_FAILURE);
	}

	PQclear(res);
	free(query);
	query = NULL;

	/// UPDATE public.images SET favourite = true WHERE id = ;
	query = (char *) malloc(56 + idLen);
	if (query != NULL) {
		if (favourite)
			sprintf_s(query, 56 + idLen, "UPDATE public.images SET favourite = true WHERE id = %s;", id);
		else
			sprintf_s(query, 57 + idLen, "UPDATE public.images SET favourite = false WHERE id = %s;", id);
	} else {
		exit(ERROR_OUTOFMEMORY);
	}

	res = PQexec(conn, query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		printf("Returned: '%s'\n", PQgetvalue(res, 0, 0));
	} else {
		if (verbose) {
			if (favourite)
				printf("Favourited post.\n");
			else
				printf("Unfavourited post.\n");
		}
	}

	PQclear(res);
	free(query);
	query = NULL;
	dbDisconnect();
	exit(0);
}
