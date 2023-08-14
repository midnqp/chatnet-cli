#include <stdlib.h>

#include "sio-client.h"
#include "ipc.h"
#include "util.h"

static char sioc_name[] = "chatnet-sio-client";

status init(char *execname) {
	status err;
	char* dir = strinit(1024);
	realpath(dirname(execname), dir);
	/* don't spawn if true */
	bool flag_nosioclient = getenv("CHATNET_NOSIOCLIENT") != NULL;

	// check if executable found in same folder
	char *sioclientpath = strinit(1);
	strappend(&sioclientpath, dir);
	strappend(&sioclientpath, "/");
	strappend(&sioclientpath, sioc_name);
	bool found_dir = entexists(sioclientpath);

	// check if executable found in PATH
	bool found_bin = cmdexists(sioc_name);

	char *cmd = strinit(1);
	if (found_dir) {
		strappend(&cmd, sioclientpath);
	}
	else if (found_bin) {
		strappend(&cmd, sioc_name);
	}
	else if (!flag_nosioclient) {
		err.code = 1;
		sprintf(err.msg,
				"binary \"%s\" not found in the same folder \"%s\", or in "
				"system $PATH.",
				sioc_name, dir);
		return err;
	}

	// launch executable
	strappend(&cmd, " &");
	if (!flag_nosioclient) {
		int opened = system(cmd);
		if (opened == -1) {
			err.code = 5;
			sprintf(err.msg, "launch of \"%s\" failed.", sioc_name);
			return err;
		}
	}

	err.code = 0;
	return err;
}

void sioclientinit(char *execname) {
	status err = init(execname);
	if (err.code != 0) {
		fprintf(stderr, "fatal: %s\n", err.msg);
		exit(2);
	}
}

void sioclientcleanup() { ipc_put_boolean("userstate", false); }
