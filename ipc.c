#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <json-c/json.h>

#include "str.h"
#include "util.h"
#include "ipc.h"

void ipc_cleanup(json_object **jsono) {
	unsetipclock(); // this must work, otherwise error in algo.
	if (*jsono != NULL) {
		json_object_put(*jsono);
		*jsono = NULL;
	}
}

void ipc_init(json_object **jsono) {
	int timeoutc = 0;
	char *dbpath = getipcpath();
	char *lockfile = getipclockfile();
	char *unlockfile = getipcunlockfile();

	while (true) {
		if (!entexists(lockfile) && entexists(unlockfile)) {
			setipclock();
			break;
		}

		int waitms = 100 * 1000; // 100 ms
		timeoutc += waitms;
		usleep(waitms);
	}

	char *dbcontents = file_read(dbpath);
	logdebug("ipc_init contents: %s\n", dbcontents);
	*jsono = json_tokener_parse(dbcontents);
	json_parse_check(*jsono, dbcontents);
	logdebug("parsed json: %s\n", json_object_to_json_string(*jsono));
}

char *ipc_get(const char *key) {
	json_object *jsono;
	ipc_init(&jsono);
	char *result = NULL;

	json_object *value = json_object_object_get(jsono, key);
	if (value != NULL) {
		result = strinit(1);
		strappend(&result, json_object_get_string(value));
	}

	ipc_cleanup(&jsono);
	return result;
}

void ipc_put(const char *key, const char *val) {
	json_object *jsono;
	ipc_init(&jsono);

	json_object_object_add(jsono, key, json_object_new_string(val));
	const char *contents = json_object_to_json_string(jsono);
	file_write(getipcpath(), contents);

	ipc_cleanup(&jsono);
}
