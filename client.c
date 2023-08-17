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
#include "markdown.h"
#include "sio-client.h"
#include "str.h"
#include "util.h"

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

char *get_chatnet_username() {
	char *username = NULL;
	if (ipc_get_is_key("username")) {
		username = ipc_get_string("username");
	} else if (config_get_is_key("username")) {
		username = config_get_string("username");
	} else {
		username = strinit(1);
		strappend(&username, "[name not set]");
	}
	return username;
}

void sendbuckets_add(char buffer[]) {
	char* username = get_chatnet_username();
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

void recvbuckets_add(char buffer[]) {
	char* username = get_chatnet_username();
	char *_buffer = strdup(buffer);
	char *strbuffer = strinit(1);
	strappend(&strbuffer, _buffer);
	free(_buffer);

	json_object *obj = json_object_new_object();
	json_object_object_add(obj, "username", json_object_new_string(username));
	json_object_object_add(obj, "type", json_object_new_string("message"));
	json_object_object_add(obj, "data", json_object_new_string(strbuffer));

	json_object *bucketarr = ipc_get_array("recvmsgbucket");
	json_object_array_add(bucketarr, obj);
	ipc_put_array("recvmsgbucket", bucketarr);
}

int array_sort_fn(const void *j1, const void *j2) {
	json_object *const *jso1, *const *jso2;

	jso1 = (json_object *const *)j1;
	jso2 = (json_object *const *)j2;
	if (!*jso1 && !*jso2)
		return 0;
	if (!*jso1)
		return -1;
	if (!*jso2)
		return 1;

	json_object *j1createdat = json_object_object_get(*jso1, "createdAt");
	json_object *j2createdat = json_object_object_get(*jso2, "createdAt");

	const char *createdat1 = json_object_get_string(j1createdat);
	const char *createdat2 = json_object_get_string(j2createdat);
	if (createdat1 == NULL && createdat2 == NULL)
		return 0;
	if (createdat1 == NULL)
		return -1;
	if (createdat2 == NULL)
		return 1;

	if (logdebug_if("cclient-recvbucket")) {
		logdebug("createdAt1 is %s and createdAt2 is %s\n", createdat1, createdat2);
	}
	int result = (int)strtol(createdat1, NULL, 10) - strtol(createdat2, NULL, 10);

	return result;
}

// Up until now, none of the messages had any line endings.
// No line-endings from chatnet-server. No line-endings from linenoise.
// Recvbucket_get() adds line-endings between messages! \r\n
char *recvbucket_get() {
	char *result = strinit(1);
	json_object *recvarr = ipc_get_array("recvmsgbucket");
	ipc_put_array("recvmsgbucket", json_object_new_array()); // empty array
	int arrlen = json_object_array_length(recvarr);
	if (arrlen == 0) {
		json_object_put(recvarr);
		return result;
	}

	// sort
	if (logdebug_if("cclient-recvbucket"))
		logdebug("recvmsgbucket length is %d\n", arrlen);
	json_object_array_sort(recvarr, *array_sort_fn);

	// iterate over and create a single string
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

char *get_chatnet_prompt() {
	char *username = get_chatnet_username();

	char *mic = NULL;

	char *result = strinit(1);
	strappend(&result, username);
	strappend(&result, ": ");
	return result;
}

int main(int argc, char *argv[]) {
	(void)argc;
	sc_log_set_thread_name("thread-main");
	logdebug("hi\n");
	GC_INIT();

	bool is_sioclient_up = sioclient_is_already_up();
	if (!is_sioclient_up) {
		createnewipc();
		initnewipc();
	}

	// check if config file exists, create if none
	char *configfile = getconfigfile();
	if (!entexists(configfile)) {
		file_write(configfile, "{}");
	}
	// check if auth exists in config
	if (!config_get_is_key("auth")) {
		config_put_string("auth", "");
	}

	if (!is_sioclient_up)
		sioclientinit(argv[0]);
	// atexit(sioclientcleanup);

	while (1) {
		bool _break = false;
		char *linenoise_prompt = get_chatnet_prompt();
		long last_dead_probe = 0;

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
			tv.tv_usec = 1000 * 10; // 10ms

			retval = select(ls.ifd + 1, &readfds, NULL, NULL, &tv);
			if (retval == -1) {
				perror("select()");
				exit(1);
			} else if (retval) {
				line = linenoiseEditFeed(&ls);
				if (line != linenoiseEditMore && ls.len > 0) // disallow empty editfeed
					break;
			} else {
				char *output = NULL;

				// check whether sioclient died 💀
				long nowinms = datenowms();
				if ((nowinms - last_dead_probe) > 0) {
					ipc_put_string("lastping-cclient", long_to_string(nowinms));

					if (ipc_get_is_key("lastping-sioclient")) {
						long lastping = strtol(ipc_get_string("lastping-sioclient"), NULL, 10);

						if ((nowinms - lastping) > 30000) { // 7 sec, death confirmed 💀
							_break = true;
							output = strinit(1);
							strappend(&output, "chatnet: exiting, you were disconnected :(\r\n");
						}
						last_dead_probe = nowinms;
					}
				}

				if (!_break)
					output = recvbucket_get();
				if (!strlen(output))
					continue;
				linenoiseHide(&ls);
				char *rendered_output = markdown_to_ansi(output);
				printf("%s\r\n", rendered_output); // for some reason, the last \r\n is being ommitted ¯\_(ツ)_/¯
				fflush(stdout);
				linenoiseShow(&ls);

				if (_break)
					break;
			}
		}

		linenoiseHide(&ls); // midnqp: hiding current buffer because this will go through recvbucket
		linenoiseEditStop(&ls);

		char *linenoise_buffer = buf;

		if (strcmp(linenoise_buffer, "/exit") == 0) {
			// ipc_put_boolean("userstate", false);
			_break = true;
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
		} else if (strncmp(linenoise_buffer, "/mic", 4) == 0) {
			char *string = strinit(16 + 1);
			int i = 0;
			int cmdlen = 4 + 1;
			for (i = cmdlen; i < 16 + cmdlen; i++) {
				char c = linenoise_buffer[i];
				if (c == '\0')
					break;
				if (isalpha(c))
					string[i - cmdlen] = tolower(c);
			}
			string[i - cmdlen] = '\0';

			ipc_put_string("voiceMessage", string);
		} else {
			sendbuckets_add(linenoise_buffer);
			recvbuckets_add(linenoise_buffer); // midnqp: for markdown!
		}

		if (line != NULL) {
			linenoiseFree(line);
			line = NULL;
		}

		if (_break)
			break;
	}

	logdebug("bye\n");
}
