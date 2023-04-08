#include <gc/gc.h>
#include <json-c/json.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "autofree.h"
#include "db.h"
#include "deps/linenoise/linenoise.h"
#include "deps/sc/sc_log.h"
#include "sio-client.h"
#include "str.h"
#include "util.h"

/* 0 = null
 * 1 = please exit whenever you can asap
 */
int f_thprint = 0;
/* 0 = null
 * 1 = please exit whenever you can asap
 * 2 = taking input
 * 3 = not taking input
 * 4 = writing to db
 */
int f_thwrite = 0;
pthread_t id_thwrite = 0;
/* store sent messages */
char *sendbucket[1024];
int sendbucketc = 0;
char linenoise_buffer[1024 * 1024] = "";
char *linenoise_prompt = ">";
char *username = NULL;

void *thread_msg_print();
void *thread_msg_write();

void sendbuckets_add(char* buffer, const char* username) {
	char *_buffer = strdup(buffer);
	char *strbuffer = strinit(1);
	strappend(&strbuffer, _buffer);
	sendbucket[sendbucketc++] = _buffer; // freed by sendbucket_get() or by OS
	linenoise_buffer[0] = '\0';

	json_object *json = json_object_new_object();
	json_object_object_add(json, "username", json_object_new_string(username));
	json_object_object_add(json, "type", json_object_new_string("message"));
	json_object_object_add(json, "data", json_object_new_string(strbuffer));
	logdebug("source object: %s\n", json_object_to_json_string(json));

	// read existing array, and add this at the end
	char *bucket = leveldbget("sendmsgbucket"); // "[...]"
	logdebug("sendbuckets_add: leveldbget:%zu %s\n", strlen(bucket), bucket);
	json_object *bucketarr = json_tokener_parse(bucket);
	json_parse_check(bucketarr, bucket);
	json_object_array_add(bucketarr, json);

	const char *dbarr = json_object_to_json_string(bucketarr);
	leveldbput("sendmsgbucket", dbarr);
	json_object_put(bucketarr);
}

char *sendbucket_get() {
	char *result = strinit(1);
	for (int i = 0; i < sendbucketc; i++) {
		strappend(&result, "You: ");
		strappend(&result, sendbucket[i]);
		strappend(&result, "\r\n");
		autofree_free(sendbucket[i]);
	}
	sendbucketc = 0;
	return result;
}

char *recvbucket_get() {
	char *result = strinit(1);
	char *arr = leveldbget("recvmsgbucket");
	if (strlen(arr) < 3)
		return "";
	leveldbput("recvmsgbucket", "[]");
	json_object *arrj = json_tokener_parse(arr);
	json_parse_check(arrj, arr);
	int len = json_object_array_length(arrj);
	for (int i = 0; i < len; i++) {
		json_object *o = json_object_array_get_idx(arrj, i);
		json_parse_check(o, arr);
		json_object *jusername = json_object_object_get(o, "username");
		json_parse_check(jusername, arr);
		const char *sender = json_object_get_string(jusername);
		if (strcmp(sender, username) == 0)
			continue;
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

void *thread_msg_print() {
	sc_log_set_thread_name("thread-msg-print");
	logdebug("started thread-msg-print\n");
	char *result = strinit(1);

	while (1) {
		if (f_thprint == 1)
			break;

		usleep(500 * 1000); // 500ms

		char *recvmsgfmt = recvbucket_get();
		bool doflush = false;

		if (strcmp(recvmsgfmt, "") != 0) {
			strappend(&result, recvmsgfmt);
			doflush = true;
		}

		if (!doflush)
			continue;

		if (id_thwrite && f_thwrite == 2) {
			pthread_cancel(id_thwrite);
			id_thwrite = 0;
			f_thwrite = 0;
		}

		printf("\33[2K\r");
		printf("%s", result);
		fflush(stdout);
		strcpy(result, "");

		if (id_thwrite == 0) {
			// only create this thread, if you had cancelled earlier
			pthread_t T3;
			pthread_create(&T3, NULL, &thread_msg_write, NULL);
		}
	}

	autofree_free(result);
	logdebug("ended thread-msg-print\n");
	return NULL;
}

void write_msg_cleanup() {
	disableRawMode(STDIN_FILENO);
	id_thwrite = 0;
}

void *thread_msg_write() {
	id_thwrite = pthread_self();
	sc_log_set_thread_name("thread-msg-write");
	logdebug("started thread-msg-write\n");
	pthread_cleanup_push(write_msg_cleanup, NULL);

	while (1) {
		f_thwrite = 2;
		linenoiseRaw(linenoise_buffer, 1024 * 1024, linenoise_prompt);
		f_thwrite = 3;

		if (strcmp(linenoise_buffer, "/exit") == 0) {
			leveldbput("userstate", "false");
			f_thprint = 1;
			break;
		} else if (strncmp(linenoise_buffer, "/name", 5) == 0) {
			char *username = strinit(strlen(linenoise_buffer));
			size_t len = strlen(linenoise_buffer) - 5;
			// TODO check: must be all alphabets, and <= 16 char long
			strncpy(username, linenoise_buffer + 6, len);
			username[len] = '\0';
			leveldbput("username", username);
			linenoise_prompt = username;
			linenoise_buffer[0] = '\0';
		} else {
			f_thwrite = 4;
			sendbuckets_add(linenoise_buffer, username);
			f_thwrite = 3;
		}
	}

	pthread_cleanup_pop(1);
	logdebug("ended thread-msg-write\n");
	return NULL;
}

int main(int argc, char *argv[]) {
	(void)argc;
	sc_log_set_thread_name("thread-main");
	logdebug("hi\n");
	GC_INIT();

	createnewdb();
	initnewdb();
	username = leveldbget("username");

	char *prompt = strinit(1);
	strappend(&prompt, username);
	strappend(&prompt, ": ");
	linenoise_prompt = prompt;

	sioclientinit(argv[0]);
	atexit(sioclientcleanup);

	pthread_t T2, T3;
	pthread_create(&T2, NULL, &thread_msg_print, NULL);
	pthread_create(&T3, NULL, &thread_msg_write, NULL);
	pthread_join(T2, NULL);
	pthread_join(T3, NULL);
	logdebug("bye!\n");
}
