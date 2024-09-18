#include "idb.h"

#include <Windows.h>
#include <winerror.h>

#include <libpq-fe.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern void addAlias(const char *tag, const char *alias) {
	PGresult *res;


	/* 1. Check if Tag Exists */
	if (!checkExists("tags", "name", tag)) {
		printf("addAlias: Error: No such tag.\n");
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* 2. Check if Alias Exists */
	if (checkExists("aliases", "alias", alias)) {
		printf("addAlias: Error: Alias already exists.\n");
		term(ERROR_FILE_EXISTS);
	} // End of If


	/* 3. Create Alias */
	res = PQexecParams(conn,
			   "INSERT INTO public.aliases (tag_id, alias) VALUES ((SELECT id FROM public.tags WHERE name = LOWER($1)), $2);",
			   2, NULL, (const char *[]) { tag, alias }, NULL, NULL, 0);
	if (!checkCmdOk(&res, "addAlias", false)) {
		PQclear(res);
		term(ERROR_DATABASE_FAILURE);
	} else {
		PQclear(res);
	} // End of If/Else
} // End of Function


extern void addAliasCmd(void) {
	char *tagName;
	char *alias;

	tagName = getFlagArg();
	alias = getFlagArg();

	if (!tagName || !alias) {
		printf("addAlias: Error: Tag name and/or alias are NULL.\n");
		term(ERROR_BAD_ARGUMENTS);
	} // End of If

	addAlias(tagName, alias);
	free(tagName);
	free(alias);

	tagName = NULL;
	alias = NULL;
} // End of Function


extern void addImplication(char *implicator, char *implication) {
	PGresult *res;
	char *implicationID;
	size_t implicationIDlen;
	char *implicatorID;
	size_t implicatorIDlen;


	/* 1. Check if Tag Exists */
	if (!checkExists("tags", "name", implicator)) {
		printf("addImplication: Error: No such tag, '%s'\n", implicator);
		return;
	} // End of If


	/* 2. Check if Implcation Exists */
	if (!checkExists("tags", "name", implication)) {
		printf("addImplication: Error: No such tag, '%s'\n", implication);
		return;
	} // End of If


	/* 3. Get Tag IDs */
	getTagIDbyName(&implicatorID, implicator, &implicatorIDlen);
	getTagIDbyName(&implicationID, implication, &implicationIDlen);


	/* 4. Check if Bridge Exists */
	res = PQexecParams(conn,
			   "SELECT exists(SELECT * FROM public.implications WHERE implicator = $1 AND implication = $2);",
			   2, NULL, (const char *[]) { implicatorID, implicationID }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "AddImplication", false))
		term(ERROR_DATABASE_FAILURE);

	if (PQgetvalue(res, 0, 0)[0] == 't') {
		printf("addImplication: Error: Implication already exists.\n");
		PQclear(res);
		return;
	} // End of If

	PQclear(res);


	/* 5. Create Relation */
	res = PQexecParams(conn,
			   "INSERT INTO public.implications (implicator, implication) VALUES ($1, $2);",
			   2, NULL, (const char *[]) { implicatorID, implicationID }, NULL, NULL, 0);
	if (!checkCmdOk(res, "AddImplication", false))
		term(ERROR_DATABASE_FAILURE);

	if (verbose)
		printf("Created implication: '%s' + '%s'", implication, implicator);
	PQclear(res);
	free(implicationID);
	free(implicatorID);
	implicationID = NULL;
	implicatorID = NULL;
} // End of If

extern void addImplicationCmd(void) {
	char *implicator;
	char *implication;

	implicator = getFlagArg();
	implication = getFlagArg();

	if (implicator == NULL || implication == NULL) {
		printf("add-implication: Error: Missing arguments.\n");
		term(ERROR_BAD_ARGUMENTS);
	} // End of If

	addImplication(implicator, implication);

	free(implication);
	free(implicator);
	implicator = NULL;
	implication = NULL;
} // End of Function


extern void addTag3(const char *name, char *cat) {
	PGresult *res;
	char *catId;
	size_t catIdLen;
	errno_t err;


	/* 1. Check if Tag Exists */
	if (checkExists("tags", "name", name)) {
		printf("addTag3: Warning: Tag exists.\n");
		return;
	} // End of If


	/* 1.5 Set Category If NULL */
	if (!cat) {
		cat = (char *) malloc(8);
		checkNull(cat, "addTag3");
		sprintf_s(cat, 8, "general");
	} // End of If


	/* 2. Get Category ID */
	res = PQexecParams(conn, "SELECT id FROM public.categories WHERE name = LOWER($1);",
			   1, NULL, (const char *[]) { cat }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "addTag3", false))
		term(ERROR_DATABASE_FAILURE);

	if (PQntuples(res) == 0) {
		printf("addTag3: Error: No such category.\n");
		PQclear(res);
		return;
	} // End of If

	catIdLen = PQgetlength(res, 0, 0) + 1;
	catId = (char *) malloc(catIdLen);
	checkNull(catId, "addTag3");
	err = sprintf_s(catId, catIdLen, "%s", PQgetvalue(res, 0, 0));
	PQclear(res);


	/* 3. Create Tag */
	res = PQexecParams(conn, "INSERT INTO public.tags (name, category_id) VALUES (LOWER($1), $2);",
			   2, NULL, (const char *[]) { name, catId }, NULL, NULL, 0);
	if (!checkCmdOk(&res, "addTag3", false))
		term(ERROR_DATABASE_FAILURE);
	printf("Tag created\n");


	/* 4. Cleanup */
	PQclear(res);
	free(catId);
	catId = NULL;
} // End of Function


extern void addTagCmd(void) {
	char *tagName;
	char *catName;

	assertFlagArgProvided();
	tagName = getFlagArg();
	if (!tagName) {
		free(tagName);
		tagName = NULL;
		return;
	} // End of If

	catName = getFlagArg();
	if (catName) {
		if (catName[0] == FLAGCHAR) {
			curArg--;
			free(catName);
			catName = NULL;
		} // End of If
	} else {
		catName = (char *) malloc(8);
		checkNull(catName, "addTagCmd");
		sprintf_s(catName, 8, "general");
	} // End of If/Else

	addTag3(tagName, catName);

	free(catName);
	free(tagName);
	catName = NULL;
	tagName = NULL;
} // End of Function


extern bool addTagToImage(char *imgId, char *tagId) {
	PGresult *res;


	/* 1. Check Arguments */
	/// Cant do anything if these are NULL.
	if (!imgId) {
		printf("addTagToImage: Error: File ID not supplied.\n");
		return false;
	} // End of If

	if (tagId == NULL) {
		printf("addTagToImage: Error: Tag ID not supplied.\n");
		return false;
	} // End of If


	/* 2. Check if Tag Exists */
	if (!checkExists("tags", "id", tagId)) {
		printf("addTagToImage: Error: Tag does not exist.\n");
		return false;
	} // End of If


	/* 3. CHeck if Image-Tag Relation Exists */
	res = PQexecParams(conn,
			   "SELECT exists(SELECT * FROM public.images_tags WHERE image_id = $1 AND tag_id = $2);",
			   2, NULL, (const char *[]) { imgId, tagId }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		PQclear(res);
		term(ERROR_DATABASE_FAILURE);
	} // End of If

	if (PQgetvalue(res, 0, 0)[0] == 't') {
		printf("addTagToImage: Warning: Relation already exists.\n");
		PQclear(res);
		return false;
	} // End of If

	PQclear(res);


	/* 4. Insert into Database */
	res = PQexecParams(conn,
			   "INSERT INTO public.images_tags (image_id, tag_id) VALUES ($1, $2) ON CONFLICT DO NOTHING;",
			   2, NULL, (const char *[]) { imgId, tagId }, NULL, NULL, 0);
	if (!checkCmdOk(&res, "addTagToImage", true)) {
		PQclear(res);
		term(ERROR_DATABASE_FAILURE);
	} // End of If


	/* 5. Terminate */
	PQclear(res);
	if (!update("images", imgId))
		return false;
	else
		return true;
}


extern bool createTag(const char *name, const char *cat) {
	/* 1. Validate Category
	 * 2. Validate Tag
	 * 3. Create Tag
	 */

	/* Check Lengths */
	if (!name || !cat) {
		printf("createTag: Error: NULL input.\n");
		return false;
	} // End of If


	/* Validate Category */
	if (!checkExists("categories", "name", cat)) {
		printf("createTag: Error: No such category, '%s'.\n", cat);
		return false;
	} // End of If


	/* Validate Name */
	if (checkExists("tags", "name", name)) {
		printf("createTag: Error: Tag '%s' Exists.\n", name);
		return false;
	} // End of If


	/* Create Tag */

	return true;
} // End of Function


extern void blacklistInternal(char *tag, bool blacklist) {
	PGresult *res;


	/* 1. Check Arguments */
	if (tag == NULL) {
		usage();
		exit(ERROR_BAD_ARGUMENTS);
	}


	/* 2. Check if Alias */
	res = PQexecParams(conn,
			   "SELECT tag_id FROM public.aliases WHERE alias = $1;",
			   1, NULL, (const char *[]) { tag }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		if (verbose)
			printf("%s", PQresultErrorMessage(res));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	if (PQntuples(res)) {
		char *id;
		int idLen;
		int tagLen;

		/* 1. Get ID */
		idLen = PQgetlength(res, 0, 0) + 1;
		id = (char *) malloc(idLen);
		if (id == NULL) {
			if (verbose)
				printf("blacklistInternal: Error: Out of memory.\n");
			PQclear(res);
			dbDisconnect();
			exit(ERROR_OUTOFMEMORY);
		}

		sprintf_s(id, idLen, "%s", PQgetvalue(res, 0, 0));
		PQclear(res);

		/* Get Tag */
		res = PQexecParams(conn,
				   "SELECT name FROM public.tags WHERE id = $1;",
				   1, NULL, (const char *[]) { id }, NULL, NULL, 0);
		if (PQresultStatus(res) != PGRES_TUPLES_OK) {
			if (verbose)
				printf("%s", PQresultErrorMessage(res));
			PQclear(res);
			dbDisconnect();
			exit(ERROR_OUTOFMEMORY);
		}

		free(tag);
		tag = NULL;
		tagLen = PQgetlength(res, 0, 0) + 1;
		tag = (char *) malloc(tagLen);
		if (tag == NULL) {
			if (verbose)
				printf("blacklistInternal: Error: Out of memory.\n");
			PQclear(res);
			dbDisconnect();
			exit(ERROR_OUTOFMEMORY);
		}

		sprintf_s(tag, tagLen, "%s", PQgetvalue(res, 0, 0));
		free(id);
		id = NULL;
	} else {
		goto NOT_ALIAS;
	}


	/* 3. Update */
NOT_ALIAS:
	if (blacklist) {
		res = PQexecParams(conn,
				   "UPDATE public.tags SET blacklisted = true, updated = now() WHERE name = $1;",
				   1, NULL, (const char *[]) { tag }, NULL, NULL, 0);
	} else {
		res = PQexecParams(conn,
				   "UPDATE public.tags SET blacklisted = false, updated = now() WHERE name = $1;",
				   1, NULL, (const char *[]) { tag }, NULL, NULL, 0);
	}

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		if (verbose)
			printf("%s", PQresultErrorMessage(res));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	/* 4. Terminate */
	PQclear(res);
	dbDisconnect();
	exit(ERROR_SUCCESS);
}


extern void changeTagCategory(const char *tagName, const char *newCat) {
	PGresult *res;


	res = PQexecParams(conn,
			   "UPDATE public.tags SET category_id = (SELECT id FROM public.categories WHERE name = $2) WHERE id = (SELECT id FROM public.tags WHERE name = $1);",
			   2, NULL, (const char *[]) { tagName, newCat }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		if (verbose)
			printf("%s", PQresultErrorMessage(res));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}
}


extern void countTags(void) {
	PGresult *res;

	res = PQexec(conn, "SELECT count(id) FROM public.tags;");
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		if (verbose)
			printf("%s", PQerrorMessage(conn));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	printf("%s\n", PQgetvalue(res, 0, 0));
	PQclear(res);
	dbDisconnect();
	exit(0);
}


extern void dropAlias(char *name) {
	PGresult *res;

	/* 1. Check Input */
	if (name == NULL) {
		if (verbose)
			printf("dropAlias: Error: No alias provided.\n");
	}

	/* 2. Check if Alias Exists */
	if (!checkExists("aliases", "alias", name)) {
		if (verbose)
			printf("dropAlias: Error: Alias already exists.\n");
		dbDisconnect();
		exit(ERROR_FILE_EXISTS);
	}

	/* 3. Create Alias */
	res = PQexecParams(conn,
			   "DELETE FROM public.aliases WHERE alias = $1;",
			   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		if (verbose)
			printf("%s", PQresultErrorMessage(res));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	exit(0);
}


extern void dropImplication(char *implicator, char *implication) {
	PGresult *res;
	char *implicationID;
	size_t implicationIDlen;
	char *implicatorID;
	size_t implicatorIDlen;

	/* 1. Check if Tag Exists */
	if (!checkExists("tags", "name", implicator)) {
		if (verbose)
			printf("addImplication: Error: No such tag, '%s'\n", implicator);
		return;
	}

	/* 2. Check if Implcation Exists */
	if (!checkExists("tags", "name", implication)) {
		if (verbose)
			printf("addImplication: Error: No such tag, '%s'\n", implication);
		return;
	}


	/* 3. Get Tag IDs */
	getTagIDbyName(&implicatorID, implicator, &implicatorIDlen);
	getTagIDbyName(&implicationID, implication, &implicationIDlen);


	/* 4. Check if Bridge Exists */
	res = PQexecParams(conn,
			   "SELECT exists(SELECT * FROM public.implications WHERE implicator = $1 AND implication = $2);",
			   2, NULL, (const char *[]) { implicatorID, implicationID }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		if (verbose)
			printf("%s", PQresultErrorMessage(res));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	if (PQgetvalue(res, 0, 0)[0] == 'f') {
		if (verbose)
			printf("addImplication: Error: Implication does not exist.\n");
		PQclear(res);
		return;
	}

	PQclear(res);


	/* 5. Create Relation */
	res = PQexecParams(conn,
			   "DELETE FROM public.implications WHERE implicator = $1 AND implcation = $2;",
			   2, NULL, (const char *[]) { implicatorID, implicationID }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		if (verbose)
			printf("%s", PQresultErrorMessage(res));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	if (verbose)
		printf("Dropped implication: '%s' + '%s'", implication, implicator);
	PQclear(res);
	free(implicationID);
	free(implicatorID);
}


extern void dropImplicationCmd(void) {
	char *implicator;
	char *implication;

	implicator = getFlagArg();
	implication = getFlagArg();

	if (!implicator || !implication) {
		printf("/add-implication: Error: Missing arguments.\n");
		term(ERROR_BAD_ARGUMENTS);
	} // End of If

	dropImplication(implicator, implication);

	free(implication);
	free(implicator);
	implicator = NULL;
	implication = NULL;
} // End of Function


extern bool dropTagFromImage(char *imageID, char *tagID) {
	PGresult *res;

	/* 1. Check if Exists in images_tags */
	res = PQexecParams(conn,
			   "SELECT exists(SELECT * FROM public.images_tags WHERE image_id = $1 AND tag_id = $2);",
			   2, NULL, (const char *[]) { imageID, tagID }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		if (verbose)
			printf("%s", PQresultErrorMessage(res));
		PQclear(res);
		return false;
	}

	if (PQntuples(res) == 0) {
		if (verbose)
			printf("dropImageFromTag: Error: No such relation.\n");
		PQclear(res);
		return false;
	}

	PQclear(res);


	/* 2. Delete from images_tags */
	res = PQexecParams(conn,
			   "DELETE FROM public.images_tags WHERE image_id = $1 AND tag_id = $2;",
			   2, NULL, (const char *[]) { imageID, tagID }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		if (verbose)
			printf("%s", PQresultErrorMessage(res));
		PQclear(res);
		return false;
	}

	if (verbose)
		printf("Dropped tag from image.\n");

	PQclear(res);
	if (!update("images", imageID))
		return false;
	else
		return true;
}


extern void dropTag(char *name) {
	PGresult *res;


	/* 1. Check if Alias */
	if (checkExists("aliases", "alias", name)) {
		char *id;
		int idLen;

		res = PQexecParams(conn,
				   "SELECT tag_id FROM public.aliases WHERE alias = $1;",
				   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
		if (PQresultStatus(res) != PGRES_TUPLES_OK) {
			if (verbose)
				printf("%s", PQresultErrorMessage(res));
			PQclear(res);
			dbDisconnect();
			exit(ERROR_DATABASE_FAILURE);
		}

		if (!PQntuples(res))
			goto DROP_TAG_NOT_ALIAS;

		/* Copy ID */
		idLen = PQgetlength(res, 0, 0) + 1;
		id = (char *) malloc(idLen);
		if (id == NULL) {
			PQclear(res);
			dbDisconnect();
			exit(ERROR_OUTOFMEMORY);
		}

		sprintf_s(id, idLen, "%s", PQgetvalue(res, 0, 0));
		free(id);
		id = NULL;
		goto DROP_TAG_IS_ALIAS;
	}


	/* 2. Check if Tag Exists */
DROP_TAG_NOT_ALIAS:
	if (!checkExists("tags", "name", name)) {
		if (verbose)
			printf("dropTag: Error: No such tag.\n");
		dbDisconnect();
		exit(ERROR_NOT_FOUND);
	}


	/* 3. Delete tag */
DROP_TAG_IS_ALIAS:
	res = PQexecParams(conn,
			   "DELETE FROM public.tags WHERE name = $1;",
			   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
	checkPqError(res);

	PQclear(res);
	dbDisconnect();
	exit(0);
}


extern void editTagCmd(void) {
	char *editTagArgs[] = {
		"rename", "r",  // 0,1
		"category", "c" // 2,3
	};
	char *tagName;


	tagName = getFlagArg();
	if (!checkExists("tags", "name", tagName)) {
		printf("/edit-tag: Error: No such tag.\n");
		term(ERROR_NOT_FOUND);
	} // End of If


	/* Parse Sub-Arguments */
	curArg++;
	while ((flag = argParse((*argv_)[curArg], editTagArgs, arrayLen(editTagArgs))) != - 1) {
		switch (flag) {
			/* Rename */
			case 0:
			case 1:
				renameTag(tagName, getFlagArg());
				break;

			/* Change Categories */
			case 2:
			case 3:
				changeTagCategory(tagName, getFlagArg());
				break;
		} // End of Switch

		curArg++;
	} // End of While


	term(ERROR_SUCCESS);
} // End of Function


// This function is inefficient as fuck, but it werks
extern void getAliasTarget(char **tag, char *name) {
	PGresult *res;
	char *id;
	size_t idLen;

	/* 0. Validate Input */
	if (!name) {
		printf("GetAliasTarget: Error: No alias supplied.\n");
		term(ERROR_BAD_ARGUMENTS);
	} // End of If


	/* 1. CHeck if a tag with same name exists */
	if (checkExists("tags", "name", name)) {
		printf("getAliasTarget: Error: Not an alias.\n");
		term(ERROR_FILE_EXISTS);
	} // End of If


	/* 2. CHeck */
	if (!isAlias(name)) {
		printf("getAliasTarget: Error: No such alias.\n");
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* 3. Get alias target ID */
	res = PQexecParams(conn,
			   "SELECT tag_id FROM public.aliases WHERE alias = $1;",
			   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "getAliasTarget", false))
		term(ERROR_DATABASE_FAILURE);


	idLen = PQgetlength(res, 0, 0) + 1;
	id = (char *) malloc(idLen);
	checkNull(id, "getAliasTarget");

	sprintf_s(id, idLen, "%s", PQgetvalue(res, 0, 0));
	PQclear(res);


	/* 3. Get Target Tag */
	res = PQexecParams(conn,
			   "SELECT name FROM public.tags WHERE id = $1;",
			   1, NULL, (const char *[]) { id }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "getAliasTarget", false))
		term(ERROR_DATABASE_FAILURE);

	idLen = PQgetlength(res, 0, 0) + 1;
	*tag = (char *) malloc(idLen);
	checkNull(*tag, "getAliasTarget");

	sprintf_s(*tag, idLen, "%s", PQgetvalue(res, 0, 0));
	PQclear(res);
} // End of Function



extern void getTagIDbyName(char **id, const char *name, size_t *len) {
	PGresult *res;


	res = PQexecParams(conn,
			   "SELECT id FROM public.tags WHERE name = lower($1);",
			   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "getTagIdByName", false))
		term(ERROR_DATABASE_FAILURE);

	if (PQntuples(res) > 0) {
		/* Tag Exists */
		*len = PQgetlength(res, 0, 0);
		(*len)++;
		*id = (char *) malloc(*len);

		if (*id == NULL) {
			*len = 0;

			PQclear(res);
			return;
		}

		sprintf_s(*id, *len, "%s", PQgetvalue(res, 0, 0));
		PQclear(res);
		return;
	} else {
		/* Check If Alias Exists */
		PQclear(res);
		res = PQexecParams(conn,
				   "SELECT tag_id FROM public.aliases WHERE alias = lower($1);",
				   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
		if (PQresultStatus(res) != PGRES_TUPLES_OK) {
			if (verbose)
				printf("%s", PQresultErrorMessage(res));
			PQclear(res);
			dbDisconnect();
			exit(ERROR_DATABASE_FAILURE);
		}

		if (PQntuples(res) > 0) {
			/* Alias Exists */
			*len = PQgetlength(res, 0, 0);
			(*len)++;
			*id = (char *) malloc(*len);
			if (*id == NULL) {
				*id = NULL;
				*len = 0;

				PQclear(res);
				return;
			}

			sprintf_s(*id, *len, "%s", PQgetvalue(res, 0, 0));
			PQclear(res);
			return;
		} else {
			PQclear(res);
			return;
		}
	}
}


extern void getTagImplications(struct tagNode *inHead) {
	struct tagNode *inCurNode;
	struct tagNode *outHead;
	struct tagNode *outCurNode;
	PGresult *res;


	if (!inHead) {
		return;
	} // End of If


	inCurNode = inHead;
	/* Allocate Head Node Or Current */
	outHead = (struct tagNode *) malloc(sizeof(struct tagNode));
	if (!outHead) {
		printf("getTagImplications2: Error: Out of memory.\n");
		term(ERROR_OUTOFMEMORY);
	} // End of If

	outCurNode = outHead;


	/* Parse Input List */
	while (inCurNode != NULL) {
		int lim;

		res = PQexecParams(conn,
				   "SELECT tags.name AS \"Name\" FROM recursive_implications((SELECT id FROM public.tags WHERE name = $1)) AS ri INNER JOIN public.tags AS tags ON ri = tags.id;",
				   1, NULL, (const char *[]) { inCurNode->name }, NULL, NULL, 0);
		if (PQresultStatus(res) != PGRES_TUPLES_OK) {
			printf("%s", PQresultErrorMessage(res));
			PQclear(res);
			dbDisconnect();
			exit(ERROR_DATABASE_FAILURE);
		} // End of If


		/* Add Results to Output List */
		lim = PQntuples(res);
		for (int i = 0; i < lim; i++) {
			int nameLen;


			//		printf("i: %d\n", i);
			nameLen = PQgetlength(res, i, 0) + 1;
			outCurNode->name = (char *) malloc(nameLen);
			if (!outCurNode->name) {
				printf("getTagImplications: Error: Out of memory.\n");
				dbDisconnect();
				exit(ERROR_OUTOFMEMORY);
			} // End of If

			sprintf_s(outCurNode->name, nameLen, "%s", PQgetvalue(res, i, 0));
			//	printf("'%s'\n", outCurNode->name);


			//	printf("Limit: %d\nCurrent: %d\n", lim, i);
			if (i + 1 < lim) {
				outCurNode->next = (struct tagNode *) malloc(sizeof(struct tagNode));
				if (!outCurNode) {
					printf("getTagImplications: Error: Out of memory.\n");
					dbDisconnect();
					exit(ERROR_OUTOFMEMORY);
				} // End of If

				outCurNode = outHead->next;
			} else {
				outCurNode->next = NULL;
			} // End of If/Else
		}         // End of For


//		printf("==========\n");
		inCurNode = inCurNode->next;
	} // End of While


//	inCurNode = inHead;
//	while (inCurNode->next)
//		inCurNode = inCurNode->next;


	/* Append List */
	outCurNode = outHead;
	while (outCurNode) {
		size_t nameLen;

		inCurNode->next = (struct tagNode *) malloc(sizeof(struct tagNode));
		if (inCurNode->next == NULL) {
			printf("getTagImplications: Error: Out of memory.\n");
			term(ERROR_OUTOFMEMORY);
		} // End of If

		inCurNode = inCurNode->next;
		nameLen = strlen(outCurNode->name) + 1;
		inCurNode->name = (char *) malloc(nameLen);
		if (inCurNode->name == NULL) {
			printf("getTagImplications: Error: Out of memory.\n");
			term(ERROR_OUTOFMEMORY);
		} // End of If

		sprintf_s(inCurNode->name, nameLen, "%s", outCurNode->name);
//		printf("'%s'\n", inCurNode->name);
		outCurNode = outCurNode->next;
	} // End of While


	inCurNode->next = NULL;
//	printf("---------- To Main ----------\n");
} // End of Function


extern bool isAlias(char *name) {
	PGresult *res;

	res = PQexecParams(conn,
			   "SELECT exists(SELECT tag_id FROM public.aliases WHERE alias = $1);",
			   1, NULL, (const char *[]) { name }, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		if (verbose)
			printf("%s", PQresultErrorMessage(res));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}

	if (PQgetvalue(res, 0, 0)[0] == 't') {
		PQclear(res);
		return true;
	} else {
		PQclear(res);
		return false;
	}
}


// TODO: Enable order by and asc/desc
extern void listTags(char *category) {
	PGresult *res;
	size_t longestCatLen;
	size_t longestTagLen;
	int i;


	/* Initialization */
	longestCatLen = getLongestTupleLength("categories", "name");
	longestTagLen = getLongestTupleLength("tags", "name");


	/* Select Data */
	if (category != NULL) {
		res = PQexecParams(conn,
				   "SELECT tags.name AS tag, categories.name AS category FROM public.tags AS tags JOIN public.tags_categories AS tc ON tags.id = tc.tag_id INNER JOIN public.categories AS categories ON categories.id = tc.category_id WHERE categories.name = $1;",
				   2, NULL, (const char *[]) { category }, NULL, NULL, 0);
	} else {
		res = PQexec(
			conn,
			"SELECT tags.name, categories.name FROM public.tags AS tags JOIN public.tags_categories AS tc ON tags.id = tc.tag_id INNER JOIN public.categories AS categories ON categories.id = tc.category_id ORDER BY tags.name ASC;");
	}

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		if (verbose)
			printf("%s", PQerrorMessage(conn));
		PQclear(res);
		dbDisconnect();
		exit(ERROR_DATABASE_FAILURE);
	}


	/* Print Data */
	if (tabulate) {
		printRowSegment("Tags", longestTagLen, 1, true);
		printRowSegment("Categories", longestCatLen, 2, true);

		for (i = 0; i < longestTagLen + 2; i++)
			printf("-");
		printf("+");

		for (i = 0; i < longestCatLen + 3; i++)
			printf("-");
		printf("\n");

		for (i = 0; i < PQntuples(res); i++) {
			printRowSegment(PQgetvalue(res, i, 0), longestTagLen, 1, false);
			printRowSegment(PQgetvalue(res, i, 1), longestCatLen, 2, false);
		}

		printf("(%d rows)\n", PQntuples(res));
	} else {
		printf("Tags, Categories\n");
		for (i = 0; i < PQntuples(res); i++) {
			printf("%s, ", PQgetvalue(res, i, 0));
			printf("%s\n", PQgetvalue(res, i, 1));
		}
	}


	dbDisconnect();
	exit(0);
}


extern void renameTag(char *name, char *newName) {
	PGresult *res;

	res = PQexecParams(conn,
			   "UPDATE public.tags SET name = $2 WHERE name = $1;",
			   2, NULL, (const char *[]) { name, newName }, NULL, NULL, 0);
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
