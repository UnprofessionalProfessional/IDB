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

extern void addWiki(char *tagName, char *description) {
	PGresult *res;
	char *tagID;
	size_t tagIDlen;

	/* Get Tag ID */
	res = PQexecParams(conn,
			   "SELECT id FROM public.tags WHERE name = $1;",
			   1, NULL, (const char *[]) { tagName }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		if (verbose)
			printf("%s", PQerrorMessage(conn));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	tagIDlen = PQgetlength(res, 0, 0) + 1;
	tagID = (char *) malloc(tagIDlen);
	if (tagID == NULL) {
		if (verbose)
			printf("addWiki: Error: Out of memory.\n");
		PQclear(res);
		dbDisconnect();
		exit(ERROR_OUTOFMEMORY);
	}

	sprintf_s(tagID, tagIDlen, "%s", PQgetvalue(res, 0, 0));
	PQclear(res);


	/* Check if Wiki Page Exists */
	if (checkExists("wikis", "tag_id", tagID)) {
		if (verbose)
			printf("addWiki: Error: Wiki page already exists.\n");
		dbDisconnect();
		exit(ERROR_FILE_EXISTS);
	}


	/* Create Wiki Page */
	if (description != NULL) {
		res = PQexecParams(conn,
				   "INSERT INTO public.wikis (tag_id, description) VALUES ($1, $2);",
				   2, NULL, (const char *[]) { tagID, description }, NULL, NULL, 0);
	} else {
		res = PQexecParams(conn,
				   "INSERT INTO public.wikis (tag_id) VALUES ($1);",
				   1, NULL, (const char *[]) { tagID }, NULL, NULL, 0);
	}

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		if (verbose)
			printf("%s", PQerrorMessage(conn));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	PQclear(res);
	dbDisconnect();
	exit(ERROR_SUCCESS);
}

extern void dropWiki(char *name) {
	PGresult *res;
	char *tagID;
	int tagIDlen;

	/* Check if Tag Exists */
	if (!checkExists("tags", "name", name)) {
		if (verbose)
			printf("printWiki: Error: No such tag.\n");
		dbDisconnect();
		exit(ERROR_FILE_NOT_FOUND);
	}

	/* Get Tag ID */
	res = PQexecParams(conn,
			   "SELECT id FROM public.tags WHERE name = $1;",
			   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		if (verbose)
			printf("%s", PQerrorMessage(conn));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	tagIDlen = PQgetlength(res, 0, 0) + 1;
	tagID = (char *) malloc(tagIDlen);
	if (tagID == NULL) {
		if (verbose)
			printf("printWiki: Error: Out of memory\n");
		PQclear(res);
		dbDisconnect();
		exit(ERROR_OUTOFMEMORY);
	}

	sprintf_s(tagID, tagIDlen, "%s", PQgetvalue(res, 0, 0));
	PQclear(res);


	/* Drop Wiki Page */
	res = PQexecParams(conn,
			   "DELETE FROM public.wikis WHERE tag_id = $1;",
			   1, NULL, (const char *[]) { tagID }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		if (verbose)
			printf("%s", PQerrorMessage(conn));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	PQclear(res);
	dbDisconnect();
	exit(ERROR_SUCCESS);
}

extern void dropWikiCmd(void) {
	char *tagName;
	char *description;

	tagName = getFlagArg();
	description = getFlagArg();
	if (!description || !tagName) {
		printf("/edit-wiki: Error: Missing argument(s).\n");
		term(ERROR_BAD_ARGUMENTS);
	} // End of if

	if (!checkExists("tags", "name", tagName)) {
		printf("main: Error: Tag '%s' does not exist.\n", tagName);
		term(ERROR_FILE_NOT_FOUND);
	} // End of If

	addWiki(tagName, description);
} // End of Function

extern void editWikiCmd(void) {
	char *tagName;
	char *description;

	tagName = getFlagArg();
	description = getFlagArg();
	if (!description || !tagName) {
		printf("/edit-wiki: Error: Missing argument(s).\n");
		term(ERROR_BAD_ARGUMENTS);
	} // End of If

	if (!checkExists("tags", "name", tagName)) {
		printf("main: Error: Tag '%s' does not exist.\n", tagName);
		term(ERROR_FILE_NOT_FOUND);
	} // End of If

	editWiki(tagName, description);
} // End of Function

extern void editWiki(const char *name, const char *description) {
	PGresult *res;
	char *tagID;
	size_t tagIDlen;

	/* Get Tag ID */
	res = PQexecParams(conn,
			   "SELECT id FROM public.tags WHERE name = $1;",
			   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		if (verbose)
			printf("%s", PQerrorMessage(conn));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	tagIDlen = PQgetlength(res, 0, 0) + 1;
	tagID = (char *) malloc(tagIDlen);
	if (tagID == NULL) {
		if (verbose)
			printf("addWiki: Error: Out of memory.\n");
		PQclear(res);
		dbDisconnect();
		exit(ERROR_OUTOFMEMORY);
	}

	sprintf_s(tagID, tagIDlen, "%s", PQgetvalue(res, 0, 0));
	PQclear(res);


	/* Check if Wiki Page Exists */
	if (!checkExists("wikis", "tag_id", tagID)) {
		if (verbose)
			printf("addWiki: Error: Wiki page does not exist.\n");
		dbDisconnect();
		exit(ERROR_FILE_NOT_FOUND);
	}


	/* Create Wiki Page */
	res = PQexecParams(conn,
			   "UPDATE public.wikis SET description = $2 WHERE tag_id = $1;",
			   2, NULL, (const char *[]) { tagID, description }, NULL, NULL, 0);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		if (verbose)
			printf("%s", PQerrorMessage(conn));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	PQclear(res);
	dbDisconnect();
	exit(ERROR_SUCCESS);
}

extern void wikiCmd(void) {
	PGresult *res;
	char *tag;
	char *tagID;
	size_t tagIDlen;


	/* 1. Get Tag Name */
	tag = getFlagArg();
	if (!tag) {
		printf("/wiki: Error: No argument provided.\n");
		term(ERROR_BAD_ARGUMENTS);
	} // End of If


	/* 2. Get Tag ID */
	getTagIDbyName(&tagID, tag, &tagIDlen);


	/* 3. Print Wiki */
	res = PQexecParams(conn,
			   "SELECT description FROM public.wikis WHERE tag_id = $1;",
			   1, NULL, (const char *[]) { tagID }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "wikiCmd", false))
		term(ERROR_DATABASE_FAILURE);

	printf("%s\n", PQgetvalue(res, 0, 0));

	PQclear(res);
	term(ERROR_SUCCESS);
} // End of Function

extern void printWiki(char *name) {
	PGresult *res;
	char *tagID;
	int tagIDlen;

	/* Check if Tag Exists */
	if (!checkExists("tags", "name", name)) {
		if (verbose)
			printf("printWiki: Error: No such tag.\n");
		dbDisconnect();
		exit(ERROR_FILE_NOT_FOUND);
	}

	/* Get Tag ID */
	res = PQexecParams(conn,
			   "SELECT id FROM public.tags WHERE name = $1;",
			   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		if (verbose)
			printf("%s", PQerrorMessage(conn));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	tagIDlen = PQgetlength(res, 0, 0) + 1;
	tagID = (char *) malloc(tagIDlen);
	if (tagID == NULL) {
		if (verbose)
			printf("printWiki: Error: Out of memory\n");
		PQclear(res);
		dbDisconnect();
		exit(ERROR_OUTOFMEMORY);
	}

	sprintf_s(tagID, tagIDlen, "%s", PQgetvalue(res, 0, 0));
	PQclear(res);


	/* Get Wiki Page */
	res = PQexecParams(conn,
			   "SELECT description FROM public.wikis WHERE tag_id = $1;",
			   1, NULL, (const char *[]) { tagID }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		if (verbose)
			printf("%s", PQerrorMessage(conn));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}


	/* Output */
	printf("%s\n\n%s", name, PQgetvalue(res, 0, 0));
	PQclear(res);
	dbDisconnect();
	exit(ERROR_SUCCESS);
}