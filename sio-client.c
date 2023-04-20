#include "sio-client.h"
#include "ipc.h"

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

	// launch executable
	char *cmd = strinit(1);
	strappend(&cmd, sioclientpath);
	strappend(&cmd, " &");
	if (getenv("CHATNET_NOSIOCLIENT") == NULL) {
		logdebug("starting sio-client\n");
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


void sioclientcleanup() {ipc_put("userstate", "false");}
