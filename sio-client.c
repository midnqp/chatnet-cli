#include <stdlib.h>

#include "sio-client.h"
#include "ipc.h"
#include "util.h"

status init(char *execname) {
	status err;
	char* thisdir = strinit(1024);
	realpath(dirname(execname), thisdir);
	// don't spawn if true
	bool flag_nosioclient = getenv("CHATNET_NOSIOCLIENT") != NULL;

	// system executable filename
	char sioc_name[] = "chatnet-sio-client";
	// the path to sioclient in same folder.
	char *sioclient_fullpath = strinit(1);
	strappend(&sioclient_fullpath, thisdir);
	strappend(&sioclient_fullpath, "/");
	strappend(&sioclient_fullpath, sioc_name);
	// javascript filename
	char javascript_filename[] = "client.js";
	char* jsfile_fullpath = strinit(1);
	strappend(&jsfile_fullpath, thisdir);
	strappend(&jsfile_fullpath, "/");
	strappend(&jsfile_fullpath, javascript_filename);
	// the path to nodejs executable
	char* nodejs_exe = strinit(1);
	if (cmdexists("node")) {
		strappend(&nodejs_exe, "node");
	}
	else {
		// the provided prebuilt static nodejs executable
		strappend(&nodejs_exe, thisdir);
		strappend(&nodejs_exe, "/");
		strappend(&nodejs_exe, "node");
	}

	bool is_found_jsfile = entexists(jsfile_fullpath);
	bool is_found_samedir = entexists(sioclient_fullpath);
	bool is_found_command = cmdexists(sioc_name);

	// finally, build the command string to simply execute!
	char *cmd = strinit(1);
	if (is_found_jsfile) {
		strappend(&cmd, nodejs_exe);
		strappend(&cmd, " ");
		strappend(&cmd, jsfile_fullpath);
	}
	else if (is_found_samedir) {
		strappend(&cmd, sioclient_fullpath);
	}
	else if (is_found_command) {
		strappend(&cmd, sioc_name);
	}
	else if (!flag_nosioclient) {
		err.code = 1;
		sprintf(err.msg,
				"binary \"%s\" not found in the same folder \"%s\", or in "
				"system $PATH.",
				sioc_name, thisdir);
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
