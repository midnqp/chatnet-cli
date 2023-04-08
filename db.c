#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <json-c/json.h>

#include "autofree.h"
#include "str.h"
#include "util.h"
#include "db.h"

int timeoutinit = 10 * 1000 * 1000; // 10 seconds

/* idempotent db cleanup */
void leveldbcleanup(json_object **jsondb) {
	unsetdblock(); // this must work, otherwise error in algo.
	if (*jsondb != NULL) {
		json_object_put(*jsondb);
		*jsondb = NULL;
	}
}

/* idempotent db init */
void leveldbinit(json_object **jsondb) {
	int timeoutc = 0;
	char *dbpath = getdbpath();
	char *lockfile = getdblockfile();
	char *unlockfile = getdbunlockfile();

	while (true) {
		if (!entexists(lockfile) && entexists(unlockfile)) {
			setdblock();
			break;
		}

		int waitms = 100 * 1000; // 100 ms
		timeoutc += waitms;
		usleep(waitms);
	}

	char *dbcontents = file_read(dbpath);
	*jsondb = json_tokener_parse(dbcontents);
	json_parse_check(*jsondb, dbcontents);
	logdebug("parsed json: %s\n", json_object_to_json_string(*jsondb));
}

char *leveldbget(const char *key) {
	json_object *jsondb;
	leveldbinit(&jsondb);
	char *result = NULL;

	json_object *value = json_object_object_get(jsondb, key);
	if (value != NULL) {
		result = strinit(1);
		strappend(&result, json_object_get_string(value));
	}

	leveldbcleanup(&jsondb);
	return result;
}

void leveldbput(const char *key, const char *val) {
	json_object *jsondb;
	leveldbinit(&jsondb);

	json_object_object_add(jsondb, key, json_object_new_string(val));
	const char *contents = json_object_to_json_string(jsondb);
	file_write(getdbpath(), contents);

	leveldbcleanup(&jsondb);
}
