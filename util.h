#ifndef CHATNET_UTIL_H_
#define CHATNET_UTIL_H_

#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>

#include "./deps/sc/sc_log.h"
#include "string.h"

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

bool entexists(char *filename);

char *getdbdir();

char *getdbpath();

char *getdblockfile();

char* getdbunlockfile();

char *getlogprevfile();

char *getloglatestfile();

void log_cleanup();

void unsetdblock();

void setdblock();

void createnewdb();

#define logdebug(...)                                                          \
	do {                                                                       \
		const char *debug = getenv("CHATNET_DEBUG");                           \
		if (debug != NULL) {                                                   \
			if (f_log_inited == false) {                                       \
				sc_log_init();                                                 \
				sc_log_set_stdout(false);\
				sc_log_set_file(getlogprevfile(), getloglatestfile());         \
				atexit(log_cleanup);                                           \
				f_log_inited = true;                                           \
			}                                                                  \
                                                                               \
			sc_log_info(__VA_ARGS__);                                         \
		}                                                                      \
	} while (0)

#endif // CHATNET_UTIL_H_
