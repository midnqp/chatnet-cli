#include <execinfo.h>
#include <json-c/json.h>
#include <json-c/json_types.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <gc/gc.h>

#include "deps/sc/sc_log.h"
#include "ipc.h"
#include "str.h"
#include "util.h"

// set idx_end = -1 to mean the end of string.
char *crop_string(const char *string, int idx_start, int idx_end) {
	int i = 0, j = 0, len = strlen(string);
	if (idx_end == -1)
		idx_end = len;

	//char *result = malloc(sizeof(char) * (len + 1)); // 1 for null
	char* result = strinit(len+1);
	for (i = idx_start; string[i] != NULL && i < idx_end; i++) {
		result[j++] = string[i];
		//strappend(&result, string[i]);
	}
	result[j] = '\0';
	return result;
}

// requires a null-terminated string.
char **split_string(const char *string, char delimiter) {
	int i, num_tokens = 0;
	for (i = 0; string[i] != NULL; i++) {
		if (string[i] == delimiter)
			num_tokens++;
	}

	char **tokens = GC_malloc(sizeof(char *) * (num_tokens + 2));
	int begin = 0, t = 0;
	for (i = 0; string[i] != NULL; i++) {
		if (string[i] == delimiter) {
			char *s = crop_string(string, begin, i);
			// printf("split_string: crop string: %s\n", s);
			tokens[t++] = s;

			begin = i + 1;
		}
	}

	// add the last item after the last delimiter
	char *s = crop_string(string, begin, -1);
	// printf("split_string: crop string: %s\n", s);
	tokens[t++] = s;

	tokens[t] = NULL;
	return tokens;
}

bool logdebug_if(const char *logflag) {
	const char *chatnet_debug = getenv("CHATNET_DEBUG");
	if (chatnet_debug == NULL)
		return false;
	char **splits = split_string(chatnet_debug, ',');
	for (int i = 0; splits[i] != NULL; i++) {
		bool matched = strcmp(splits[i], logflag) == 0;
		if (matched) {
			return true;
		}
	}
	return false;
}

char *long_to_string(long value) {
	// Allocate memory for the string.
	char *string = strinit(sizeof(char) * 10);

	// Convert the long integer to a string.
	sprintf(string, "%ld", value);

	// Return the string.
	return string;
}

// Date.now() of javascript ported to C 😁
long datenowms() {
	struct timespec currentTime;
	clock_gettime(CLOCK_REALTIME, &currentTime);

	// Print the current time in milliseconds.
	long millis = currentTime.tv_sec * 1000 + currentTime.tv_nsec / 1000000;
	return millis;
}

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
char *system_out(char *cmd) { return NULL; }

char *getconfigdir() {
	char *result = strinit(1);
	strappend(&result, getenv("HOME"));
	strappend(&result, "/.config");
	return result;
}

char *getconfigfile() {
	char *dir = getconfigdir();
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
	char *ipcdir = getipcdir(); // not removed across client sessions.
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
	// ipc_put_string("username", genusername());
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
	char *result = strinit(1);
	int size = backtrace(array, 10);
	char **strings = backtrace_symbols(array, size);
	if (strings != NULL) {
		for (int i = 0; i < size; i++) {
			strappend(&result, strings[i]);
			strappend(&result, "\r\n");
		}
		free(strings);
	}

	return result;
}

bool is_file_json(const char* filename) {
	bool result=true;
	char* contents = file_read(filename);
	if (contents==NULL)  return false;
	if (strlen(strtrim(contents)) == 0) return false;
	json_object* json = json_tokener_parse(contents);
	if (json == NULL) result = false;
	json_object_put(json);
	return result;
}

void json_parse_check(json_object *o, const char *str) {
	if (o != NULL)
		return;
	sc_log_error("json parse failed for string:\n%s\n", str);
	logdebug("stack trace:\n%s\n", print_stacktrace());
	exit(4);
}
