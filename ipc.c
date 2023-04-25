#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
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

/* check if given key exists */
bool ipc_get_is_key(const char* key) {
	json_object *jsono;
	ipc_init(&jsono);
	json_object *value;
	bool result = json_object_object_get_ex(jsono, key, &value);
	ipc_cleanup(&jsono);
	return result;
}

char *ipc_get(const char *key) {
	json_object *jsono;
	ipc_init(&jsono);
	char *result = NULL;

	json_object *value; 
	bool found = json_object_object_get_ex(jsono, key, &value);
	if (found != false) {
		result = strinit(1);
		strappend(&result, json_object_get_string(value));
		json_object_get_type(jsono);
	}

	ipc_cleanup(&jsono);
	return result;
}

/* Asserts whether value is found, use 
 * `ipc_get_is_key()` to check if value exists.
 */
bool ipc_get_boolean(const char* key) {
	json_object *jsono;
	ipc_init(&jsono);
	bool result = false;

	json_object* value;
	bool found = json_object_object_get_ex(jsono, key, &value);
	assert(found == true);
	result = json_object_get_boolean(value);

	ipc_cleanup(&jsono);
	return result;
}

char* ipc_get_string(const char* key) {
	json_object *jsono;
	ipc_init(&jsono);
	char* result = strinit(1);

	json_object* value;
	bool found = json_object_object_get_ex(jsono, key, &value);
	assert(found == true);
	strappend(&result, json_object_get_string(value));

	ipc_cleanup(&jsono);
	return result;
}

/* required to free the returned result 
 * by `json_object_put()`.
 */
json_object* ipc_get_array(const char* key) {
	json_object* jsono;
	ipc_init(&jsono);
	json_object* result = json_object_new_array();

	json_object* value;
	bool found = json_object_object_get_ex(jsono, key, &value);
	assert(found == true);
	assert(json_object_get_type(value) == json_type_array);

	for (int i=0; i < json_object_array_length(value); i++) {
		json_object* item = json_object_get( json_object_array_get_idx(value, i));
		json_object_array_add(result, item);
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

void ipc_put_string(const char* key, const char* val) {
	json_object *jsono;
	ipc_init(&jsono);

	json_object_object_add(jsono, key, json_object_new_string(val));

	const char *contents = json_object_to_json_string(jsono);
	file_write(getipcpath(), contents);
	ipc_cleanup(&jsono);
}

void ipc_put_boolean(const char* key, bool val) {
	json_object *jsono;
	ipc_init(&jsono);

	json_object_object_add(jsono, key, json_object_new_boolean(val));

	const char *contents = json_object_to_json_string(jsono);
	file_write(getipcpath(), contents);
	ipc_cleanup(&jsono);
}

/* do not free the arg `val` through `json_object_put()` */
void ipc_put_array(const char* key, json_object* val) {
	assert(json_object_get_type(val) == json_type_array);
	json_object *jsono;
	ipc_init(&jsono);

	json_object_object_add(jsono, key, val);

	const char *contents = json_object_to_json_string(jsono);
	file_write(getipcpath(), contents);
	ipc_cleanup(&jsono);
}
