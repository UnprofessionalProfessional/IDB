#include "idb.h"

#include <Windows.h>
#include <winerror.h>

#include <stdio.h>


extern void parseCmdArgs(void) {
	char *cmdArgs[] = {
		"import", "i",			// Command 0,1
		"export", "x",			// Command 2,3
		"drop-image",			// Command 4
		"edit-image", "e",		// Command 5,6
		"favourite", "f",		// Command 7,8
		"unfavourite",			// Command 9
		"add-tag",			// Command 10
		"edit-tag",			// Command 11
		"drop-tag",			// Command 12
		"stat",				// Command 13
		"add-alias",			// Command 14
		"drop-alias",			// Command 15
		"add-implication",		// Command 16
		"list-colours",			// Command 17	TODO
		"drop-implication",		// Command 18
		"add-pool",			// Command 19
		"edit-pool",			// Command 20
		"drop-pool",			// Command 21
		"add-to-pool",			// Command 22
		"drop-from-pool",		// Command 23
		"add-wiki",			// Command 24
		"edit-wiki",			// Command 25
		"drop-wiki",			// Command 26
		"query", "q",			// Command 27,28
		"meta", "m",			// Command 29,30
		"blacklist", "b",		// Command 31,32
		"whitelist", "w",		// Command 33,34
		"wiki",				// Command 35
		"list-favourites", "lf",	// Command 36,37	TODO
		"cache",			// Command 38
		"list-db",			// Command 39
		"help", "?",			// Command 40,41
		"list-tags",			// Command 42
		"list-image-tags",		// Command 43
		"version",			// Command 48
		"test-alias", "ta",		// Command 44,45
		"test-implication", "ti",	// Command 46,47
		"test-ll",			// Command, 48
	};


	while ((flag = argParse((*argv_)[curArg], cmdArgs, arrayLen(cmdArgs))) != -1) {
		switch (flag) {
			/* Import */
			case 0:
			case 1:
				importCmd();
				break;

			/* Export */
			case 2:
			case 3:
				exportCmd();
				break;

			/* Drop Image */
			case 4:
				dropImgCmd();
				break;

			/* Edit Image */
			case 5:
			case 6:
				editImgCmd();
				break;

			/* Favourite */
			case 7:
			case 8:
				favourite(getFlagArg());
				break;

			/* Unfavourite */
			case 9:
				unfavourite(getFlagArg());
				break;

			/* Add Tag */
			case 10:
				printf("Add Tag\n");
				addTagCmd();
				break;

			/* Edit Tag */
			case 11:
				editTagCmd();
				break;

			/* Drop Tag */
			case 12:
				dropTag(getFlagArg());
				break;

			/* Statistics */
			case 13:
				/* Albums
				 * Aliases
				 * Comics
				 * Generic pools
				 * Images
				 * Images with commentary
				 * Images with translated commentary
				 * Implications
				 * Manga
				 * Wikis
				 * Tags
				 * Translated images
				 * Total tuples
				 */
				break;

			/* Add Alias */
			case 14:
				addAliasCmd();
				break;

			/* Drop Alias */
			case 15:
				dropAlias(getFlagArg());
				break;

			/* Add Implication */
			case 16:
				addImplicationCmd();
				break;

			// TODO: Make up a feature for this.
			case 17:
				break;

			/* Drop Implication */
			case 18:
				dropImplicationCmd();
				break;

			/* Add Pool */
			case 19:
				addPoolCmd();
				break;

			/* Edit Pool */
			case 20:
				editPoolCmd();
				break;

			/* Drop Pool */
			case 21:
				dropPool(getFlagArg());
				break;

			/* Add to Pool */
			case 22:
				addToPoolCmd(false);
				break;

			/* Drop From Pool */
			case 23:
				addToPoolCmd(true);
				break;

			/* Add Wiki */
			case 24:
				dropWikiCmd();
				break;

			/* Edit Wiki */
			case 25:
				editWikiCmd();
				break;

			/* Drop Wiki */
			case 26:
				dropWiki(getFlagArg());
				break;

			/* Query */
			case 27:
			case 28:
				queryImgCmd();
				break;

			/* Meta */
			case 29:
			case 30:
				printf("meta\n");
				break;

			/* Blacklist */
			case 31:
			case 32:
				blacklist(getFlagArg());
				break;

			/* Whitelist */
			case 33:
			case 34:
				whitelist(getFlagArg());
				break;

			/* Wiki */
			case 35:
				wikiCmd();
				break;

			/* Count Images */
			case 36:
				countImages();
				break;

			/* Count Tags */
			case 37:
				countTags();
				break;

			/* Cache */
			case 38:
				cache = true;
			// TODO: Make it take an argument
				break;

			/* List Cached Databases */
			case 39:
				printf("list-db\n");
				break;

			/* Usage */
			case 40:
			case 41:
				usage();
				term(ERROR_SUCCESS);

			/* List Tags */
			case 42:
				listTags(getFlagArg());
				break;

			/* List Image Tags */
			case 43:
				listImageTags(getFlagArg());
				break;

			/* Version */
			case 44:
				printf("Image Database "VERSION"\n");
				term(ERROR_SUCCESS);

			/* Test Alias DEBUG */
			case 45:
			case 46: {
				char *tag;
				int ret;

				getAliasTarget(&tag, getFlagArg());
				printf("%s\n", tag);
				term(ERROR_SUCCESS);
			} // End of Case

			/* Test Implication DEBUG */
			case 47:
			case 48: {
				struct tagNode *head;
				struct tagNode *currNode;

				head = (struct tagNode *) malloc(sizeof(struct tagNode));
				if (!head) {
					printf("test-implication: Error: Out of memory.\n");
					term(ERROR_OUTOFMEMORY);
				} // End of If

				head->name = getFlagArg();
				head->next = NULL;
				currNode = head;
				getTagImplications(head);

				while (currNode->next) {
					printf("%s\n", currNode->name);
					currNode = currNode->next;
				} // End of While

				term(ERROR_SUCCESS);
			} // End of Case


			/* Test Linked List */
			case 49: {
				struct tagNode *rootTag;
				struct tagNode *curTag;

				rootTag = (struct tagNode *) malloc(sizeof(struct tagNode));
				if (!rootTag) {
					printf("Error: Out of memory.\n");
					term(ERROR_OUTOFMEMORY);
				} // End of If


				/* Get Arguments */
				curTag = rootTag;
				curTag->name = getFlagArg();
				curTag = curTag->next;
				curTag = NULL;
				curArg++;

				while (curArg < argc_) {
					curTag = (struct tagNode *) malloc(sizeof(struct tagNode));
					if (!curTag) {
						printf("Error: Out of memory.\n");
						term(ERROR_OUTOFMEMORY);
					} // End of If

					curTag->name = getFlagArg();
					curTag = curTag->next;
					curTag = NULL;
					curArg++;
				} // End of While


				getTagImplications(rootTag);
				curTag = rootTag;

				while (curTag) {
					printf("%s\n", curTag->name);
					curTag = curTag->next;
				} // End of While

				term(ERROR_SUCCESS);
			} // End of Case

			default:
				break;
		} // End of Switch

		curArg++;
	} // End of While
} // End of Function

extern void parseConnArgs(char **dbName, char **dbUsername, char **dbPassword, unsigned long **dbPort, char **dbAddr) {
	char *cfgArgs[] = {
		"db-name", "d",		// CFG 0,1
		"username", "u",	// CFG 2,3
		"password", "p",	// CFG 4,5
		"port",			// CFG 6
		"address", "a",		// CFG 7,8
		"colour", "c",		// CFG 9, 10
		"id",			// CFG 11
		"tabulate", "t",	// CFG 12,13
		"verbose", "v"		// CFG 14,15
	};

	while ((flag = argParse((*argv_)[curArg], cfgArgs, arrayLen(cfgArgs))) != -1) {
		switch (flag) {
			/* Database Name */
			case 0:
			case 1: {
				*dbName = getFlagArg();
				printf("dbName: '%s'\n", *dbName);
				break;
			} // End of Case

			/* Username */
			case 2:
			case 3: {
				*dbUsername = getFlagArg();
				printf("dbUsername: '%s'\n", *dbUsername);
				break;
			} // End of Case

			/* Password */
			case 4:
			case 5: {
				*dbPassword = getFlagArg();
				printf("dbPassword: '%s'\n", *dbPassword);
				break;
			} // End of Case

			/* Port */
			case 6: {
				*dbPort = atol(getFlagArg());
				printf("dbPort: '%lu'\n", *dbPort);
				break;
			} // End of Case

			/* DB Address */
			case 7:
			case 8: {
				*dbAddr = getFlagArg();
				printf("dbAddr: '%s'\n", *dbAddr);
				break;
			} // End of Case

			/* Colour */
			case 9:
			case 10:
				colour = true;
				break;

			/* Print IDs */
			case 11:
				printf("id\n");
				printID = true;
				break;

			/* Tabulate */
			case 12:
			case 13:
				tabulate = true;
				break;

			/* Verbose */
			case 14:
			case 15:
				verbose = true;
				break;

			default:
				break;
		} // End of Switch

		curArg++;
	} // End of While
} // End of Function
