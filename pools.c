#include "idb.h"

#include <Windows.h>
#include <winerror.h>

#include <libpq-fe.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


extern void addPool(const char *name, const char *desc, char *type) {
	errno_t err;
	char *poolID;
	int poolIDlen;
	char *typeID;
	int typeIDlen;
	PGresult *res;


	/* 0. Pool Type */
	if (!type) {
		type = (char *) malloc(8);
		if (!type) {
			printf("addPool: Error: Out of memory.\n");
			term(ERROR_OUTOFMEMORY);
		} // End of If

		err = sprintf_s(type, 8, "generic");
	} // End of If


	/* 1. Check if Pool Exists */
	if (checkExists("pools", "name", name)) {
		printf("addPool: Error: Pool '%s' already exists.\n", name);
		term(ERROR_FILE_EXISTS);
	} // End of If

	/* 2. Get Type ID */
	res = PQexecParams(conn,
			   "SELECT id FROM public.pool_types WHERE name = $1;",
			   1, NULL, (const char *[]) { type }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "addPool", false))
		term(ERROR_DATABASE_FAILURE);

	if (PQntuples(res) == 0) {
		printf("addPool: Error: Invalid type.\n");
		term(ERROR_FILE_NOT_FOUND);
	} // End of If

	typeIDlen = PQgetlength(res, 0, 0) + 1;
	typeID = (char *) malloc(typeIDlen);
	checkNull(typeID, "addPool");

	err = sprintf_s(typeID, typeIDlen, "%s", PQgetvalue(res, 0, 0));
	PQclear(res);


	/* 3. Begin Transaction*/
	begin(&res);


	/* 4. Create Pool */
	if (desc != NULL) {
		res = PQexecParams(conn,
				   "INSERT INTO public.pools (name, description) VALUES ($1, $2);",
				   2, NULL, (const char *[]) { name, desc }, NULL, NULL, 0);
	} else {
		res = PQexecParams(conn,
				   "INSERT INTO public.pools (name) VALUES ($1);",
				   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
	} // End of If/Else

	if (!checkCmdOk(res, "addPool", true))
		term(ERROR_DATABASE_FAILURE);


	/* 5. Get Pool ID */
	res = PQexecParams(conn,
			   "SELECT id FROM public.pools WHERE name = $1;",
			   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "addPool", true))
		term(ERROR_DATABASE_FAILURE);

	poolIDlen = PQgetlength(res, 0, 0) + 1;
	poolID = (char *) malloc(poolIDlen);
	checkNull(poolID, "addPool");

	err = sprintf_s(poolID, poolIDlen, "%s", PQgetvalue(res, 0, 0));
	PQclear(res);


	/* 6. Add Type */
	res = PQexecParams(conn,
			   "INSERT INTO public.pools_pooltypes (pool_id, type_id) VALUES ($1, $2);",
			   2, NULL, (const char *[]) { poolID, typeID }, NULL, NULL, 0);
	if (!checkCmdOk(res, "addPool", true))
		term(ERROR_DATABASE_FAILURE);

	PQclear(res);


	/* 7. Commit Transaction */
	res = PQexec(conn, "COMMIT TRANSACTION;");
	if (!checkCmdOk(res, "addPool", true))
		term(ERROR_DATABASE_FAILURE);

	PQclear(res);
	term(ERROR_SUCCESS);
} // End of Function

extern void addPoolCmd(void) {
	char *name;
	char *description;
	char *type;

	name = getFlagArg();
	description = getFlagArg();
	type = getFlagArg();

	addPool(name, description, type);
} // End of Function

extern void editPoolCmd(void) {
	char *editPoolArgs[] = {
		"rename", "r",     // 0,1
		"description", "d" // 2, 3
	};
	char *poolName;

	/* Get values */
	assertFlagArgProvided();
	poolName = getFlagArg();
	curArg++;

	/* Check if the pool exists */
	if (!checkExists("pools", "name", poolName)) {
		printf("main: Error: Pool '%s' does not exist.\n", poolName);
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* Evaluate arguments */
	while ((flag = argParse((*argv_)[curArg], editPoolArgs, arrayLen(editPoolArgs))) != - 1) {
		switch (flag) {
			/* Rename */
			case 0:
			case 1:
				printf("Rename Pool\n");
				renamePool(poolName, getFlagArg());
				break;

			/* Edit Description */
			case 2:
			case 3:
				editPoolDescription(poolName, getFlagArg());
				break;
		} // End of Switch

		curArg++;
	} // End of While

	term(ERROR_SUCCESS);
} // End of Function

extern void editPoolDescription(char *name, char *desc) {
	PGresult *res;

	res = PQexecParams(conn,
			   "UPDATE public.pools SET description = $2 WHERE name = $1;",
			   2, NULL, (const char *[]) { name, desc }, NULL, NULL, 0);
	if (!checkCmdOk(res, "addPool", true))
		term(ERROR_DATABASE_FAILURE);
	PQclear(res);
} // End of Function

extern void dropPool(char *name) {
	PGresult *res;


	/* 1. Check if Pool Exists */
	if (!checkExists("pools", "name", name)) {
		printf("dropPool: Error: Pool '%s' does not exist.\n", name);
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* 2. Delete Pool*/
	res = PQexecParams(conn,
			   "DELETE FROM public.pools WHERE name = $1;",
			   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
	if (!checkCmdOk(res, "addPool", true))
		term(ERROR_DATABASE_FAILURE);

	if (verbose)
		printf("Deleted pool '%s'\n", name);
	term(0);
} // End of Function

extern void renamePool(char *name, char *newName) {
	PGresult *res;

	res = PQexecParams(conn,
			   "UPDATE public.pools SET name = $2 WHERE name = $1;",
			   2, NULL, (const char *[]) { name, newName }, NULL, NULL, 0);
	if (!checkCmdOk(res, "addPool", true))
		term(ERROR_DATABASE_FAILURE);
	PQclear(res);
} // End of Function
