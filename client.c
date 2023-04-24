#include "deps/linenoise/linenoise.h"
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

void sendbuckets_add(char *buffer, const char *username) {
	char *_buffer = strdup(buffer);
	char *strbuffer = strinit(1);
	strappend(&strbuffer, _buffer);
	free(_buffer);
	/*linenoise_buffer[0] = '\0';*/

	json_object *json = json_object_new_object();
	json_object_object_add(json, "username", json_object_new_string(username));
	json_object_object_add(json, "type", json_object_new_string("message"));
	json_object_object_add(json, "data", json_object_new_string(strbuffer));
	logdebug("source object: %s\n", json_object_to_json_string(json));

	// read existing array, and add this at the end
	char *bucket = ipc_get("sendmsgbucket"); // "[...]"
	logdebug("sendbuckets_add: leveldbget:%zu %s\n", strlen(bucket), bucket);
	json_object *bucketarr = json_tokener_parse(bucket);
	json_parse_check(bucketarr, bucket);
	json_object_array_add(bucketarr, json);

	const char *dbarr = json_object_to_json_string(bucketarr);
	ipc_put("sendmsgbucket", dbarr);
	json_object_put(bucketarr);
}

char *recvbucket_get() {
	char *result = strinit(1);
	char *arr = ipc_get("recvmsgbucket");
	if (strlen(arr) < 3)
		return "";
	ipc_put("recvmsgbucket", "[]");
	json_object *arrj = json_tokener_parse(arr);
	json_parse_check(arrj, arr);
	int len = json_object_array_length(arrj);
	for (int i = 0; i < len; i++) {
		json_object *o = json_object_array_get_idx(arrj, i);
		json_parse_check(o, arr);
		json_object *jusername = json_object_object_get(o, "username");
		json_parse_check(jusername, arr);
		const char *sender = json_object_get_string(jusername);
		json_object *jdata = json_object_object_get(o, "data");
		json_parse_check(jdata, arr);
		const char *message = json_object_get_string(jdata);
		strappend(&result, sender);
		strappend(&result, ": ");
		strappend(&result, message);
		strappend(&result, "\r\n");
	}
	json_object_put(arrj);
	return result;
}

int main(int argc, char *argv[]) {
	(void)argc;
	sc_log_set_thread_name("thread-main");
	logdebug("hi\n");
	GC_INIT();

	createnewipc();
	initnewipc();
	username = ipc_get("username");

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
			tv.tv_usec = 1000 * 500; // 500ms

			retval = select(ls.ifd + 1, &readfds, NULL, NULL, &tv);
			if (retval == -1) {
				perror("select()");
				exit(1);
			} else if (retval) {
				line = linenoiseEditFeed(&ls);
				if (line != linenoiseEditMore)
					break;
			} else {
				// Timeout occurred
				linenoiseHide(&ls);
				printf("%s", recvbucket_get());
				linenoiseShow(&ls);
			}
		}

		linenoiseEditStop(&ls);

		bool _break = false;
		char *linenoise_buffer = buf;
		if (strcmp(linenoise_buffer, "/exit") == 0) {
			ipc_put("userstate", "false");
			_break = true;
			// break; // because `char* line` needs to be freed
		} else if (strncmp(linenoise_buffer, "/name", 5) == 0) {
			char *uname = strinit(16 + 1);
			int j = 0;
			for (int i = 6; (i < 16 + 6 || linenoise_buffer[i] != '\0'); i++)
				uname[j++] = linenoise_buffer[i];

			uname[j] = '\0';
			ipc_put("username", uname);
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
