#include <libgen.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <json-c/json.h>
#include "deps/linenoise/linenoise.h"

#include "autofree.h"
#include "db.h"
#include "string.h"
#include "util.h"
#include "sio-client.h"

/* 0 = null
 * 1 = please exit whenever you can asap
 */
int f_thread_msg_print = 0;
/* 0 = null
 * 1 = please exit whenever you can asap
 * 2 = taking input
 * 3 = not taking input
 * 4 = writing to db
 */
int f_thread_msg_write = 0;
/*pthread_mutex_t mutex_thread_msg_write_dead = PTHREAD_MUTEX_INITIALIZER;*/
pthread_t id_thread_msg_write = 0;
/* local sendbucket to store sent messages */
char* sendbucket[64]; // so that it gets freed by os in case of crash.
int sendbucketc = 0;
char linenoise_buffer[1024*1024] = "";
const char* linenoise_prompt = ">";

void *thread_msg_print();
void *thread_msg_write();

void sendbuckets_add() {
	char* _buffer = strdup(linenoise_buffer);
	char* strbuffer = strinit(1);
	strappend(&strbuffer, _buffer);
	sendbucket[sendbucketc++] = _buffer; // freed by sendbucket_get() or by OS
	linenoise_buffer[0] = '\0';

	/*struct json_object* json = json_object_new_object();*/
	/*json_object_object_add(json, "username",json_object_new_string( "midnqp"));*/
	/*json_object_object_add(json, "type",json_object_new_string( "message"));*/
	/*json_object_object_add(json, "data",json_object_new_string(strbuffer));*/

	// read existing array, and add this at the end
	char* sendmsgbucket = leveldbget("sendmsgbucket"); // "[...]"
	/*struct json_object* sendmsgbucketarr = json_tokener_parse(sendmsgbucket);*/
	/*json_object_array_add(sendmsgbucketarr, json);*/

	/*const char* newmsgarr = json_object_to_json_string(sendmsgbucketarr);*/
	/*leveldbput("sendmsgbucket", newmsgarr);*/
	/*json_object_put(json);*/
	/*json_object_put(sendmsgbucketarr);*/
}

char* sendbucket_get() {
	char* result = strinit(1);
	for (int i=0; i<sendbucketc; i++) {
		strappend(&result, "You: ");
		strappend(&result, sendbucket[i]);
		strappend(&result, "\r\n");
		free(sendbucket[i]);
	}
	sendbucketc = 0;
	return result;
}

char* recvbucket_get() {
	return "";
	/*char* arr = leveldbget("recvmsgbucket");*/
	/*json_object* obj = json_tokener_parse(arr);*/
}

void *thread_msg_print() {
	pthread_detach(pthread_self());
	int timeoutc= 0;

	while (1) {
		char *recvmsgfmt = recvbucket_get();
		char *sendmsgfmt = sendbucket_get();
		bool f_flush = false;
		char *result = strinit(1);

		if (strcmp(recvmsgfmt, "") != 0) {
			/*strappend(&result, "friend_uid: ");*/
			strappend(&result, recvmsgfmt);
			/*strappend(&result, "\r\n");*/
			f_flush = true;
		}

		if (strcmp(sendmsgfmt, "") != 0) {
			/*strappend(&result, "muhammad: ");*/
			strappend(&result, sendmsgfmt);
			/*strappend(&result, "\r\n");*/
			f_flush = true;
		}

		if (f_flush == true) {
			if (id_thread_msg_write) {
				if (f_thread_msg_write == 2)  {
					pthread_cancel(id_thread_msg_write);
					id_thread_msg_write = 0;
					f_thread_msg_write = 0;
				}
				else if (f_thread_msg_write == 4) {
					/*f_thread_msg_write = 1;*/
					/*pthread_mutex_lock(&f1mutex_thread_msg_write);*/
					// that thread exited
					/*id_thread_msg_write = 0;*/
				/*f_thread_msg_write = 0;*/
				}
			}

			printf("\33[2K\r");
			printf("%s", result);
			fflush(stdout);
			autofree_free(result);

			/*if ( f_thread_msg_write == 1) { */
				/*pthread_mutex_unlock(&mutex_thread_msg_write_dead);*/
			/*}*/
				
			if (id_thread_msg_write == 0) {
				// only create this thread, if you had cancelled earlier
				pthread_t T3;
				pthread_create(&T3, NULL, &thread_msg_write, NULL);
			}
		} else {
			timeoutc++;
			autofree_free(result);
			sleep(1);
		}

		if (f_thread_msg_print == 1) 
			break;
		
	}

	printf("thread-print exited\n");
	return NULL;
}

void write_msg_cleanup() {
	disableRawMode(STDIN_FILENO);
	id_thread_msg_write = 0;
}


void *thread_msg_write() {
	id_thread_msg_write = pthread_self();
	pthread_detach(pthread_self());
	pthread_cleanup_push(write_msg_cleanup, NULL);

	while (1) {
		f_thread_msg_write = 2;
		linenoiseRaw(linenoise_buffer, 1024 * 1024, linenoise_prompt);
		f_thread_msg_write = 3;
		if (strcmp(linenoise_buffer, "/exit") == 0) {
			leveldbput("userstate", "false");
			f_thread_msg_print = 1;
			break;
		} else  {
			// TODO make this a thread, otherwise input is blocked
			f_thread_msg_write = 4;
			sendbuckets_add();
			f_thread_msg_write = 3;
		}
		
	}

	pthread_cleanup_pop(1);
	/*id_thread_msg_write = 0;*/
	/*f_thread_msg_write = 0;*/
	printf("thread-write exited\n");
	return NULL;
}

void disablerawmodefn() {
	disableRawMode(STDIN_FILENO);
}

int main(int argc, char *argv[]) {

	atexit(dealloc);
	at_quick_exit(disablerawmodefn);

	/*sioclientinit(argv[0]);*/


	/*pthread_t T2, T3;*/
	/*pthread_create(&T2, NULL, &thread_msg_print, NULL);*/
	/*printf("main: before pthread_create\n");*/
	/*pthread_create(&T3, NULL, &thread_msg_write, NULL);*/
	/*printf("main: before pthread_exit\n");*/

	/*pthread_exit(NULL);*/
}
