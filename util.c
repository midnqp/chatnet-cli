#include <uuid/uuid.h>

#include "util.h"
#include "db.h"

/* check if file or folder exists */
bool entexists(char *filename) { return access(filename, F_OK) != -1; }

char *getdbdir() {
	char *result = strinit(1);
	strappend(&result, getenv("HOME"));
	strappend(&result, "/.config/chatnet-client");
	return result;
}

char *getdbpath() {
	char *dir = getdbdir();
	strappend(&dir, "/db.json");
	return dir;
}

char *getdblockfile() {
	char *dir = getdbdir();
	strappend(&dir, "/LOCK");
	return dir;
}

char *getdbunlockfile() {
	char *dir = getdbdir();
	strappend(&dir, "/UNLOCK");
	return dir;
}

char *getloglatestfile() {
	char *dir = getdbdir();
	strappend(&dir, "/log-latest.txt");
	return dir;
}

char *getlogprevfile() {
	char *dir = getdbdir();
	strappend(&dir, "/log.0.txt");
	return dir;
}

void setdblock() { rename(getdbunlockfile(), getdblockfile()); }

void unsetdblock() { 
	/*logdebug("lock -> unlock\n"); */
	rename(getdblockfile(), getdbunlockfile()); 
	/*logdebug("found unlockfile: %s\n", entexists(getdbunlockfile())? "true":"false");*/
}

/* no need to invoke this manually, invoked by
 * leveldbinit() from db.c, according to conditions
 */
void createnewdb() {
	char *dbpath = getdbpath();
	char *dbdir = getdbdir();
	char *unlockfile = getdbunlockfile();
	char *lockfile = getdblockfile();

	/*rmdir(dbdir);*/
	if (entexists(lockfile))
		unlink(lockfile);
	if (entexists(unlockfile))
		unlink(unlockfile);
	if (entexists(dbpath))
		unlink(dbpath);

	mkdir(dbdir, 0700);
	FILE *dbfile = fopen(dbpath, "a+");
	fputs("{}\n", dbfile);
	fclose(dbfile);
	fclose(fopen(unlockfile, "a+"));
}

char *genusername() {
	char *username = strinit(48);
	char uuidstr[37];
	uuid_t uuid;
	uuid_generate(uuid);
	uuid_unparse_lower(uuid, uuidstr);
	/*char *idxptr = strchr(uuidstr, '-');*/
	/*int idx = -1;*/
	/*idx = idxptr - uuidstr;*/
	strcpy(username, uuidstr);
	return username;
}

void initnewdb() {
	leveldbput("userstate", "true");
	leveldbput("sendmsgbucket", "[]");
	leveldbput("recvmsgbucket", "[]");
	leveldbput("username", genusername());
}

void initnewdb_raw() {
}

void log_cleanup() {
	if (f_log_inited == true) {
		sc_log_term();
		f_log_inited = false;
	}
}

char *file_read(const char *filename) {
	FILE *file = fopen(filename, "rb");
	if (!file)
		return NULL;

	fseek(file, 0L, SEEK_END);
	size_t filesize = ftell(file);
	rewind(file);
	char *result = strinit(filesize + 1);
	size_t readsize = fread(result, sizeof(char), filesize, file);
	result[filesize] = '\0';
	fclose(file);
	return result;
}

void file_write(const char *filename, const char *contents) {
	FILE *file = fopen(filename, "w");
	fputs(contents, file);
	fclose(file);
}

void json_parse_check(json_object *o, const char *str) {
	enum json_tokener_error error = json_tokener_get_error(o);
    if (error != json_tokener_success) {
		sc_log_error("json parse failed for string:\n%s", str);
		exit(4);
	}
}

