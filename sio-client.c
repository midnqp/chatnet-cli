#include "sio-client.h"

char *sioc_name = "chatnet-sio-client";

status init(char *execname) {
	status err;
	char *dir = dirname(execname);

	// check if executable found
	char *sioclientpath = strinit(1);
	strappend(&sioclientpath, dir);
	strappend(&sioclientpath, "/");
	strappend(&sioclientpath, sioc_name);
	if (!entexists(sioclientpath)) {
		err.code = 1;
		sprintf(err.msg, "binary \"%s\" not found in the same folder.",
				sioc_name);
		return err;
	}

	// check if "prebuilds" dir found, as needed by executable
	char *prebuildspath = strinit(1);
	strappend(&prebuildspath, dir);
	strappend(&prebuildspath, "/prebuilds");
	if (!entexists(prebuildspath)) {
		err.code = 1;
		sprintf(err.msg, "folder \"prebuilds\" not found in the same folder.");
		return err;
	}

	// launch executable
	char *cmd = strinit(1);
	strappend(&cmd, sioclientpath);
	strappend(&cmd, " &");
	if (getenv("CHATNET_NOSIOCLIENT") == NULL) {
		int opened = system(cmd);
		if (opened == -1) {
			err.code = 5;
			sprintf(err.msg, "launch of \"%s\" failed.", sioc_name);
			return err;
		}
	}

	// executable creates the database path
	int sleepc = 0;
	while (1) {
		if (sleepc == 10) {
			err.code = 2;
			sprintf(err.msg, "database not initiated.");
			return err;
		}

		if (entexists(getdbpath()))
			break;
		sleepc++;
		sleep(1);
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
