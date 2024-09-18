#include "idb.h"

#include <Windows.h>
#include <winerror.h>

#include <Shlwapi.h>
#include <libpq-fe.h>
#include <sha-256.h>
#include <stdio.h>

#pragma comment(lib, "shlwapi.lib")


extern bool addImgCommentary(char *id, char lang[3], const char *comm) {
	PGresult *res;

	/* Validate Commentary */
	if (!comm) {
		printf("addImgCommentary: Error: No commentary provided.\n");
		return false;
	} // End of If

	/* Validate Language */
	if (!checkExists("languages", "code", lang)) {
		printf("addImgCommentary: Error: No such language code.\n");
		return false;
	} // End of If


	/* Check if Commentary Exists */
	res = PQexecParams(conn,
			   "SELECT exists(SELECT image_id, lang FROM public.commentary WHERE image_id = $1 AND lang = $2);",
			   2, NULL, (const char *[]) { id, lang }, NULL, NULL, 0);

	if (!checkTuplesOk(&res, "addImgCommentary", false))
		return false;

	if (PQgetvalue(res, 0, 0)[0] == 't') {
		if (verbose)
			printf("addImgCommentary: Error: Commentary already exists.\n");
		PQclear(res);
		return false;
	} else {
		PQclear(res);
	} // End of If/Else


	/* Add Commentary */
	res = PQexecParams(conn,
			   "INSERT INTO public.commentary (image_id, lang, data) VALUES ($1, $2, $3) ON CONFLICT DO NOTHING;",
			   3, NULL, (const char *[]) { id, lang, comm }, NULL, NULL, 0);

	if (checkCmdOk(res, "addImgCommentary", true))
		term(ERROR_DATABASE_FAILURE);

	if (update("images", id))
		return true;
	else
		return false;
} // End of Function

extern void addImgSrc(char *id, const char *src) {
	PGresult *res;

	/* 0. Validate Source */
	if (!src) {
		printf("addImgSrc: Error: src is NULL.\n");
		return; // Shouldn't this terminate the program?
	}               // End of If

	/* 1. Check if exists */
	if (!checkExists("images", "id", id)) {
		printf("addImgSrc: Error: No such image.\n");
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* 2. Check if has src*/
	if (checkExists("images_source", "image_id", id)) {
		printf("addImgSrc: Error: Source already exists.\n");
		term(ERROR_FILE_EXISTS);
	} // End of If

	/* 3. Add src */
	res = PQexecParams(conn,
			   "INSERT INTO public.images_source (image_id, source) VALUES ($1, $20);",
			   2, NULL, (const char *[]) { id, src }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "addImgSrc", false)) {
		term(ERROR_DATABASE_FAILURE);
	} else {
		PQclear(res);
		term(ERROR_SUCCESS);
	} // End of If/Else
} // End of Function

extern void addImageToPool(char *pool, const char *imageID, const bool drop) {
	char *poolID;
	int poolIDlen;
	PGresult *res;


	poolID = NULL;


	/* 1. Check if Image Exists */
	if (!checkExists("images", "id", imageID)) {
		printf("addImageToPool: Error: Image '%s' does not exist.\n", imageID);
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* 2. Check if Pool Exists */
	res = PQexecParams(conn,
			   "SELECT id FROM public.pools WHERE name = $1;",
			   1, NULL, (const char *[]) { pool }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "addImageToPool", false))
		term(ERROR_DATABASE_FAILURE);

	if (PQntuples(res) == 0) {
		printf("addImageToPool: Error: Pool '%s' does not exist.\n", pool);
		PQclear(res);
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	poolIDlen = PQgetlength(res, 0, 0) + 1;
	poolID = (char *) malloc(poolIDlen);
	if (!poolID) {
		printf("addImageToPool: Error: Out of memory.\n");
		PQclear(res);
		term(ERROR_OUTOFMEMORY);
	} // End of If

	sprintf_s(poolID, poolIDlen, "%s", PQgetvalue(res, 0, 0));
	PQclear(res);


	/* 3.  Check if Image is in Pool and Get ID */
	res = PQexecParams(conn,
			   "SELECT exists(SELECT image_id, pool_id FROM public.images_pools WHERE image_id = $1 AND pool_id = $2);",
			   2, NULL, (const char *[]) { imageID, poolID }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "addImageToPool", false))
		term(ERROR_DATABASE_FAILURE);


	/* WHat is the point of this code? */
	if (PQgetvalue(res, 0, 0)[0] == 't') {
		/* Case: exists and don't drop */
		if (!drop) {
			printf("addImageToPool: Error: Image is already in pool.\n");
			PQclear(res);
			dbDisconnect();
			exit(ERROR_FILE_EXISTS);
		} // End of If
	} else {
		/* Case: does not exist and drop */
		if (drop) {
			printf("addImageToPool: Error: Image is not in pool.\n");
			PQclear(res);
			dbDisconnect();
			exit(ERROR_FILE_NOT_FOUND);
		} // End of If
	}         // End of If/Else

	PQclear(res);


	/* 5. Add Image to Pool */
	if (drop) {
		res = PQexecParams(conn,
				   "DELETE FROM public.images_pools WHERE image_id = $1 AND pool_id = $2;",
				   2, NULL, (const char *[]) { imageID, poolID }, NULL, NULL, 0);
	} else {
		res = PQexecParams(conn,
				   "INSERT INTO public.images_pools (image_id, pool_id) VALUES ($1, $2);",
				   2, NULL, (const char *[]) { imageID, poolID }, NULL, NULL, 0);
	} // End of If/Else

	if (!checkCmdOk(res, "addImageToPool", false)) {
		term(ERROR_DATABASE_FAILURE);
	} else {
		PQclear(res);
		term(ERROR_SUCCESS);
	} // End of IF/Else
} // End of Function

extern void addToPoolCmd(bool drop) {
	char *poolName;
	char *imageID;

	poolName = getFlagArg();
	imageID = getFlagArg();

	addImageToPool(poolName, imageID, drop);
} // End of Function

extern void countImages(void) {
	PGresult *res;

	res = PQexec(conn, "SELECT count(id) FROM public.images;");
	if (checkTuplesOk(&res, "countImages", false)) {
		printf("%s\n", PQgetvalue(res, 0, 0));
		term(ERROR_SUCCESS);
	} else {
		term(ERROR_DATABASE_FAILURE);
	} // End of If/Else
} // End of Function

extern void dropImgCmd(void) {
	assertFlagArgProvided();
	dropImage(getFlagArg());
} // End of Function

extern void editImgCmd(void) {
	char *editImgArgs[] = {
		"add-tag", "at",	 // 0,1
		"drop-tag", "dt",	// 2,3
		"rating", "r,",	  // 4,5
		"add-commentary", "ac",  // 6,7
		"drop-commentary", "dc", // 8,9
		"edit-commentary", "e",  // 10,11
		"set-master", "sm",      // 12,13
		"get-master", "gm",      // 14,15
		"get-slaves", "gs",      // 16,17
		"unset-master", "um",    // 18,19
	};
	char *imageId;
	PGresult *res;


	imageId = getFlagArg();
	if (!checkExists("images", "id", imageId)) {
		printf("/edit-image: Error: No such image.\n");
		term(ERROR_FILE_NOT_FOUND);
	} // End of if


	curArg++;
	while ((flag = argParse((*argv_)[curArg], editImgArgs, arrayLen(editImgArgs))) != - 1) {
		switch (flag) {
			/* Add Tag */
			case 0:
			case 1: {
				char *tagID;
				size_t tagIDlen;
				char *tagName;

				begin(&res);

				/* 1. Get Input */
				tagName = getFlagArg();
				getTagIDbyName(&tagID, tagName, &tagIDlen);
				if (!tagID) {
					printf("/add-tag: Error: No such tag.\n");
					rollback(&res);
					PQclear(res);
					term(ERROR_NOT_FOUND);
				} // End of If

				if (!addTagToImage(imageId, tagID)) {
					rollback(&res);
					PQclear(res);
				} else {
					commit(&res);
				} // End of If/Else

				free(tagID);
				free(tagName);
				tagID = NULL;
				tagName = NULL;
				curArg++;
				break;
			} // End of Case

			/* Drop Tag From Image */
			case 2:
			case 3: {
				struct tagNode *head;
				char *tagID;
				int tagIDlen;


				/* Setup */
				head = (struct tagNode *) malloc(sizeof(struct tagNode));
				if (!head) {
					printf("drop-tag: Error: Out of memory.\n");
					term(ERROR_OUTOFMEMORY);
				} // End of If

				head->name = getFlagArg();
				if (checkExists("tags", "name", head->name)) {
					printf("/drop-tag: Error: No such tag.\n");
					rollback(&res);
					term(ERROR_NOT_FOUND);
				} // End of If

				head->next = NULL;
				getTagImplications(head);


				/* Tag Stuff */
				begin(&res);

				while (head) {
					getTagIDbyName(&tagID, head->name, &tagIDlen);
					if (tagID == NULL)
						goto EDIT_TAG__DROP_TAG_NULL;

					if (dropTagFromImage(imageId, tagID)) {
						printf("drop-tag-from-image: Error: Rollingback transaction.\n");
						rollback(&res);
						PQclear(res);
						break;
					} // End of If


					free(tagID);
					tagID = NULL;
				EDIT_TAG__DROP_TAG_NULL:
					head = head->next;
				} // End of While

				curArg++;
				break;
			} // End of Case

			/* Rating */
			case 4:
			case 5:
				setImageRating(imageId, getFlagArg());
				curArg++;
				break;

			/* Add Commentary */
			case 6:
			case 7: {
				char *lang;
				char *comm;

				lang = getFlagArg();
				comm = getFlagArg();
				if (strlen(lang) > 2) {
					printf("/add-commentary: Error: Invalid language code.\n");
					term(ERROR_BAD_ARGUMENTS);
				} // End of If

				addImgCommentary(imageId, lang, comm);
				free(lang);
				free(comm);
				lang = NULL;
				comm = NULL;

				break;
			} // End of Case

			/* Drop Commentary */
			case 8:
			case 9:
				dropCommentary(imageId, getFlagArg());
				curArg++;
				break;

			/* Edit Commentary */
			case 10:
			case 11: {
				char *lang;
				char *newComm;

				lang = getFlagArg();
				if (strlen(lang) > 2) {
					printf("editCommentary: Error: Invalid language code.\n");
					free(lang);
					lang = NULL;
					break;
				} // End of If

				newComm = getFlagArg();
				editCommentary(imageId, lang, newComm);
				free(lang);
				free(newComm);

				lang = NULL;
				newComm = NULL;
				break;
			} // End of Case

			/* Set Master */
			case 12:
			case 13: {
				char *masterId;
				char *index;


				masterId = getFlagArg();
				index = getFlagArg();

				setImageMaster(masterId, imageId, index);
				// TODO: Free index
				break;
			} // End of Case

			/* Get Master */
			case 14:
			case 15:
				getImageMaster(imageId);
				break;

			/* Get Slaves */
			case 16:
			case 17:
				getImageSlaves(imageId);
				break;

			/* Unset Master */
			case 18:
			case 19:
				unsetImageMaster(imageId);
				break;

			/* Set Source */
			case 22:
			case 23:
				addImgSrc(imageId, getFlagArg());
				break;

			/* Edit Source */
			case 24:
			case 25:
				editImgSrc(imageId, getFlagArg());
				break;

			/* Drop Source */
			case 26:
			case 27:
				dropImgSrc(imageId);
				break;

			default:
				if (verbose)
					printf("ERROR: invalid argumuent\n");
				break;
		} // End of Switch
	} // End of While

	term(ERROR_SUCCESS);
} // End of Function

extern void dropImgSrc(char *id) {
	PGresult *res;

	/* 1. Check if image exists */
	if (!checkExists("images", "id", id)) {
		printf("dropImgSrc: Error: No such image.\n");
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* 2. Check if source exists */
	if (!checkExists("images_source", "image_id", id)) {
		printf("dropImgSrc: Error: No such source.\n");
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* 3. Drop source */
	res = PQexecParams(conn,
			   "DELETE FROM public.images_source WHERE image_id = $1;",
			   1, NULL, (const char *[]) { id }, NULL, NULL, 0);
	if (!checkCmdOk(res, "dropImgSrc", false))
		term(ERROR_DATABASE_FAILURE);

	PQclear(res);
	term(ERROR_SUCCESS);
} // End of Function

extern void editImgSrc(char *id, const char *src) {
	PGresult *res;


	/* 0. Validate Source */
	if (!src) {
		printf("editImgSrc: Error: src is NULL.\n");
		return;
	} // End of If


	/* 1. Check if exists */
	if (!checkExists("images", "id", id)) {
		printf("editImgSrc: Error: No such image.\n");
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* 2. Check if has src*/
	if (checkExists("images_source", "image_id", id)) {
		printf("editImgSrc: Error: Source already exists.\n");
		term(ERROR_FILE_EXISTS);
	} // End of If


	/* 3. Add src */
	res = PQexecParams(conn,
			   "UPDATE public.images_source SET source = $2 WHERE image_id = $1;",
			   2, NULL, (const char *[]) { id, src }, NULL, NULL, 0);
	if (checkCmdOk(res, "editImgSrc", false)) {
		PQclear(res);
		term(ERROR_SUCCESS);
	} else {
		term(ERROR_DATABASE_FAILURE);
	} // End of If/Else
} // End of Function

extern void exportCmd(void) {
	char *id;
	char *path;

	assertFlagArgProvided();
	id = getFlagArg();
	path = getFlagArg();

	exportImage(id, path);
} // End of Function

static void getLang(char *dst[3]) {
	char *lang;


	lang = getFlagArg();

	// We assume strlen excludes the trailing NULL.
	if (strlen(lang) != 2) {
		printf("getLang: Error: Invalid language code. See IETF 1766/ISO 639 for more details.\n");
		term(ERROR_BAD_ARGUMENTS);
	} // End of If


	*dst[0] = tolower(lang[0]);
	*dst[1] = tolower(lang[1]);
	*dst[2] = '\0';


	free(lang);
	lang = NULL;
} // End of Function

extern void importCmd(void) {
	bool copy;
	int importFlag;
	char *path;
	size_t fileLen;
	struct tagNode *tagCurr;
	struct tagNode *tagRoot;
	struct comNode *comCurr;
	struct comNode *comRoot;
	char *imgImportArgs[] = {
		"tag", "t",
		"commentary", "c",
		"copy", "y",
	};
	char *rating;


	printf("IMPORT\n");
	assertFlagArgProvided();
	path = getFlagArg();

	copy = false;
	comRoot = NULL;
	tagRoot = NULL;
	comCurr = NULL;
	tagCurr = NULL;
	rating = NULL;
	curArg++;


	if (!path) {
		usage();
		term(ERROR_BAD_ARGUMENTS);
	} // End of If


	while ((importFlag = argParse((*argv_)[curArg], imgImportArgs, arrayLen(imgImportArgs))) != -1) {
//		printf("Flag: '%s'\n", (*argv_)[curArg]);
		switch (importFlag) {
			/* Tags */
			case 0: // Takes 1 argument
			case 1: {
				// FIXME: This code is a complete disaster
//				printf("AND\n");
				if (!tagRoot) {		// tagRoot is undefined, so we must do this.
					tagRoot = (struct tagNode *) malloc(sizeof(struct tagNode));
					if (!tagRoot) {
						printf("importCmd: Error: Out of memory.\n");
						term(ERROR_OUTOFMEMORY);
					} // End of If

					tagCurr = tagRoot;
					tagCurr->name = getFlagArg();
					tagCurr->next = NULL;
				} else {
					tagCurr->next = (struct tagNode *) malloc(sizeof(struct tagNode));
					if (!tagCurr) {
						printf("importCmd: Error: Out of memory.\n");
						term(ERROR_OUTOFMEMORY);
					} // End of If

					tagCurr->name = getFlagArg();
					tagCurr->next = NULL;
				} // End of If/Else

				printf("\n");
				break;
			} // End of Case


			/* Commentary */
			case 2: // Takes 2 arguments
			case 3: {
				/* Add to the list if tags is NULL; else, add to the end. */
				// TODO: Write function to get language
				if (!comRoot) {


					comRoot = (struct comNode *) malloc(sizeof(struct comNode));
					if (!comRoot) {
						printf("importCmd: Error: Out of memory.\n");
						term(ERROR_OUTOFMEMORY);
					} // End of If


					comCurr = comRoot;
					getLang(comCurr->lang);


					comCurr->dat = getFlagArg();
					comCurr->next = NULL;
					curArg++;
				} else {
					// FIXME: comm->lang is off by one?
					char *lang;


					/* Get Language */
					lang = getFlagArg();
					if (strlen(lang) != 2) {
						printf("Language code must be 2 letters. See IETF 1766/ISO 639 for more details.");
						term(ERROR_BAD_ARGUMENTS);
					} // End of If


					/* Prepare new node */
					comCurr->next = (struct comNode *) malloc(sizeof(struct comNode));
					comCurr = comCurr->next;
					if (!comCurr) {
						printf("Out of memory\n");
						term(ERROR_OUTOFMEMORY);
					} // End of If


					comCurr->lang[0] = tolower(lang[0]);
					comCurr->lang[1] = tolower(lang[1]);
					comCurr->lang[2] = '\0';

					comCurr->dat = getFlagArg();
					comCurr->next = NULL;
					curArg++;


					free(lang);
					lang = NULL;
				} // End of If/Else

				break;
			} /* End of Case */


			/* Copy */
			case 4:
			case 5:
				/* FIXME: This appears to be causing problems with other flags. */
				copy = true;
//				curArg++;
				break;


			/* Rating */
			case 6:
			case 7:
				rating = getFlagArg();
				break;
		} // End of Switch

		curArg++;
	} // End of while

	if (!rating) {
		rating = (char *) malloc(8);
		checkNull(rating, "importCmd");
		sprintf_s(rating, 8, "general");
	} // End of If


	printf("Copy: %d\n", copy);
	importImage3(path, copy, rating, tagRoot, comRoot);
	term(ERROR_SUCCESS);
} // End of Function

extern void importImage3(const char *imgPath, bool copy, char *rating, struct tagNode *tagsRoot,
			 struct comNode *commRoot) {
	char *ext;
	char *dstPath;
	size_t dstPathLen;
	char *imgId;
	size_t imgIdLen;
	char *imgDat;
	FILE *imgFile;
	size_t imgFileLen;
	PGresult *res;
	char *sha256;
	struct tagNode *curTag;
	struct comNode *curComm;



	/** 0. Setup **/
	curTag = tagsRoot;
	curComm = commRoot;
	printf("Path: '%s'\n", imgPath);


	/* 1. Get File Size */
	if (fopen_s(&imgFile, imgPath, "r")) {
		DWORD ret;

		ret = GetLastError();
		switch (ret) {
			case ERROR_FILE_NOT_FOUND:
				printf("importImage: Error: No such file.\n");
				break;

			default:
				break;
		} // End of Switch

		term(ret);
	} // End of If

	fseek(imgFile, 0L, SEEK_END);
	imgFileLen = ftell(imgFile);
	printf("Size: %lluB\n", imgFileLen);


	/** 2. Load File Into Buffer **/
	imgDat = (char *) malloc(imgFileLen + 1);
	if (!imgDat) {
		printf("importImage: Error: Out of memory.\n");
		term(ERROR_OUTOFMEMORY);
	} // End of If

	fread_s(imgDat, imgFileLen + 1, imgFileLen, 1, imgFile);
	fclose(imgFile);


	/** 3. Get File Extension **/
	// #NOTE We might have a problem if a file lacking an extension is imported.
	 getFileExtension(&ext, imgPath); // TODO: Test this
	 printf("Extension: '%s'\n", ext);


	/** 4. Calculate File SHA256 Sum **/
	getSHA256(&sha256, imgDat, imgFileLen); // 5. Calculate SHA256 sum and copy it into a string
	printf("SHA256: '%s'\n", sha256);


	/** 5. Check if File is in Database **/
	// #NOTE: The SHA256 sums we use contain only LOWERCASE characters.
	printf("Checking image...\t\t");
	// FIXME: This uses the wrong function!
	if (checkFileExists(sha256)) {
		printf("importImage: Error: File has already been imported.\n");
		term(ERROR_FILE_EXISTS);
	} else {
		printf("OK\n");
	} // End of If/Else


	/** 7. Begin Transaction **/
	begin(&res); // 7. Begin transaction


	/** 8. Insert Image (SHA256, rating, extension) **/
	printf("Creating image record...\t");
	res = PQexecParams(conn,
			   "INSERT INTO public.images (sha256, extension) VALUES ($1, $2);",
			   2, NULL, (const char *[]) { sha256, ext },
			   NULL, NULL, 0);
	if (!checkCmdOk(&res, "importImages", true))
		term(ERROR_DATABASE_FAILURE);
	else
		printf("OK\n");
	PQclear(res);


	/** 9. Get Image ID **/
	printf("Getting image ID...\t\t");
	res = PQexecParams(conn,
			   "SELECT id FROM public.images WHERE sha256 = $1;",
			   1, NULL, (const char *[]) { sha256 }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "importImages", true))
		term(ERROR_DATABASE_FAILURE);
	else
		printf("OK\n");


	imgIdLen = PQgetlength(res, 0, 0);
	imgIdLen++;
	imgId = (char *) malloc(imgIdLen);
	checkNull(imgId, "importImages");
	sprintf_s(imgId, imgIdLen, "%s", PQgetvalue(res, 0, 0));
	PQclear(res);


	/** 10. Insert Rating **/
	if (!rating) {
		rating = (char *) malloc(5);
		checkNull(imgPath, "importImage3");
		sprintf_s(rating, 5, "safe");
	} else {
		// Validate rating, and fix it if it's invalid.
		if (!checkExists("ratings", "name", rating)) {
			rating = (char *) malloc(5);
			checkNull(imgPath, "importImage3");
			sprintf_s(rating, 5, "safe");
		} // End of If
	} // End of If/Else

	printf("Assigning rating...\t\t");
	res = PQexecParams(conn,
			   "INSERT INTO public.images_ratings (image_id, rating_id) VALUES ($1, (SELECT id FROM public.ratings WHERE name = $2));",
			   2, NULL, (const char *[]) { imgId, rating }, NULL,
			   NULL, 0);
	if (!checkCmdOk(&res, "importImages", true))
		term(ERROR_DATABASE_FAILURE);
	else
		printf("OK\n");


	/** 11. Insert Tags **/
	if (!curTag) {
		getTagImplications(curTag);
		while (curTag) {
			char *tagId;
			size_t tagIdLen;


			if (!curTag->name) {
				printf("importImage: Warning: Tag name is Null. Setting to 'tagme'.\n");
				curTag->name = (char *) malloc(6);
				checkNull(curTag->name, "importImages");
				sprintf_s(curTag->name, 6, "tagme");
			} // End of If

			getTagIDbyName(&tagId, curTag->name, &tagIdLen);
			checkNull(curTag->name, "importImages");
			if (!addTagToImage(imgId, tagId)) {
				// #NOTE: The error message and rollback are handled by addTagToImage().
				term(ERROR_DATABASE_FAILURE);
			} // End of If

			curTag = curTag->next;
			free(tagId);
			tagId = NULL;
		} // End of While
	} else {
		if (!addTagToImage(imgId, "1")) {
			// #NOTE: The error message and rollback are handled by addTagToImage().
			term(ERROR_DATABASE_FAILURE);
		} // End of If
	} // End of If/Else


	/** 12. Insert Commentary **/
	while (curComm) {
		if (!curComm->dat) {
			printf("importImages: Error: Commentary is NULL.\n");
			goto NULL_COMMENTARY;
		} // End of If

		addImgCommentary(imgId, curComm->lang, curComm->dat);
	NULL_COMMENTARY:
		curComm = curComm->next;
	} // End of While


	/** 13. Generate File Path */
	dstPathLen = strlen(appData[1]) + strlen(sha256) + 3;
	dstPath = (char *) malloc(dstPathLen);
	checkNull(dstPath, "importImage");
	sprintf_s(dstPath, dstPathLen, "%s\\%s", appData[1], sha256);


	/** 14. Move/Copy File **/
	if (copy) {
		if (!CopyFileExA(imgPath, dstPath, NULL, NULL, false, COPY_FILE_ALLOW_DECRYPTED_DESTINATION | COPY_FILE_NO_BUFFERING)) {
			// #TODO: Use FormatMessage() and print
			rollback(&res);
			term(GetLastError());
		} // End of If
	} else {
		if (!MoveFileExA(imgPath, dstPath,
				 MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
			rollback(&res);
			term(GetLastError());
		} // End of If
	} // End of If/Else


	/** 15. Commit Transaction **/
	commit(&res);
} // End of Function

extern void dropCommentary(char *imageID, char *lang) {
	PGresult *res;
	char *commID;
	int commIDlen;


	/* Validate Language */
	if (!checkExists("languages", "code", lang)) {
		printf("dropCommentary: Error: No such language code.\n");
		return;
	} // End of If


	/* Check if image has commentary */
	res = PQexecParams(conn,
			   "SELECT id, image_id, lang FROM public.commentary WHERE image_id = $1 AND lang = $2;",
			   2, NULL, (const char *[]) { imageID, lang }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "dropCommentary", false)) {
		return;
	} // End of If

	if (PQntuples(res)) {
		commIDlen = PQgetlength(res, 0, 0) + 1;
		commID = (char *) malloc(commIDlen);
		if (!commID) {
			printf("dropCommentary: Error: Out of memory.\n");
			PQclear(res);
			term(ERROR_OUTOFMEMORY);
		} // End of If

		sprintf_s(commID, commIDlen, "%s", PQgetvalue(res, 0, 0));
		PQclear(res);
	} else {
		printf("dropCommentary: Error: No such commentary.\n");
		PQclear(res);
		return;
	} // End of If/Else


	/* Begin Transaction */
	begin(&res);


	/* Delete Commentary */
	res = PQexecParams(conn,
			   "DELETE FROM public.commentary WHERE id = $1;",
			   1, NULL, (const char *[]) { commID }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "dropCommentary", false)) {
		return;
	} // End of If


	/* Complete Transaction */
	if (!update("images", imageID)) {
		rollback(&res);
		PQclear(res);
	} else {
		commit(&res);
	} // End of If/Else
} // End of Function

extern void dropImage(const char *id) {
	PGresult *res;

	/* Check if it exists */
	res = PQexecParams(conn,
			   "SELECT exists(SELECT id FROM public.images WHERE id = $1);",
		1, NULL, (const char *[]) { id }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "dropImage", false))
		term(ERROR_DATABASE_FAILURE);

	if (PQgetvalue(res, 0, 0)[0] == 'f') {
		printf("dropImage: Error: No such image.\n");
		PQclear(res);
		term(ERROR_FILE_NOT_FOUND);
	} // End of If

	PQclear(res);

	/* Delete it */
	res = PQexecParams(conn,
			   "DELETE FROM public.images WHERE id = $1;",
		1, NULL, (const char *[]) { id }, NULL, NULL, 0);
	if (!checkCmdOk(res, "dropImage", false))
		term(ERROR_DATABASE_FAILURE);

	PQclear(res);
	term(ERROR_SUCCESS);
} // End of Function

extern void editCommentary(char *imageID, char lang[3], char *comm) {
	PGresult *res;

	/* Validate Language */
	if (!checkExists("languages", "code", lang)) {
		printf("dropCommentary: Error: No such language code.\n");
		return;
	} // End of If

	/* Check if image has commentary */
	res = PQexecParams(conn,
			   "SELECT id, image_id, lang FROM public.commentary WHERE image_id = $1 AND lang = $2;",
			   2, NULL, (const char *[]) { imageID, lang }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "editCommentary", false))
		return;

	if (PQntuples(res) != 0) {
		char *commID;
		int commIDlen;

		commIDlen = PQgetlength(res, 0, 0) + 1;
		commID = (char *) malloc(commIDlen);
		checkNull(commID, "editCommentary");

		sprintf_s(commID, commIDlen, "%s", PQgetvalue(res, 0, 0));
		PQclear(res);
	} else {
		printf("dropCommentary: Error: No such commentary.\n");
		PQclear(res);
		return;
	} // End of If/Else


	/* Update */
	begin(&res);		// Begin Transaction
	res = PQexecParams(conn,
			   "UPDATE public.commentary SET data = $3 WHERE image_id = $1 AND lang = $2;",
			   3, NULL, (const char *[]) { imageID, lang, comm }, NULL, NULL, 0);
	if (!checkCmdOk(res, "editCommentary", false))
		return;


	/* Update */
	PQclear(res);
	if (!update("images", imageID)) {
		rollback(&res);
		PQclear(res);
	} else {
		commit(&res);
	} // End of If
} // End of Function

extern void exportImage(char *id, char *dst) {
	char *ext;
	size_t extLen;
	char *dstPath;
	size_t dstPathLen;
	char *srcPath;
	size_t srcPathLen;
	PGresult *res;
	char *sha256;
	size_t sha256Len;


	ext = NULL;
	if (!id || !dst) {
		printf("Export: Error: File ID and/or destination noth provided.\n");
		term(ERROR_BAD_ARGUMENTS);
	} // End of If

	/* Check if the file exists */
	res = PQexecParams(conn,
			   "SELECT exists(SELECT id FROM public.images WHERE id = $1);",
			   1, NULL, (const char *[]) { id }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "exportImage", false))
		term(ERROR_DATABASE_FAILURE);

	if (PQgetvalue(res, 0, 0)[0] == 'f') {
		printf("exportImage: Error: File does not exist.\n");
		PQclear(res);
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* Get file info */
	PQclear(res);
	res = PQexecParams(conn,
			   "SELECT sha256, extension FROM public.images WHERE id = $1;",
			   1, NULL, (const char *[]) { id }, NULL, NULL, 0);

	sha256Len = PQgetlength(res, 0, 0);
	sha256Len++;
	sha256 = (char *) malloc(sha256Len);
	checkNull(sha256, "exportImage");
	sprintf_s(sha256, sha256Len, "%s", PQgetvalue(res, 0, 0));


	extLen = PQgetlength(res, 0, 1);
	if (extLen > 0) {
		extLen++;
		ext = (char *) malloc(extLen);
		checkNull(ext, "exportImage");
		sprintf_s(ext, extLen, "%s", PQgetvalue(res, 0, 1));
	} // End of If


	/* Generate srcPath */
	PQclear(res);
	srcPathLen = strlen(appData[1]) + sha256Len + 2;
	srcPath = (char *) malloc(srcPathLen);
	checkNull(srcPath, "exportImage");
	sprintf_s(srcPath, srcPathLen, "%s\\%s", appData[1], sha256);


	/* Check if srcPath is valid */
	if (!PathFileExistsA(srcPath)) {
		printf("Export: Error: File does not exist.\n");
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* Generate dstPath */
	if (ext) {
		dstPathLen = strlen(dst) + strlen(ext) + 1;
		dstPath = (char *) malloc(dstPathLen);
		checkNull(dstPath, "exportImage");
		sprintf_s(dstPath, dstPathLen, "%s%s", dst, ext);
	} // End of If


	/* Copy File */
	dstPath = dst; // TODO: idk if this will work
	if (!CopyFileA(srcPath, dstPath, true)) {
		DWORD ret = GetLastError();

		if (verbose) {
			printf("Error code: %lu\n", ret);
			printf("Export: Error: File may already exist.\n");
		} // End of If

		term(ret);
	} // End of If

	term(ERROR_SUCCESS);
} // End of Function

extern void getImageMaster(char *slaveID) {
	PGresult *res;


	/* 1. Check if Image Exists */
	if (!checkExists("images", "id", slaveID)) {
		printf("getImageMaster: Error: No such image, %s\n", slaveID);
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* 2. Check if Image has a Master */
	res = PQexecParams(conn,
			   "SELECT exists(SELECT * FROM public.image_relations WHERE slave_id = $1);",
			   1, NULL, (const char *[]) { slaveID }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "getImageMaster", false))
		term(ERROR_DATABASE_FAILURE);

	if (PQgetvalue(res, 0, 0)[0] == 'f') {
		printf("getImageMaster: Error: Image has no master relation.\n");
		PQclear(res);
		term(ERROR_NOT_FOUND);
	} // End of If


	/* 3. Select Data */
	PQclear(res);
	res = PQexecParams(conn,
			   "SELECT master_id FROM public.image_relations WHERE slave_id = $1;",
			   1, NULL, (const char *[]) { slaveID }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "getImageMaster", false))
		term(ERROR_DATABASE_FAILURE);


	/* 4. Print Output */
	printf("%s\n", PQgetvalue(res, 0, 0));
	PQclear(res);
	term(ERROR_SUCCESS);
} // End of Function

extern void getImageSlaves(char *masterID) {
	PGresult *res;
	int count;


	/* 1. Check if Image Exists */
	if (!checkExists("images", "id", masterID)) {
		printf("getImageSlaves: Error: No such image.\n");
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* 2. Check if has Slaves */
	res = PQexecParams(conn,
			   "SELECT slave_id FROM public.image_relations WHERE master_id = $1;",
			   1, NULL, (const char *[]) { masterID }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "getImageSlaves", false))
		term(ERROR_DATABASE_FAILURE);

	count = PQntuples(res);
	if (!count) {
		printf("getImageMaster: Error: Image has no slave relation(s).\n");
		PQclear(res);
		term(ERROR_SUCCESS);
	} // End of If


	/* 3.Print */
	for (int i = 0; i < PQntuples(res); i++)
		printf("%s\n", PQgetvalue(res, 0, 0));


	PQclear(res);
	term(ERROR_SUCCESS);
} // End of Function

extern void listImageTags(const char *id) {
	int longestTagLen = 0;
	PGresult *res;


	if (!id) {
		printf("listImageTags: Error: No argument.\n");
		term(ERROR_BAD_ARGUMENTS);
	} // End of If

	/* Initialization */
	longestTagLen = getLongestTupleLength("tags", "name");

	/* Select */
	res = PQexecParams(conn,
			   "SELECT i.id AS \"Image ID\", t.name AS \"Tags\" FROM public.images AS i JOIN public.images_tags AS it ON i.id = it.image_id INNER JOIN public.tags AS t ON it.tag_id = t.id WHERE i.id = $1 ORDER BY \"Tags\" ASC;",
			   1, NULL, (const char *[]) { id }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "listImageTags", false))
		term(ERROR_DATABASE_FAILURE);


	/* Print */
	if (tabulate) {
		printRowSegment("Image ID", 8, 1, true);
		printRowSegment("Tags", longestTagLen, 2, true);

		for (int i = 0; i < 8 + 2; i++)
			printf("-");
		printf("+");

		for (int i = 0; i < longestTagLen + 3; i++)
			printf("-");
		printf("\n");

		for (int i = 0; i < PQntuples(res); i++) {
			printRowSegment(PQgetvalue(res, i, 0), 8, 1, false);
			printRowSegment(PQgetvalue(res, i, 1), longestTagLen, 2, false);
		} // End of If

		printf("(%d rows)\n", PQntuples(res));
	} else {
		printf("Image ID, Tags\n");
		for (int i = 0; i < PQntuples(res); i++) {
			printf("%s, ", PQgetvalue(res, i, 0));
			printf("%s\n", PQgetvalue(res, i, 1));
		} // End of If
	} // End of If/Else


	PQclear(res);
	term(ERROR_SUCCESS);
} // End of Function

extern void setImageMaster(const char *masterID, const char *slaveID, const char *index) {
	PGresult *res;


	// Check if master exists
	// check if slave exists
	// check if index is not null
	printf("Master: %s\n", masterID);
	printf("Slave: %s\n", slaveID);
	printf("Index: %s\n", index);


	/* 1. Check if Master Exists */
	if (!checkExists("images", "id", masterID)) {
		printf("setImageMaster: Error: Master does not extist.\n");
		term(ERROR_NOT_FOUND);
	} // End of If


	/* 2. Check if Slave Exists */
	if (!checkExists("images", "id", slaveID)) {
		printf("setImageMaster: Error: Slave does not extist.\n");
		term(ERROR_NOT_FOUND);
	} // End of If


	/* 3. Validate Index */
	if (!index) {
		printf("setImageMaster: Error: NULL index.\n");
		term(ERROR_BAD_ARGUMENTS);
	} // End of If

	if (atol(index) <= 0) {
		printf("setImageMaster: Error: Slave index must be greater than 0.");
		term(ERROR_BAD_ARGUMENTS);
	} // End of If


	/* 3. Check if Relation Exists */
	res = PQexecParams(conn,
			   "SELECT exists(SELECT master_id, slave_id FROM public.image_relations WHERE master_id = $1 AND slave_id = $2);",
			   2, NULL, (const char *[]) { masterID, slaveID }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "setImageMaster", false))
		term(ERROR_DATABASE_FAILURE);
	if (PQgetvalue(res, 0, 0)[0] == 't') {
		printf("setImagemaster: Error: Master-slave relation already exists.\n");
		PQclear(res);
		term(ERROR_FILE_EXISTS);
	} // End of If


	PQclear(res);
	res = PQexecParams(conn,
			   "SELECT exists(SELECT master_id, slave_id FROM public.image_relations WHERE master_id = $2 AND slave_id = $1);",
			   2, NULL, (const char *[]) { masterID, slaveID }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "setImageMaster", false))
		term(ERROR_DATABASE_FAILURE);
	if (PQgetvalue(res, 0, 0)[0] == 't') {
		printf("setImagemaster: Error: Master-slave relation already exists.\n");
		term(ERROR_FILE_EXISTS);
	} // End of If



	/* 4. Create Relation */
	PQclear(res);
	printf("Creating hierical relationship...\n");
	res = PQexecParams(conn,
			   "INSERT INTO public.image_relations (master_id, slave_id, index) VALUES ($1, $2, $3);",
			   3, NULL, (const char *[]) { masterID, slaveID, index }, NULL, NULL, 0);
	if (!checkCmdOk(&res, "setImageMaster", false))
		term(ERROR_DATABASE_FAILURE);


	/* 5. Terminate */
	PQclear(res);
	term(ERROR_SUCCESS);
} // End of Function

extern bool setImageRating(const char *imgId, const char *rating) {
	char *ratingId;
	int ratingIDlen;
	PGresult *res;


	/* 1. Check if Image Exists*/
	if (!checkExists("images", "id", imgId)) {
		printf("setImageRating: Error: No such image.\n");
		return false;
	} // End of If


	/* 2. Validate Rating */
	if (!checkExists("ratings", "name", rating)) {
		printf("setImageRating: Error: No such rating.\n");
		return false;
	} // End of If


	/* 3. Get Rating ID */
	res = PQexecParams(conn,
			   "SELECT id FROM public.ratings WHERE name = $1;",
			   1, NULL, (const char *[]) { rating }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "setImageRating", false))
		return false;

	ratingIDlen = PQgetlength(res, 0, 0) + 1;
	ratingId = malloc(ratingIDlen);

	checkNull(ratingId, "setImageRating");
	sprintf_s(ratingId, ratingIDlen, "%s", PQgetvalue(res, 0, 0));
	printf("%s\n", ratingId);
	PQclear(res);


	/* 4. Check if Rating Has Been Set */
	res = PQexecParams(conn,
			   "SELECT exists(SELECT * FROM public.images_ratings WHERE image_id = $1);",
			   1, NULL, (const char *[]) { imgId }, NULL, NULL, 0);
	if (!checkTuplesOk(&res, "setImageRating", false))
		return false;


	/* Set New Rating */
	if (PQgetvalue(res, 0, 0)[0] == 't') {
		res = PQexecParams(conn,
				   "UPDATE public.images_ratings SET rating_id = $2 WHERE image_id = $1;",
				   2, NULL, (const char *[]) { imgId, ratingId }, NULL, NULL, 0);
	} else {
		res = PQexecParams(conn,
				   "INSERT INTO public.images_ratings (image_id, rating_id) VALUES ($1, $2);",
				   2, NULL, (const char *[]) { imgId, ratingId }, NULL, NULL, 0);
	} // End of If/Else

	if (!checkCmdOk(res, "setImageRating", false))
		return false;


	PQclear(res);
	if (!update("images", imgId))
		return false;
	else
		return true;
} // End of Function

extern void unsetImageMaster(char *slaveID) {
	PGresult *res;

	/* 1. Check if Exists */
	if (!checkExists("images", "id", slaveID)) {
		printf("unsetImageMaster: Error: No such image");
		term(ERROR_FILE_NOT_FOUND);
	} // End of If


	/* 2. Delete */
	res = PQexecParams(conn,
			   "DELETE FROM public.images WHERE slave_id = $1",
			   1, NULL, (const char *[]) { slaveID }, NULL, NULL, 0);
	if (!checkCmdOk(res, "unsetImageMaster", false))
		term(ERROR_DATABASE_FAILURE);

	PQclear(res);
	term(ERROR_SUCCESS);
} // End of If
