#include "deps/linenoise/linenoise.h"
#include <assert.h>
#include <gc.h>
#include <gc/gc.h>
#include <json-c/json.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "deps/linenoise/linenoise.h"
#include "deps/sc/sc_log.h"
#include "ipc.h"
#include "sio-client.h"
#include "str.h"
#include "util.h"

char *linenoise_prompt = NULL;
char *username = NULL;

void sendbuckets_add(char buffer[], const char *username) {
	char *_buffer = strdup(buffer);
	char *strbuffer = strinit(1);
	strappend(&strbuffer, _buffer);
	free(_buffer);
	/*linenoise_buffer[0] = '\0';*/

	json_object *obj = json_object_new_object();
	json_object_object_add(obj, "username", json_object_new_string(username));
	json_object_object_add(obj, "type", json_object_new_string("message"));
	json_object_object_add(obj, "data", json_object_new_string(strbuffer));
	logdebug("sendbuckets_add: source object: %s\n", json_object_to_json_string(obj));

	json_object *bucketarr = ipc_get_array("sendmsgbucket");
	json_object_array_add(bucketarr, obj);
	ipc_put_array("sendmsgbucket", bucketarr);
}

char *recvbucket_get() {
	char *result = strinit(1);
	json_object *recvarr = ipc_get_array("recvmsgbucket");
	ipc_put_array("recvmsgbucket", json_object_new_array()); // empty array
	int arrlen = json_object_array_length(recvarr);
	for (int i = 0; i < arrlen; i++) {
		json_object *item = json_object_array_get_idx(recvarr, i);
		json_parse_check(item, "");

		json_object *jusername = json_object_object_get(item, "username");
		json_parse_check(jusername, "");
		const char *sender = json_object_get_string(jusername);

		json_object *jdata = json_object_object_get(item, "data");
		json_parse_check(jdata, "");
		const char *message = json_object_get_string(jdata);

		strappend(&result, sender);
		strappend(&result, ": ");
		strappend(&result, message);
		strappend(&result, "\r\n");
	}
	json_object_put(recvarr);
	return result;
}

// Note: do not access config from threads!
char *config_get_string(const char *key) {
	char *configfile = getconfigfile();
	assert(entexists(configfile) == true);
	char *file = file_read(configfile);
	json_object *jsono = json_tokener_parse(file);
	json_parse_check(jsono, file);

	char *result = strinit(1);
	json_object *value;
	bool found = json_object_object_get_ex(jsono, key, &value);
	assert(found == true);
	strappend(&result, json_object_get_string(value));
	json_object_put(jsono);

	return result;
}

bool config_get_is_key(const char *key) {
	char *configfile = getconfigfile();
	assert(entexists(configfile) == true);
	char *file = file_read(configfile);
	json_object *jsono = json_tokener_parse(file);
	json_parse_check(jsono, file);

	json_object *value;
	bool found = json_object_object_get_ex(jsono, key, &value);
	json_object_put(jsono);
	return found;
}

void config_put_string(const char *key, const char *val) {
	char *configfile = getconfigfile();
	assert(entexists(configfile) == true);
	char *file = file_read(configfile);
	json_object *jsono = json_tokener_parse(file);
	json_parse_check(jsono, file);

	json_object_object_add(jsono, key, json_object_new_string(val));
	const char *contents = json_object_to_json_string(jsono);
	file_write(configfile, contents);
	json_object_put(jsono);
}

int main(int argc, char *argv[]) {
	(void)argc;
	sc_log_set_thread_name("thread-main");
	logdebug("hi\n");
	GC_INIT();

	createnewipc();
	initnewipc();

	// check if config file exists, create if none
	char *configfile = getconfigfile();
	if (!entexists(configfile)) {
		file_write(configfile, "{}");
	}
	// check if username exists in config
	if (config_get_is_key("username")) {
		username = config_get_string("username");
	} else {
		//username = genusername();
		//username = strinit(1);
		//strappend(&username, "[name not set]");
	}
	// check if auth exists in config
	if (!config_get_is_key("auth")) {
		config_put_string("auth", "");
	}

	char *prompt = strinit(1);
	strappend(&prompt, username);
	strappend(&prompt, ": ");
	linenoise_prompt = prompt;

	sioclientinit(argv[0]);
	atexit(sioclientcleanup);

	while (1) {
		char *line;
		struct linenoiseState ls;
		char buf[1024];
		linenoiseEditStart(&ls, -1, -1, buf, sizeof(buf), linenoise_prompt);
		while (1) {
			fd_set readfds;
			struct timeval tv;
			int retval;

			FD_ZERO(&readfds);
			FD_SET(ls.ifd, &readfds);
			tv.tv_sec = 0;
			tv.tv_usec = 1000 * 500; // 500ms

			retval = select(ls.ifd + 1, &readfds, NULL, NULL, &tv);
			if (retval == -1) {
				perror("select()");
				exit(1);
			} else if (retval) {
				logdebug("receiving input\n");
				line = linenoiseEditFeed(&ls);
				if (line != linenoiseEditMore)
					break;
			} else {
				logdebug("checking for any output\n");
				const char *output = recvbucket_get();
				if (!strlen(output))
					continue;
				linenoiseHide(&ls);
				printf("%s", output);
				linenoiseShow(&ls);
			}
		}

		linenoiseEditStop(&ls);

		bool _break = false;
		char *linenoise_buffer = buf;
		if (strcmp(linenoise_buffer, "/exit") == 0) {
			ipc_put_boolean("userstate", false);
			_break = true;
			// break; // because `char* line` needs to be freed
		} else if (strncmp(linenoise_buffer, "/name", 5) == 0) {
			char *uname = strinit(16 + 1);
			int j = 0;
			for (int i = 6; (i < 16 + 6 || linenoise_buffer[i] != '\0'); i++)
				uname[j++] = linenoise_buffer[i];

			uname[j] = '\0';
			ipc_put_string("username", uname); // this gets noticed by checkIfAuthChanged() in client.ts
			//config_put_string("username", uname);
			username = uname;
			strcpy(linenoise_prompt, "");
			strappend(&linenoise_prompt, uname);
			strappend(&linenoise_prompt, ": ");
		} else
			sendbuckets_add(linenoise_buffer, username);

		if (line != NULL)
			linenoiseFree(line);

		if (_break)
			break;
	}
}
