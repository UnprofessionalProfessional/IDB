#include "idb.h"

#include <Windows.h>
#include <winerror.h>
#include <shlobj_core.h>

#include <libpq-fe.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Creates a value in `table` if it does not exist.
 * @param table: Table to insert into
 * @param value: Name of value
 * @param id: ID of value (Required)
 * @return True if no errors were encountered.
*/
static bool sanityInsert(char *table, char *value, char *id) {
	PGresult *res;
	char *query;
	size_t queryLen;

	if (checkExists(table, "name", value))
		return true;

	queryLen = 53 + strlen(table) + strlen(value) + strlen(id);
	query = (char *) malloc(queryLen);
	printf("%zu\n", queryLen);
	if (query == NULL) {
		rollback(&res);
		PQclear(res);
		return false;
	}

	sprintf_s(query, queryLen, "INSERT INTO public.%s (name, id) VALUES ('%s', %s);", table, value, id);
	res = PQexec(conn, query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		PQclear(res);
		rollback(&res);
		PQclear(res);
		free(query);
		query = NULL;

		return false;
	} else {
		PQclear(res);
		free(query);
		query = NULL;

		return true;
	}
}

extern void checkSanity(void) {
	PGresult *res;
	int i;
	char *num;
	char *categories[] = {
		"general",
		"meta",
		"artist",
		"copyright",
		"character",
		"location",
	};
	char *ratings[] = {
		"general",
		"sensitive",
		"questionable",
		"explicit"
	};
	char *tags[] = {
		"tagme",
		"animated",
		"video",
		"vector",
		"translated",
		"translation_request",
		"commentary",
		"commentary_request",
	};
	char *poolTypes[] = {
		"generic",
		"manga",
		"comic",
		"album",
	};

	res = PQexec(conn, "BEGIN TRANSACTION;");
	PQclear(res);

	/* Tag Categories */
	for (i = 0; i < arrayLen(categories); i++) {
		size_t il;

		il = (intLen(i + 1)) + 1;
		num = (char *) malloc(il);
		if (num == NULL) {
			PQclear(res);
			rollback(&res);
			PQclear(res);
			return;
		}

		sprintf_s(num, il, "%d", i + 1);
		if (!sanityInsert("categories", categories[i], num)) {
			rollback(&res);
			free(num);
			num = NULL;
			return;
		}

		free(num);
		num = NULL;
	}

	/* Ratings */
	for (i = 0; i < arrayLen(ratings); i++) {
		size_t il;

		il = (intLen(i + 1)) + 1;
		num = (char *) malloc(il);
		if (num == NULL) {
			PQclear(res);
			rollback(&res);
			PQclear(res);
			return;
		}

		sprintf_s(num, il, "%d", i + 1);
		if (!sanityInsert("ratings", ratings[i], num)) {
			rollback(&res);
			PQclear(res);
			free(num);
			num = NULL;
			return;
		}

		free(num);
		num = NULL;
	}

	/* Tags */
	for (i = 0; i < arrayLen(tags); i++) {
		size_t il;

		il = (intLen(i + 1)) + 1;
		num = (char *) malloc(il);
		if (num == NULL) {
			PQclear(res);
			rollback(&res);
			PQclear(res);
			return;
		}

		sprintf_s(num, il, "%d", i + 1);
		if (!sanityInsert("tags", tags[i], num)) {
			rollback(&res);
			free(num);
			num = NULL;
			return;
		}

		free(num);
		num = NULL;
	}

	/* Pool Types */
	for (i = 0; i < arrayLen(poolTypes); i++) {
		size_t il;

		// TODO: Try casting to an int
		il = (intLen(i + 1)) + 1;
		num = (char *) malloc(il);
		if (num == NULL) {
			PQclear(res);
			rollback(&res);
			PQclear(res);
			return;
		}

		sprintf_s(num, il, "%d", i + 1);
		if (!sanityInsert("pool_types", poolTypes[i], num)) {
			rollback(&res);
			PQclear(res);
			free(num);
			num = NULL;
			return;
		}

		free(num);
		num = NULL;
	}

	res = PQexec(conn, "COMMIT TRANSACTION;");	// FIXME: This never works in Powershell or Command Prompt; but it does in Cygwin
	PQclear(res);
}

extern bool checkExists(const char *table, const char *column, const char *value) {
	PGresult *res;
	char *query;
	size_t queryLen;

	queryLen = 54 + strlen(table) + strlen(column);
	query = (char *) malloc(queryLen);
	if (!query)
		term(ERROR_OUTOFMEMORY);

	sprintf_s(query, queryLen, "SELECT exists(SELECT * FROM public.%s WHERE %s = $1);", table, column);

	res = PQexecParams(conn, query, 1, NULL, (const char *[]) { value }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "checkExists", false))
		return false;


	free(query);
	query = NULL;

	if (PQgetvalue(res, 0, 0)[0] == 't') {
		PQclear(res);
		return true;
	} else {
		PQclear(res);
		return false;
	} // End of If/Else
} // End of Class

extern void rollback(PGresult **res) {
	*res = PQexec(conn, "ROLLBACK TRANSACTION;");
	if (PQresultStatus(*res) != PGRES_COMMAND_OK) {
		printf("%s", PQerrorMessage(conn));
		term(ERROR_DATABASE_FAILURE);
	} // End of If
} // End of Function

extern void begin(PGresult **res) {
	*res = PQexec(conn, "BEGIN TRANSACTION;");
	if (PQresultStatus(*res) != PGRES_COMMAND_OK) {
		printf("%s", PQerrorMessage(conn));
		term(ERROR_DATABASE_FAILURE);
	} // End of If

	PQclear(*res);
} // End of Function

extern void commit(PGresult **res) {
	*res = PQexec(conn, "COMMIT TRANSACTION;");
	if (PQresultStatus(*res) != PGRES_COMMAND_OK) {
		printf("%s", PQerrorMessage(conn));
		term(ERROR_DATABASE_FAILURE);
	} // End of If

	PQclear(*res);
} // End of Function

/* @brief Disconnect from Postres.
 * @param Return code of the process
 */
extern void term(int ret) {
	dbDisconnect();
	exit(ret);
} // End of Function

extern bool update(const char *table, const char *id) {
	char *query;
	size_t queryLen;
	PGresult *res;


	queryLen = strlen(table) + 53;
	query = (char *) malloc(queryLen);
	if (!query) {
		printf("update: Error: Out of memory.\n");
		term(ERROR_OUTOFMEMORY);
	} // End of If


	sprintf_s(query, queryLen, "UPDATE public.%s SET updated = now() WHERE id = $1;", table);
	res = PQexecParams(conn,
			   query,
			   1, NULL, (const char *[]) { id }, NULL, NULL, 0);
	if (!checkCmdOk(&res, "update", false))
		term(ERROR_DATABASE_FAILURE);


	PQclear(res);
	free(query);
	query = NULL;
	return true;
} // End of Class


extern bool checkTuplesOk(PGresult **res, char *caller, const bool rb) {
	if (PQresultStatus(*res) != PGRES_TUPLES_OK) {
//		printf("%s: %s", caller, PQresultErrorMessage(res));
		printf("%s: %s", caller, PQerrorMessage(conn));

		if (rb) {
			PQclear(*res);
			rollback(res);
			PQclear(*res);
		} // End of If

		return false;
	} else {
		return true;
	} // End of If/Else
} // End of Function


extern bool checkCmdOk(PGresult **res, const char *caller, const bool rb) {
	if (PQresultStatus(*res) != PGRES_COMMAND_OK) {
//		printf("%s: %s\n", caller, PQresultErrorMessage(*res));
		printf("%s: %s", caller, PQerrorMessage(conn));

		if (rb) {
			PQclear(*res);
			rollback(res);
		} // End of If

		return false;
	} else {
		return true;
	} // End of If/Else
} // End of Function
