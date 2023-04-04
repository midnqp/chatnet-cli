#include "db.h"

bool f_dbcleaned = false;
bool f_dbinited = false;
bool f_atexitadded = false;
json_object *jsondb = NULL;
FILE *dbfile = NULL;
/* 4 million microseconds equals 4 seconds */
static int timeoutinit = 4 * 1000 * 1000;
char *leveldberr = NULL;

/* idempotent db cleanup */
void leveldbcleanup() {
	if (f_dbcleaned == false) {
		if (dbfile != NULL)
			fclose(dbfile);
		if (jsondb != NULL)
			json_object_put(jsondb);
		f_dbcleaned = true;
		f_dbinited = false;
	}
}

/* idempotent db init */
void leveldbinit() {
	if (f_dbinited == false) {
		int timeoutc = 0;
		char* dbdir = getdbdir();
		char *dbpath = getdbpath();
		char *lockfile = getdblockfile();

		// create if missing ;)
		if (!entexists(dbdir)) {
			logdebug("not found, creating db");
			createnewdb();
		}

		while (timeoutc < timeoutinit) {
			if (!entexists(lockfile)) {
				setdblock();
				dbfile = fopen(dbpath, "r+");
				break;
			}

			logdebug("database locked, retrying ... (%d/%d) ms\n",
					 timeoutc / 1000, timeoutinit / 1000);
			int waitms = 500 * 1000;
			timeoutc += waitms;
			usleep(waitms); // 500ms
		}
		if (dbfile == NULL) {
			/*fprintf(stderr, "database conn failed\n");*/
			/*exit(2);*/
			unsetdblock();
			logdebug("conn failed, recreating database\n");
			createnewdb();
			dbfile = fopen(dbpath, "r+");
		}

		fseek(dbfile, 0L, SEEK_END);
		long filesize = ftell(dbfile);
		rewind(dbfile);

		char *contents = strinit(filesize+1);
		size_t readsize = fread(contents, 1, filesize, dbfile);
		rewind(dbfile);
		if (readsize != filesize) {
			fprintf(stderr, "file read failed\n");
			exit(5);
		}
		contents[filesize] = '\0';
		jsondb = json_tokener_parse(contents);

		unsetdblock();
		f_dbinited = true;
		f_dbcleaned = false;
		if (!f_atexitadded) {
			atexit(leveldbcleanup);
			f_atexitadded = true;
		}
	}
}

char *leveldbget(const char *key) {
	leveldbinit();
	char *val = "";

	json_object *valjson = json_object_object_get(jsondb, key);
	if (valjson != NULL) {
		val = json_object_get_string(valjson);
	}
	leveldbcleanup();
	return val;
}

void leveldbput(const char *key, const char *val) {
	leveldbinit();
	json_object_object_add(jsondb, key, json_object_new_string(val));
	const char* contents = json_object_to_json_string(jsondb);
	fputs(contents, dbfile);
	leveldbcleanup();
}
