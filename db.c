#include "db.h"

bool f_dbcleaned = false;
bool f_dbinited = false;
bool f_atexitadded = false;
json_object *jsondb = NULL;
FILE *dbfile = NULL;
/* 5 million microseconds equals 5 seconds */
static int timeoutinit = 5 * 1000 * 1000;
char *leveldberr = NULL;

/*void leveldbcleanup();*/

/*rocksdb_t *db;*/
/*rocksdb_options_t *dbopts;*/
/*rocksdb_readoptions_t *ropts;*/
/*rocksdb_writeoptions_t *wopts;*/

/* idempotent db init */
// void leveldbinit() {
//	if (f_dbinited == false) {
//		int timeoutc = 0;
//		dbopts = rocksdb_options_create();
//
//		// voila! keep retrying connection...
//		/*while (timeoutc < timeoutinit) {*/
//			db = rocksdb_open(dbopts, getdbpath(), &leveldberr);
//			rocksdb_options_set_create_if_missing(dbopts, 1);
//
//			/*if (leveldberr != NULL) {*/
//				/*// error! io locked.*/
//				/*logdebug("", " database error: %s\n", leveldberr);*/
//				/*int waitms = 1 * 1000; // 1ms*/
//				/*[>usleep(waitms);<]*/
//				/*sleep(1);*/
//				/*timeoutc += waitms;*/
//				/*logdebug("db", " connecting (%d/%d)\n", timeoutc,
// timeoutinit);*/
//			/*} else {*/
//				// no error!
//				ropts = rocksdb_readoptions_create();
//				wopts = rocksdb_writeoptions_create();
//				f_dbinited = true;
//				f_dbcleaned = false;
//				atexit(leveldbcleanup);
//				/*break;*/
//			/*}*/
//		/*}*/
//
//		if (leveldberr != NULL) {
//			fprintf(stderr, "fatal: database conn failed\n%s\n", leveldberr);
//			exit(2);
//		}
//	}
// }

void leveldbcleanup() {
	if (f_dbcleaned == false) {
		if (dbfile != NULL)
			fclose(dbfile);
		if (jsondb != NULL)
			json_object_put(jsondb);
		f_dbcleaned = true;
		f_dbinited = true;
	}
}

/* idempotent db init */
void leveldbinit() {
	if (f_dbinited == false) {
		int timeoutc = 0;
		char *dbpath = getdbpath();
		char *lockfile = getdblockfile();

		while (timeoutc < timeoutinit) {
			if (!entexists(lockfile)) {
				setdblock();
				dbfile = fopen(dbpath, "r+");
				break;
			}

			int waitms = 500 * 1000;
			usleep(waitms); // 500ms
			timeoutc += waitms;
		}
		if (dbfile == NULL) {
			fprintf(stderr, "database conn failed\n");
			exit(2);
		}

		unsetdblock();
		f_dbinited = true;
		f_dbcleaned = false;
		if (!f_atexitadded) {
			atexit(leveldbcleanup);
			f_atexitadded = true;
		}
	}
}

/* idempotent db cleanup */
/*void leveldbcleanup() {
	if (f_dbcleaned == false) {
		rocksdb_readoptions_destroy(ropts);
		rocksdb_writeoptions_destroy(wopts);
		rocksdb_options_destroy(dbopts);
		if (leveldberr != NULL)  rocksdb_free(leveldberr);
		if (db != NULL)
			rocksdb_close(db);
		f_dbcleaned = true;
		f_dbinited = false;
	}
}*/

char *leveldbget_e(const char *key, char **errptr) {
	leveldbinit();
	size_t len = -1;
	char *data = rocksdb_get(dbfile, ropts, key, strlen(key), &len, errptr);

	char *result = strinit(len + 1);
	memcpy(result, data, len);
	result[len] = '\0';
	rocksdb_free(data);
	leveldbcleanup();

	return result;
}

char *leveldbget(const char *key) {
	char *val = leveldbget_e(key, &leveldberr);
	if (leveldberr != NULL) {
		fprintf(stderr, "fatal: database query failed\n%s\n", leveldberr);
		exit(2);
	}
	return val;
}

void leveldbput_e(const char *key, const char *val, char **errptr) {
	leveldbinit();
	rocksdb_put(dbfile, wopts, key, strlen(key), val, strlen(val), errptr);
	leveldbcleanup();
}

void leveldbput(const char *key, const char *val) {
	leveldbput_e(key, val, &leveldberr);
	if (leveldberr != NULL) {
		fprintf(stderr, "fatal: database save failed\n%s\n", leveldberr);
		exit(2);
	}
}
