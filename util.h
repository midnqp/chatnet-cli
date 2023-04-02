#ifndef CHATNET_UTIL_H_
#define CHATNET_UTIL_H_

#include <stdbool.h>
#include <unistd.h>
#include "string.h"

#ifndef DEBUG
#define DEBUG 0
#endif

// TODO remove
#define _add(a, b, c) a b c

// TODO remove
#define logdebug(label, ...)                                                   \
	if (DEBUG) {                                                               \
		/* blue bg color */                                                    \
		const char *l = _add("\033[104m", label, "\033[0m ");       \
		printf(l __VA_ARGS__);                                            \
	}

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

bool entexists(char *filename);

char *getdbpath(); 

#endif // CHATNET_UTIL_H_
