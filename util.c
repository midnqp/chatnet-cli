#include <json-c/json.h>
#include <json-c/json_types.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <execinfo.h>

#include "./deps/sc/sc_log.h"
#include "str.h"
#include "ipc.h"
#include "util.h"

void log_cleanup() {
	if (!f_log_inited)
		return;
	sc_log_term();
	f_log_inited = false;
}

/* check if file or folder exists */
bool entexists(char *filename) { return access(filename, F_OK) != -1; }

char *getdbdir() {
	char *result = strinit(1);
	strappend(&result, getenv("HOME"));
	strappend(&result, "/.config/chatnet-client");
	return result;
}

char *getdbpath() {
	char *dir = getdbdir();
	strappend(&dir, "/ipc.json");
	return dir;
}

char *getdblockfile() {
	char *dir = getdbdir();
	strappend(&dir, "/LOCK");
	return dir;
}

char *getdbunlockfile() {
	char *dir = getdbdir();
	strappend(&dir, "/UNLOCK");
	return dir;
}

char *getloglatestfile() {
	char *dir = getdbdir();
	strappend(&dir, "/log-latest.txt");
	return dir;
}

char *getlogprevfile() {
	char *dir = getdbdir();
	strappend(&dir, "/log.0.txt");
	return dir;
}

void setdblock() { rename(getdbunlockfile(), getdblockfile()); }

void unsetdblock() { rename(getdblockfile(), getdbunlockfile()); }

void createnewdb() {
	char *dbpath = getdbpath();
	char *dbdir = getdbdir();
	char *unlockfile = getdbunlockfile();
	char *lockfile = getdblockfile();

	if (!entexists(dbdir))
		mkdir(dbdir, 0700);
	if (entexists(lockfile))
		unlink(lockfile);
	if (entexists(unlockfile))
		unlink(unlockfile);
	if (entexists(dbpath))
		unlink(dbpath);

	file_write(dbpath, "{}");
	file_write(unlockfile, "");
	logdebug("dbpath %s contains: %s\n", dbpath, file_read(dbpath));
}

char *genusername() {
	char *username = strinit(48);
	char uuidstr[37];
	uuid_t uuid;
	uuid_generate(uuid);
	uuid_unparse_lower(uuid, uuidstr);
	strcpy(username, uuidstr);
	return username;
}

void initnewdb() {
	ipc_put("userstate", "true");
	ipc_put("sendmsgbucket", "[]");
	ipc_put("recvmsgbucket", "[]");
	ipc_put("username", genusername());
}

char *file_read(const char *filename) {
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		logdebug("reading file '%s' failed\n",filename);
		return NULL;
	}

	fseek(file, 0L, SEEK_END);
	size_t filesize = ftell(file);
	rewind(file);
	char *result = strinit(filesize + 1);
	(void)fread(result, sizeof(char), filesize, file);
	result[filesize] = '\0';
	fclose(file);
	return result;
}

void file_write(const char *filename, const char *contents) {
	FILE *file = fopen(filename, "w");
	/*fputs(contents, file);*/
	fprintf(file, "%s", contents);
	fflush(file);
	fsync(fileno(file));
	fclose(file);
}

char* print_stacktrace() {
	void* array[10];
	char** strings;
	int size;
	char* result = strinit(1);

	size=backtrace(array, 10);
	strings = backtrace_symbols(array, size);
	if (strings != NULL) 
		for (int i=0; i < size; i++)  {
			strappend(&result, strings[i]);
			strappend(&result, "\n");
		}
	free(strings);
	
	return result;
}

void json_parse_check(json_object *o, const char *str) {
	if (o != NULL) return;
	sc_log_error("json parse failed for string:\n%s", str);
	logdebug("stack trace:\n%s\n",print_stacktrace());
	exit(4);
}
