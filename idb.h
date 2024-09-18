#pragma once

#ifndef IDB_H
#define IDB_H


#include <windows.h>
#include <winreg.h>

#include <libpq-fe.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>


extern void addAlias(const char *tag, const char *alias);

extern void addAliasCmd(void);

extern bool addImgCommentary(char *id, char lang[3], const char *comm);

extern void addImageToPool(char *pool, const char *imageID, const bool drop);

extern void addImgSrc(char *id, const char *src);

extern void addImplication(char *implicator, char *implication);

extern void addImplicationCmd(void);

/**
 * @brief Create a new pool
 * @param name Name of the new pool
 * @param desc Description of the pool
 * @param type Type of pool
 */
extern void addPool(const char *name, const char *desc, char *type);

extern void addPoolCmd(void);

extern void addTag3(const char *name, char *cat);

extern void addTagCmd(void);

extern bool addTagToImage(char *imgId, char *tagId);

extern void addToPoolCmd(bool drop);

extern void addWiki(char *tagName, char *description);

/**
 * @brief Parse flags and their arguments
 * @param argc Number of arguments
 * @param arg Value to compare against the values of fmt
 * @param fmt Array of possible arguments
 * @param numFlags Number of elements in arg.
 * @return The index of fmt to which the current value of arg belongs to.
 * @todo Change fmt to be a pointer to an array
 */
extern int argParse(char *arg, char **fmt, const int numFlags);

extern void assertFlagArgProvided(void);

extern void begin(PGresult **res);

extern void blacklistInternal(char *tag, bool blacklist);

/**
 * @brief Check if an entry already exists.
 * @param table: Name of the table to query from
 * @param column: Name of the column to check against (excluding public.*)
 * @param value: Value being checked
 * @return True if a tuple containing `value` exists.
 * @note DO NOT USE FOR UNSANITIZED USER INPUT!!!
*/
extern bool checkExists(const char *table, const char *column, const char *value);

extern bool checkFileExists(const char *path);

extern void checkNull(const char *str, const char *caller);

extern void checkPqError(PGresult *res);

extern void checkSanity(void);

extern void changeTagCategory(const char *tagName, const char *newCat);

extern bool checkCmdOk(PGresult **res, const char *caller, const bool rb);

extern bool checkTuplesOk(PGresult **res, char *caller, const bool rb);

extern void commit(PGresult **res);

extern void countImages(void);

extern void countTags(void);

extern void dbConnect(char *username, char *dbName, char *passwd, char *addr, uint16_t port);

extern int dirInit(void);

extern void dropAlias(char *name);

extern void dropCommentary(char *imageID, char *lang);

extern void dropImage(char *id);

extern void dropImgCmd(void);

extern void dropImgSrc(char *id);

extern void dropImplication(char *implicator, char *implication);

extern void dropImplicationCmd(void);

extern void dropPool(char *name);

extern void dropTag(char *name);

extern bool dropTagFromImage(char *imageID, char *tagID);

extern void dropWiki(char *name);

extern void dropWikiCmd(void);

extern void editCommentary(char *imageID, char lang[3], char *comm);

extern void editImgCmd(void);

extern void editImgSrc(char *id, const char *src);

extern void editPoolCmd(void);

extern void editPoolDescription(char *name, char *desc);

extern void editTagCmd(void);

extern void editWiki(char *name, char *description);

extern void editWikiCmd(void);

extern void exportCmd(void);

extern void exportImage(char *id, char *dest);

extern void favouriteInternal(char *id, bool favourite);

/**
 * @brief Get the target of an alias.
 * @param tag Pointer to the address of target tag being returned.
 * @param name Name of the alias.
 * @return True of success
 */
extern void getAliasTarget(char **tag, char *name);

/**
 * @brief Get the file extension of a file.
 * @param buf: Pointer to the destination string (NOT an array!)
 * @param fileName: Name/path of the file
 * @return The number of bytes written.
*/
extern int getFileExtension(char **buf, const char *fileName);

/**
 * @brief Get the argument of a flag.
 * @param
 * @param
 * @return
 * TODO: Rewrite this to be memory safe
 */
extern char *getFlagArg(void);

extern void getImageMaster(char *slaveID);

extern void getImageSlaves(char *masterID);

extern int getLongestTupleLength(const char *table, const char *column);

extern void getSHA256(char **dest, char *dat, size_t len);

/**
 * @brief Get a tag's ID
 * @param id Buffer to store the result in.
 * @param name Name of the tag to query.
 * @param len Number of bytes written.
 */
extern void getTagIDbyName(char **id, const char *name, size_t *len);

/**
 * @brief Append all implications of a tag to a list of tags.
 * @param head Last node in a list of tags.
 */
extern void getTagImplications(struct tagNode *head);

extern int getTupleLength(const char *table, const char *column, const char *value);

/**
 * @brief Add a file to the database.
 * @param imgPath Path of the file to be imported
 * @param copy Copy or move the file into the IDB directory
 * @param rating String representation of the rating to be applied
 * @param tagsRoot Linked list of tags associated with the file
 * @param commRoot commentary Linked list containing commentary (if any)
 */
extern void importImage3(const char *imgPath, bool copy, char *rating, struct tagNode *tagsRoot, struct comNode *commRoot);

extern int initReg(void);

extern bool isAlias(char *name);

extern void listImageTags(const char *id);

/**
 * @brief List all tags
 * @param category Name of the category to select from (optional)
 */
extern void listTags(char *category);

/**
 * @brief
 */
extern void parseCmdArgs(void);

/**
 * @brief
 * @param dbName
 * @param dbUsername
 * @param dbPassword
 * @param dbPort
 * @param dbAddr
 */
extern void parseConnArgs(char **dbName, char **dbUsername, char **dbPassword, unsigned long **dbPort, char **dbAddr);

/**
 * @brief Parse flags of the import command.
 */
extern void importCmd(void);

/**
 * @brief Print part of a row.
 * @param value The value to be printed in the middle.
 * @param longestValueLen The longest value in the column.
 * @param eolChar The character to print at the end of the line (0 for nothing, 1 for |, and 2 for \n)
 * @param centre Centre text (true), or align left (false).
 */
extern void printRowSegment(char *value, size_t longestValueLen, uint8_t eolChar, bool centre);

extern void printWiki(char *name);

/**
 * @brief
 * @param andRoot List of tags to be included.
 * @param bliRoot Blacklist exclusion list; temporarily disable user-specified blacklists.
 * @param notRoot List of tags to be excluded.
 * @param orRoot
 * @param xorRoot
 * @param datRoot List of dates; before, after, on.
 */
extern void queryImages2(struct tagNode *andRoot, struct tagNode *bliRoot, struct tagNode *notRoot,
			 struct tagNode *orRoot, struct orNode *xorRoot, struct dateNode *datRoot);

extern void queryImgCmd(void);

extern void renamePool(char *name, char *newName);

/**
 * @brief Change the name of a tag
 * @param name Existing name of the tag
 * @param newName New name for the tag
 * @notes Does not accept aliases.
*/
extern void renameTag(char *name, char *newName);

extern void rollback(PGresult **res);

extern void setImageMaster(const char *masterID, const char *slaveID, const char *index);

extern bool setImageRating(const char *imgId, const char *rating);

extern bool shouldRollback(PGresult *res, int status);

/**
 * @brief Concatenate strings with a format.
 * @param dest Pointer to the destination string.
 * @param destLen Current length of dest.
 * @param fmt printf-style format.
 * @param ... Format string
 * @return 0
 */
extern int strCatFmtS(char **dest, size_t destLen, const char *fmt, ...);

extern void term(int ret);

extern void unsetImageMaster(char *slaveID);

/**
 * @brief Set the updated column of a row to now()
 * @param Name of the table
 * @param ID of the value
 * @return True on success
 */
extern bool update(const char *table, const char *id);

extern void usage(void);

extern void wikiCmd(void);

extern char *appData[2];


/* Variables */
extern int argc_;
extern char ***argv_;
extern bool cache;
extern bool colour;
extern PGconn *conn; // PostgreSQL connection handle
extern int curArg;
extern int flag;
extern PHKEY idbKey;
extern int numFlags;
extern bool printID;
extern bool tabulate;
extern bool verbose; // Verbosity flag


struct comNode {
	char lang[3];
	char *dat;
	struct comNode *next;
};


struct dateNode {
	char *date;
	char *when;	// BEFORE, AFTER ON
	struct dateNode *next;
};


struct tagNode {
	char *name;
	struct tagNode *next;
};


struct orNode {
	char *tagA;
	char *tagB;
	struct orNode *next;
};


struct ratNode {
	bool s;
	bool q;
	bool x;
	struct ratNode *next;
};


#define blacklist(id) blacklistInternal(id, true);
#define favourite(id) favouriteInternal(id, true)
#define intLen(x) (floor(log10(abs(x))) + 1)
#define arrayLen(arr) (sizeof(arr) / sizeof(arr[0]))
#define dbDisconnect() PQfinish(conn)
#define unfavourite(id) favouriteInternal(id, false)
#define whitelist(id) blacklistInternal(id, false);


//#define DBG
#define FLAGCHAR '/'
#define SYSRT "\\idb"		// %PROGRAMFILES%\idb
#define VERSION "0.8.0-ALPHA"



/* Colours */


#endif
