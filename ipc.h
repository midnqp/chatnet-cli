#ifndef CHATNET_DB_H_
#define CHATNET_DB_H_

#include <stdbool.h>
#include <json-c/json_types.h>

void ipc_init(json_object** jsono);

void ipc_cleanup(json_object** jsono);

/* get value by key, fatal exit on error */
//char *ipc_get(const char *key);

json_object* ipc_get_array(const char* key);

bool ipc_get_boolean(const char* key);

char* ipc_get_string(const char* key);

/* put value by key, fatal exit on error */
//void ipc_put(const char *key, const char *val);

void ipc_put_array(const char* key, json_object* val);

void ipc_put_string(const char* key, const char* val);

void ipc_put_boolean(const char* key, bool val);

#endif // ends CHATNET_DB_H_
