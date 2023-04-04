#ifndef CHATNET_DB_H_
#define CHATNET_DB_H_

#include <rocksdb/c.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <json-c/json.h>

#include "autofree.h"
#include "string.h"
#include "util.h"

/* in case of any error, this is populated */
//char *leveldberr = NULL;

void leveldbinit();

/* takes ERRPTR and adds error in it, instead of fatal exit */
char *leveldbget_e(const char *key, char **errptr);

/* get value by key, fatal exit on error */
char *leveldbget(const char *key);

/* put value by key, fatal exit on error */
void leveldbput(const char *key, const char *val);

/* takes ERRPTR and adds error in it, instead of fatal exit */
void leveldbput_e(const char *key, const char *val, char **errptr);

#endif // ends CHATNET_DB_H_
