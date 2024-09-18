/* TODO: Spin off the table algorithm into its own program.
 */
#include "idb.h"

#include <Windows.h>
#include <winerror.h>

#include <libpq-fe.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// FIXME: gets the length of the longest tuple in the table, not the query.
extern int getLongestTupleLength(const char *table, const char *column) {
	char *query;
	size_t queryLen;
	PGresult *res;
	int ret;


	/* Generate Query */
	queryLen = strlen(table) + strlen(column) + 51;
	query = (char *) malloc(queryLen);
	checkNull(query, "getLongestTuple");
	sprintf_s(query, queryLen, "SELECT length(%s) FROM public.%s ORDER BY length DESC;", column, table);


	/* Run Query */
	res = PQexec(conn, query);
	if (!checkTuplesOk(&res, "getLongestTuple", false))
		term(ERROR_DATABASE_FAILURE);


	/* Finish */
	ret = atoi(PQgetvalue(res, 0, 0));
	PQclear(res);
	free(query);
	query = NULL;

	return ret;
}

extern int getTupleLength(const char *table, const char *column, const char *value) {
	PGresult *res;
	int ret;


	res = PQexecParams(conn,
			   "SELECT length($2) FROM public.$1 WHERE $2 = $3;",
			   3, NULL, (const char *[]) { table, column, value }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "getTupleLength", false))
		term(ERROR_DATABASE_FAILURE);


	ret = PQgetlength(res, 0, 0);
	PQclear(res);
	return ret;
} // End of Function

extern void printRowSegment(char *value, size_t longestValueLen, uint8_t eolChar, bool centre) {
	size_t i;
	size_t middle;
	double middleF;
	size_t valueLen;


	valueLen = strlen(value);
	if (centre) {
		/* Calculation */
		if (valueLen > longestValueLen) {
			middle = 1;
		} else {
			middleF = (float) (longestValueLen / 2.0f) - (valueLen / 2.0f);
			middle = (size_t) (middleF + 1.0);
		}

		/* Work */
		for (i = 0; i < middle; i++)
			printf(" ");
		printf("%s", value);

		for (i = 0; i < middle; i++)
			printf(" ");
	} else {
		/* Calculation */
		if (valueLen > longestValueLen)
			middle = valueLen - longestValueLen;
		else
			middle = longestValueLen - valueLen;

		/* Work */
		printf(" %s", value);
		for (i = 0; i < middle + 1; i++)
			printf(" ");
	}

	/* End of Line */
	switch (eolChar) {
		case 0:
			break;
		case 1:
			printf("|");
			break;
		case 2:
			printf("\n");
		default:
			break;
	}
}
