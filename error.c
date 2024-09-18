#include "idb.h"

#include <Windows.h>

#include <libpq-fe.h>
#include <stdio.h>

extern void checkPqError(PGresult *res) {
	switch (PQresultStatus(res)) {
		case PGRES_COMMAND_OK:
#ifdef DBG
			printf("Command OK\n");
#endif
			break;

		case PGRES_TUPLES_OK:
#ifdef DBG
			printf("Tuples OK\n");
#endif
			break;

		case PGRES_FATAL_ERROR:
			dbDisconnect();
			printf("%s", PQerrorMessage(conn));
			exit(ERROR_DATABASE_FAILURE);
			break;

		case PGRES_SINGLE_TUPLE:
#ifdef DBG
			printf("Single tuple\n");
#endif // DBG
			break;

		default:
			printf("checkPqError: %s", PQerrorMessage(conn));
			break;
	}
}

extern bool shouldRollback(PGresult *res, int status) {
	if (PQresultStatus(res) != status) {
		printf("%s", PQerrorMessage(conn));
		PQclear(res);

		res = PQexec(conn, "ROLLBACK TRANSACTION;");
		checkPqError(res);
		PQclear(res);

		return true;
	} else {
		return false;
	}
}

extern void checkNull(const char *str, const char *caller) {
	if (!str) {
		printf("%s: Error: NULL String.\n", caller);
		term(ERROR_BAD_ARGUMENTS);
	} // End of If
} // End of Function