#include "db.h"

size_t leveldblen = -1;
bool f_dbcleaned = false;
bool f_dbinited = false;

leveldb_t *db;
leveldb_options_t *dbopts;
leveldb_readoptions_t *ropts;
leveldb_writeoptions_t *wopts;

void leveldbcleanup();

/* idempotent db init */
void leveldbinit() {
	if (f_dbinited == false) {
		dbopts = leveldb_options_create();
		db = leveldb_open(dbopts, getdbpath(), &leveldberr);
		ropts = leveldb_readoptions_create();
		wopts = leveldb_writeoptions_create();
		f_dbinited = true;
		atexit(leveldbcleanup);

		if (leveldberr != NULL) {
			fprintf(stderr, "fatal: database conn failed\n%s\n", leveldberr);
			exit(2);
		}
	}
}

/* idempotent db cleanup */
void leveldbcleanup() {
	if (f_dbcleaned == false) {
		leveldb_readoptions_destroy(ropts);
		leveldb_writeoptions_destroy(wopts);
		leveldb_options_destroy(dbopts);
		leveldb_close(db);
		f_dbcleaned = true;
	}
}


char *leveldbget_e(const char *key, char **errptr) {
	size_t len = -1;
	char *data = leveldb_get(db, ropts, key, strlen(key), &len, errptr);

	char *result = strinit(len + 1);
	memcpy(result, data, len);
	result[len] = '\0';
	leveldb_free(data);

	return result;
}

char *leveldbget(const char *key) { 
	char* val = leveldbget_e(key, &leveldberr);
	if (leveldberr != NULL) {
		fprintf(stderr, "fatal: database query failed\n%s\n", leveldberr);
		exit(2);
	}
	return val;
}


void leveldbput_e(const char *key, const char *val, char **errptr) {
	leveldb_put(db, wopts, key, strlen(key), val, strlen(val), errptr);
}

void leveldbput(const char *key, const char *val) {
	leveldbput_e(key, val, &leveldberr);
	if (leveldberr != NULL) {
		fprintf(stderr, "fatal: database save failed\n%s\n", leveldberr);
		exit(2);
	}
}
