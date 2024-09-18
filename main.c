/* idb.c
 * TODO: Migrate inline TODOs to a Microsoft Project document
 * TODO: Add UTF-16 support
 * TODO: Write a function to validate language codes
 * TODO: Create warnings as errors feature
 * TODO: Add verbose messages to all fatal cases and warnings
 * TODO: Write a function to replace spaces with underscores in tag names (from input)
 * TODO: Rewrite functions with shitty variables like "var[3]"
 * TODO: SQL lower input
 * TODO: Replace underscores with spaces in table output (limited cases)
 * TODO: upper() the first letter of each word in table output
 * TODO: Replace all checkPqError's
 * TODO: Rewrite exportImage
 * TODO: Rewrite dropImage
 * TODO: Rewrite favouriteInternal
 * TODO: Document every function
 * TODO: Write generic fatal error functions
 * TODO: Replace all PQerrorMessage's with PQresultErrorMessage
 * TODO: Make void functions return a value as needed.
 * TODO: Write a statistics function, using the query
 *	SELECT
 *		relname AS TableName,
 *		n_live_tup AS LiveTuples,
 *		n_dead_tup AS DeadTuples
 *	FROM pg_stat_user_tables;
 * TODO: Add source function to import
 * TODO: Steamline naming schemes ("image" -> "img")
 * TODO: Add list favourites feature
 * TODO: Rewrite error checking functions
 * TODO: Replace manual error checking with functions (where possible)
 * TODO: Make it possible to disable blacklists without removing them.
 * TODO: Replace goto statements with continue
 * TODO:
 */

#define _CRTDBG_MAP_ALLOC

#include "idb.h"

#include <windows.h>
#include <winerror.h>

#include <libpq-fe.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>


int argc_;
char ***argv_;
bool cache;
bool colour;
int curArg;
int flag;
bool printID;
bool tabulate;
bool verbose;


int main(int argc, char *argv[]) {
	int ret;
	char *dbName = NULl;
	char *dbUsername = NULL;
	char *dbPassword = NULL;
	char *dbAddr = NULL;
	uint16_t dbPort = 0;
	char *metaArgs[] = {
		"pools", "p",
		"categories",
		"c",
		"tags", "t"
	};
	PGresult *res;


	if (argc == 1) {
		usage();
		return ERROR_BAD_ARGUMENTS;
	} // End of If


	argc_ = argc;
	argv_ = &argv;

	cache = false;
	colour = false;
	curArg = 1;
	flag = 0;
	tabulate = false;
	verbose = false;


	/* Registry */
	flag = 0;
	curArg = 1;
	ret = initReg();
	if (ret)
		return ret;


	/* Parse Connection arguments */
	parseConnArgs(&dbName, &dbUsername, &dbPassword, &dbPort, &dbAddr);
	dbConnect(dbUsername, dbName, dbPassword, dbAddr, dbPort);
	dirInit();
	checkSanity();
	parseCmdArgs();		// Parse Command Arguments
} // End of Main