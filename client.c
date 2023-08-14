#include "deps/linenoise/linenoise.h"
#include <assert.h>
#include <ctype.h>
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
#include "markdown.h"

void sendbuckets_add(char buffer[], const char *username) {
	char *_buffer = strdup(buffer);
	char *strbuffer = strinit(1);
	strappend(&strbuffer, _buffer);
	free(_buffer);

	json_object *obj = json_object_new_object();
	json_object_object_add(obj, "username", json_object_new_string(username));
	json_object_object_add(obj, "type", json_object_new_string("message"));
	json_object_object_add(obj, "data", json_object_new_string(strbuffer));
	if (logdebug_if("cclient-sendbucketsadd"))
		logdebug("sendbuckets_add: the json is %s\n", json_object_to_json_string(obj));

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
	// check if auth exists in config
	if (!config_get_is_key("auth")) {
		config_put_string("auth", "");
	}

	sioclientinit(argv[0]);
	atexit(sioclientcleanup);

	while (1) {
		bool _break = false;
		char *username = NULL;
		char *linenoise_prompt = NULL;

		if (ipc_get_is_key("username")) {
			username = ipc_get_string("username");
		}
		else if (config_get_is_key("username")) {
			username = config_get_string("username");
		} else {
			username = strinit(1);
			strappend(&username, "[name not set]");
		}
		linenoise_prompt = strinit(1);
		strappend(&linenoise_prompt, username);
		strappend(&linenoise_prompt, ": ");

		char *line;
		struct linenoiseState ls;
		char buf[10240];
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
				line = linenoiseEditFeed(&ls);
				if (line != linenoiseEditMore && ls.len > 0) // disallow empty editfeed
					break;
			} else {
				long nowinms = datenowms();
				ipc_put_string("lastping-cclient", long_to_string(nowinms));
				long lastping = strtol(ipc_get_string("lastping-sioclient"), NULL, 10);

				char *output = NULL;
				if ((nowinms - lastping) > 10000) {
					_break = true;
					output = strinit(1);
					strappend(&output, "chatnet: terminating, something went awry :(\r\n");
				} else {
					output = recvbucket_get();
				}
				if (!strlen(output))
					continue;
				linenoiseHide(&ls);
				char* rendered_output = markdown_to_ansi(output);
				printf("%s\r\n", rendered_output);
				linenoiseShow(&ls);

				if (_break)
					break;
			}
		}

		linenoiseEditStop(&ls);

		char *linenoise_buffer = buf;
		if (strcmp(linenoise_buffer, "/exit") == 0) {
			ipc_put_boolean("userstate", false);
			_break = true;
			// break; because `char* line` needs to be freed
		} else if (strncmp(linenoise_buffer, "/name", 5) == 0) {
			char *uname = strinit(16 + 1);
			int a;
			for (a = 6; a < 16 + 6; a++) {
				char c = linenoise_buffer[a];
				if (c == '\0')
					break;
				if (isalpha(c))
					uname[a - 6] = tolower(c);
			}
			uname[a - 6] = '\0';

			// this gets noticed by checkIfAuthChanged() in client.ts
			ipc_put_string("username", uname);
		} else
			sendbuckets_add(linenoise_buffer, username);

		if (line != NULL) {
			linenoiseFree(line);
			line = NULL;
		}

		if (_break)
			break;
	}
}
