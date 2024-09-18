#include "idb.h"

#include <Windows.h>
#include <winerror.h>

#include <libpq-fe.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern void queryImgCmd(void) {
	char *queryArgs[] = {
		"and",		"a",	// Images which have this tag.
		"not",		"n",	// Images which do NOT have this tag.
		"or",			// Images which tag A or tag B.
		"xor",		"x",	// Images which have either tag A, or tag B.
		"bli",		"b",	// Blacklists to be temporarily included
		"date",		"d",	// "before", "after", or "on"; followed by a timestamp.
		"pool",		"p",	// Query only from a pool.
	};

	struct tagNode *andRoot;
	struct tagNode *bliRoot;
	struct datNode *datRoot;	// TODO
	struct tagNode *notRoot;
	struct orNode *orRoot;
	struct orNode *xorRoot;

	struct tagNode *andCurr;
	struct tagNode *bliCurr;
	struct datRoot *datCurr;	// TODO
	struct tagNode *notCurr;
	struct orNode *orCurr;
	struct orNode *xorCurr;

	char *pool;			// TODO


	/* Setup */
	pool = NULL;
	curArg++;

	andRoot = NULL;
	bliRoot = NULL;
	datRoot = NULL;
	notRoot = NULL;
	orRoot = NULL;
	xorRoot = NULL;

	andCurr = NULL;
	bliCurr = NULL;
	datCurr = NULL;
	notCurr = NULL;
	orCurr = NULL;
	xorCurr = NULL;


	/* Parse Arguments */
	while ((flag = argParse((*argv_)[curArg], queryArgs, arrayLen(queryArgs))) != -1) {
		switch (flag) {
			/* AND */
			case 0:
			case 1: {
				char *tmp;	// Store tag name
				size_t tmpLen;


				tmp = getFlagArg();
				tmpLen = strlen(tmp) + 1;


				/* If root has not been set, initialize.
				 * If root has been set, initialize ->next
				 */
				if (!andRoot) {
					andRoot = (struct tagNode *) malloc(sizeof(struct tagNode));
					if (!andRoot) {
						printf("queryImages: Error: Out of memory.\n");
						term(ERROR_OUTOFMEMORY);
					} // End of If

					andCurr = andRoot;
					andCurr->next = NULL;
				} else {
					andCurr->next = (struct tagNode *) malloc(
						sizeof(struct tagNode));
					if (!(andCurr->next)) {
						printf("query: Error: Out of memory; struct could not be allocated.\n");
						term(ERROR_OUTOFMEMORY);
					} // End of If

					andCurr = andCurr->next;
					andCurr->next = NULL;
				} // End of If/Else


				andCurr->name = (char *) malloc(tmpLen);
				if (!andCurr->name) {
					printf("query: Error: Out of memory; name could not be allocated.\n");
					term(ERROR_OUTOFMEMORY);
				} // End of If

				sprintf_s(andCurr->name, tmpLen, "%s", tmp);
				//							printf("0x%p\n", andRoot->name);
				free(tmp);
				tmp = NULL;

				break;
			} // End of Case


			/* Not */
			case 2:
			case 3: {
				if (!notRoot) {
					notRoot = (struct tagNode *) malloc((sizeof(struct tagNode)));
					if (!notRoot) {
						printf("query: Error: Out of memory.\n");
						term(ERROR_OUTOFMEMORY);
					} // End of If

					notCurr = notRoot;
				} // End of If

				notCurr = (struct tagNode *) malloc(sizeof(struct tagNode));
				if (!(notCurr->next)) {
					printf("query: Error: Out of memory.\n");
					term(ERROR_OUTOFMEMORY);
				} // End of If

				notCurr->name = getFlagArg();
				notCurr = notRoot->next;
				break;
			} // End of Case


			/* Or */
			case 4: {
				if (!orRoot) {
					orRoot = (struct orNode *) malloc(sizeof(struct orNode));
					if (!orRoot) {
						printf("queryImages: Error: Out of memory.\n");
						term(ERROR_OUTOFMEMORY);
					} // End of IF

					orRoot->tagA = NULL;
					orRoot->tagB = NULL;
					orCurr = orRoot;
				} else {
					orCurr->next = (struct orNode *) malloc(sizeof(struct orNode));
					if (!orCurr->next) {
						printf("queryImages: Error: Out of memory.\n");
						term(ERROR_OUTOFMEMORY);
					} // End of If

					orCurr = orCurr->next;
				} // End of If/Else

				orCurr->tagA = getFlagArg();
				orCurr->tagB = getFlagArg();
				break;
			} // End of Case


			/* XOR */
			case 5:
			case 6: {
				if (!xorRoot) {
					xorRoot = (struct orNode *) malloc( (sizeof(struct orNode)));
					if (!xorRoot) {
						printf("query: Error: Out of memory.\n");
						term(ERROR_OUTOFMEMORY);
					} // End of If

					xorCurr = xorRoot;
					xorRoot->tagA = NULL;
					xorRoot->tagB = NULL;
					xorCurr->next = NULL;
				} else {
					xorCurr->next = (struct orNode *) malloc(sizeof(struct orNode));
					if (!(xorCurr->next)) {
						printf("queryImages: Error: Out of memory.\n");
						term(ERROR_OUTOFMEMORY);
					} // End of If

					xorCurr = xorCurr->next;
				} // End of If/Else

				xorCurr->tagA = getFlagArg();
				xorCurr->tagB = getFlagArg();
				break;
			} // End of Case


			/* BLI */
			case 7:
			case 8: {
			}


			/* Date */
			/*
			 * 1. Before/After/On
			 * 2. Timestamp
			 */
			case 9:
			case 10: {
			} // End of Case

			/* Pool */
			case 11:
			case 12: {
				pool = getFlagArg();
				break;
			} // End of Case


			default:
				break;
		} // End of Switch

		curArg++;
	} // End of While


	/* Run */
	queryImages2(andRoot, bliRoot, notRoot, orRoot, xorRoot, datRoot);
} // End of Function


// TODO: Implement date & time constraints
// TODO: Implement wildcards
// *foo		->	LIKE '%foo'
// foo*		->	LIKE 'foo%'
// *foo*	->	LIKE	'%foo%'
// *f*oo	->	LIKE	'%f%oo'
// #NOTE: The NOT operator is used last.
extern void queryImages2(struct tagNode *andRoot, struct tagNode *bliRoot, struct tagNode *notRoot,
			 struct tagNode *orRoot, struct orNode *xorRoot, struct dateNode *datRoot) {
	PGresult *res;
	char *queries[] = {
		"CREATE TEMPORARY TABLE and_tags (id INTEGER UNIQUE NOT NULL);",
		"CREATE TEMPORARY TABLE not_tags (id INTEGER UNIQUE NOT NULL);",
		"CREATE TEMPORARY TABLE or_tags (id INTEGER UNIQUE NOT NULL);",
		"CREATE TEMPORARY TABLE xor_tags (a_id INTEGER UNIQUE NOT NULL, b_id INTEGER UNIQUE NOT NULL);",
		"CREATE TEMPORARY TABLE out_imgs (id INTEGER UNIQUE NOT NULL);",
		"INSERT INTO not_tags (id) SELECT id FROM public.tags WHERE blacklisted = true;",
		"DELETE FROM and_tags WHERE id = (SELECT * FROM not_tags);",
		"DELETE FROM and_tags WHERE id = (SELECT * FROM or_tags);",
		"DELETE FROM and_tags WHERE id = (SELECT a_id FROM xor_tags UNION SELECT b_id FROM xor_tags);",
		"DELETE FROM or_tags WHERE id = (SELECT * FROM not_tags);",
		"DELETE FROM or_tags WHERE id = (SELECT a_id FROM xor_tags UNION SELECT b_id FROM xor_tags);",
		"DELETE FROM xor_tags WHERE a_id = (SELECT * FROM not_tags);",
		"DELETE FROM xor_tags WHERE b_id = (SELECT * FROM not_tags);",
		"INSERT INTO out_imgs (id) SELECT DISTINCT and_query((SELECT ARRAY(SELECT id FROM and_tags)));",
		"INSERT INTO out_imgs (id) SELECT DISTINCT or_query((SELECT ARRAY(SELECT id FROM or_tags)));",
		"INSERT INTO out_imgs (id) SELECT DISTINCT xor_query((SELECT ARRAY(SELECT id FROM xor_tags)));"
	}; // End of Array


	/* 1. Begin Transaction*/
	begin(&res);


	/* Create Temporary Tables & Add Blacklists */
	for (int i = 0; i < 6; i++) {
		PQexec(conn, queries[i]);

		if (!checkCmdOk(&res, "queryImages2", true)) {
			printf("ERROR\n");
			term(ERROR_DATABASE_FAILURE);
		} // End of If

		PQclear(res);
	} // End of For


	/* Remove BLIs */
	printf("Creation commands OK\n");
	if (bliRoot) {
		struct tagNode *bliCurr;

		bliCurr = bliRoot;
		while (bliCurr) {
			res = PQexecParams(conn,
					   "DELETE FROM not_tags WHERE id = $1;",
				1, NULL, (const char *[]) { bliCurr->name }, NULL, NULL, 0);
			if (!checkCmdOk(&res, "queryImages2", true))
				term(ERROR_DATABASE_FAILURE);

			PQclear(res);
			bliCurr = bliCurr->next;
		} // End of While
	} // End of If


	/* Insert AND Tags */
	if (andRoot) {
		struct tagNode *andCurr;

		andCurr = andRoot;
		while (andCurr) {
//			printf("'%s'\n", andCurr->name);
			res = PQexecParams(conn,
					   "INSERT INTO and_tags (id) VALUES ((SELECT id FROM public.tags WHERE name = $1));",
					   1, NULL, (const char *[]) { andCurr->name }, NULL, NULL, 0);
			if (!checkCmdOk(&res, "queryImages2", true))
				term(ERROR_DATABASE_FAILURE);

			PQclear(res);
			andCurr = andCurr->next;
		} // End of If

		printf("AND commands OK\n");
	} // End of If


	/* Insert NOT Tags */
	if (notRoot) {
		struct tagNode *notCurr;

		notCurr = notRoot;
		while (notCurr) {
			res = PQexecParams(conn,
					   "INSERT INTO and_tags (id) VALUES ($1);",
					   1, NULL, (const char *[]) { notCurr->name }, NULL, NULL, 0);
			if (!checkCmdOk(res, "queryImages2", true))
				term(ERROR_DATABASE_FAILURE);

			PQclear(res);
			notCurr = notCurr->next;
		} // End of If
	} // End of If


	/* Insert OR Tags */
	if (orRoot) {
		struct tagNode *orCurr;

		orCurr = orRoot;
		while (orCurr) {
			res = PQexecParams(conn,
					   "INSERT INTO and_tags (id) VALUES ($1);",
					   1, NULL, (const char *[]) { orCurr->name }, NULL, NULL, 0);
			if (!checkCmdOk(res, "queryImages2", true))
				term(ERROR_DATABASE_FAILURE);

			PQclear(res);
			orCurr = orCurr->next;
		} // End of If
	} // End of If


	/* Insert XOR Tags */
	if (xorRoot) {
		struct orNode *xorCurr;

		xorCurr = xorRoot;
		while (xorCurr) {
			res = PQexecParams(conn,
					   "INSERT INTO xor_tags (a_id, b_id) VALUES ($1, $2);",
					   2, NULL, (const char *[]) { xorCurr->tagA, xorCurr->tagB }, NULL, NULL, 0);
			if (!checkCmdOk(res, "queryImages2", true))
				term(ERROR_DATABASE_FAILURE);

			PQclear(res);
			xorCurr = xorCurr->next;
		} // End of If
	} // End of If


	/* Filter Tags */
	for (int i = 6; i < 15; i++) {
		printf("%d: %s\n", i, queries[i]);
		PQexec(conn, queries[i]);
		if (!checkCmdOk(&res, "queryImages2", true))
			term(ERROR_DATABASE_FAILURE);

		printf("\n");
		PQclear(res);
	} // End of For


	/* Filter XOR Tags */
	if (xorRoot) {
		struct orNode *xorCurr;


		xorCurr = xorRoot;
		while (xorCurr) {
			res = PQexecParams(conn,
					   "INSERT INTO out_img SELECT image_id FROM (SELECT image_id, COUNT(tag_id) AS tag_count FROM public.images_tags WHERE tag_id IN ($1, $2) GROUP BY image_id) AS img_counts WHERE tag_count = 1 ON CONFLICT DO NOTHING;",
					   2, NULL, (const char *[]) { xorCurr->tagA, xorCurr->tagB }, NULL, NULL, 0);
			if (!checkCmdOk(&res, "queryImages2", true))
				term(ERROR_DATABASE_FAILURE);
			else
				PQclear(res);

			xorCurr = xorCurr->next;
		} // End of While
	} // End of If


	/* TODO: Filter By Date & Time */


	/* Print Results */
	res = PQexec(conn, "SELECT * FROM out_imgs;");
	if (!checkTuplesOk(&res, "queryImages2", true))
		term(ERROR_DATABASE_FAILURE);
	for (int i = 0; i < PQntuples(res); i++)
		printf("%s\n", PQgetvalue(res, i, 0));
	PQclear(res);


	/* Terminate */
	rollback(&res);
	term(ERROR_SUCCESS);
} // End of Function
