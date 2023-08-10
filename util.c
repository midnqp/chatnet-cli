#include <execinfo.h>
#include <json-c/json.h>
#include <json-c/json_types.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "./deps/sc/sc_log.h"
#include "ipc.h"
#include "str.h"
#include "util.h"

void log_cleanup() {
	if (!f_log_inited)
		return;
	sc_log_term();
	f_log_inited = false;
}

/* check if file or folder exists */
bool entexists(char *filename) { return access(filename, F_OK) != -1; }

bool cmdexists(char *cmdname) {
	char str[1024];
#ifdef _WIN32
	const char *which_cmd = "where";
	const char *null_dev = "NUL";
#else
	const char *which_cmd = "which";
	const char *null_dev = "/dev/null";
#endif

	snprintf(str, sizeof(str), "%s %s > %s 2>&1", which_cmd, cmdname, null_dev);
	return system(str) == 0;
}

/* runs a system cmd and returns output */
char* system_out(char* cmd) {
	return NULL;
}

char *getconfigdir() {
	char *result = strinit(1);
	strappend(&result, getenv("HOME"));
	strappend(&result, "/.config");
	return result;
}

char *getconfigfile() {
	char* dir = getconfigdir();
	strappend(&dir, "/.chatnet.json");
	return dir;
}

char *getipcdir() {
	char *dir = getconfigdir();
	strappend(&dir, "/.chatnet-client");
	return dir;
}

char *getipcpath() {
	char *dir = getipcdir();
	strappend(&dir, "/ipc.json");
	return dir;
}

char *getipclockfile() {
	char *dir = getipcdir();
	strappend(&dir, "/LOCK");
	return dir;
}

char *getipcunlockfile() {
	char *dir = getipcdir();
	strappend(&dir, "/UNLOCK");
	return dir;
}

char *getloglatestfile() {
	char *dir = getipcdir();
	strappend(&dir, "/log-latest.txt");
	return dir;
}

char *getlogprevfile() {
	char *dir = getipcdir();
	strappend(&dir, "/log.0.txt");
	return dir;
}

void setipclock() { rename(getipcunlockfile(), getipclockfile()); }

void unsetipclock() { rename(getipclockfile(), getipcunlockfile()); }

void createnewipc() {
	char *ipcpath = getipcpath();
	char *ipcdir = getipcdir();
	char *unlockfile = getipcunlockfile();
	char *lockfile = getipclockfile();
	char *configdir = getconfigdir();

	if (!entexists(configdir))
		mkdir(configdir, 0700);
	if (!entexists(ipcdir))
		mkdir(ipcdir, 0700);
	if (entexists(lockfile))
		unlink(lockfile);
	if (entexists(unlockfile))
		unlink(unlockfile);
	if (entexists(ipcpath))
		unlink(ipcpath);

	file_write(ipcpath, "{}");
	file_write(unlockfile, "");
	logdebug("ipc path %s contains: %s\n", ipcpath, file_read(ipcpath));
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

void initnewipc() {
	ipc_put_boolean("userstate", true);
	ipc_put_array("sendmsgbucket", json_object_new_array());
	ipc_put_array("recvmsgbucket", json_object_new_array());
	ipc_put_string("username", genusername());
}

char *file_read(const char *filename) {
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		logdebug("reading file '%s' failed\n", filename);
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
	fprintf(file, "%s", contents);
	fflush(file);
	fclose(file);
}

char *print_stacktrace() {
	void *array[10];
	char **strings;
	int size;
	char *result = strinit(1);

	size = backtrace(array, 10);
	strings = backtrace_symbols(array, size);
	if (strings != NULL)
		for (int i = 0; i < size; i++) {
			strappend(&result, strings[i]);
			strappend(&result, "\n");
		}
	free(strings);

	return result;
}

void json_parse_check(json_object *o, const char *str) {
	if (o != NULL)
		return;
	sc_log_error("json parse failed for string:\n%s", str);
	logdebug("stack trace:\n%s\n", print_stacktrace());
	exit(4);
}
