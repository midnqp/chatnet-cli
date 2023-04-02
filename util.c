#include "util.h"

/* check if file or folder exists */
bool entexists(char *filename) { return access(filename, F_OK) != -1; }


char *getdbpath() {
	char *result = strinit(1);
	strappend(&result, getenv("HOME"));
	strappend(&result, "/.config/chatnet-client");
	return result;
}
