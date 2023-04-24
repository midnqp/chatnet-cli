#ifndef CHATNET_UTIL_H_
#define CHATNET_UTIL_H_

#include "./deps/sc/sc_log.h"
#include <json-c/json_types.h>

typedef struct {
	/* ok = 0
	 * not found = 1
	 * corruption = 2
	 * not supported = 3
	 * invalid = 4
	 * io error = 5
	 *
	 * @see https://github.com/google/leveldb
	 */
	int code;
	char msg[1024];
} status;

static bool f_log_inited = false;

#define logdebug(...)                                                          \
	do {                                                                       \
		const char *debug = getenv("CHATNET_DEBUG");                           \
		if (debug == NULL)                                                     \
			break;                                                             \
		if (!f_log_inited) {                                                   \
			sc_log_init();                                                     \
			sc_log_set_stdout(false);                                          \
			sc_log_set_file(getlogprevfile(), getloglatestfile());             \
                                                                               \
			atexit(log_cleanup);                                               \
			f_log_inited = true;                                               \
		}                                                                      \
                                                                               \
		sc_log_info(__VA_ARGS__);                                              \
	} while (0)

bool entexists(char *filename);

char* getconfigdir();

char *getipcdir();

char *getipcpath();

char *getipclockfile();

char *getipcunlockfile();

char *getlogprevfile();

char *getloglatestfile();

void unsetipclock();

void setipclock();

void createnewipc();

void initnewipc();

char *file_read(const char *filename);

void file_write(const char *filename, const char *contents);

void json_parse_check(json_object *o, const char *str);

void log_cleanup();

#endif // CHATNET_UTIL_H_
