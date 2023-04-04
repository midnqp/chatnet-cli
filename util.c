#include "util.h"

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

void setdblock() {
	rename(getdbunlockfile(), getdblockfile());
}

void unsetdblock() {
	rename(getdblockfile(), getdbunlockfile());
}

void createnewdb() {
	char *dbpath = getdbpath();
	char *dbdir = getdbdir();
	char *unlockfile = getdbunlockfile();
	char *lockfile = getdblockfile();

	/*rmdir(dbdir);*/
	if (entexists(lockfile)) unlink(lockfile);
	if(entexists(unlockfile)) unlink(unlockfile);
	if(entexists(dbpath)) unlink(dbpath);

	mkdir(dbdir, 0700);
	FILE *dbfile = fopen(dbpath, "a+");
	fputs("{}", dbfile);
	fclose(dbfile);
	fclose(fopen(unlockfile, "a+"));
}

void log_cleanup() {
	if (f_log_inited == true) {
		sc_log_term();
		f_log_inited = false;
	}
}
