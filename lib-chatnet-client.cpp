// File: lib-chatnet.cpp
// Contains client-side code to communicate with server.

// Copyright (C) 2021 Muhammad Bin Zafar <midnightquantumprogrammer@gmail.com>
// Licensed under the MIT License: https://opensource.org/licenses/mit-license.php




#include "include/curl/curl.h"
#include "include/lib-cpython-builtins.cpp"

const char* err = "\033[0;31m[Error]\033[0m";
const char* info = "\033[0;32m[Info]\033[0m";
#define log  _GRY "[Log]" _R0
#define warn _YLW "[Warning]" _R0

const char* cdir = "./chatnet.cache";
#define uSendDir str_addva(cdir, "/_uSend_")
#define shkeyDir str_addva(cdir, "")
#define uRecvAllFn str_addva(cdir, "/_uRecvAll_") //TODO Store chatlogs in cdir/uRecvFn
#define readingAlreadyFn str_addva(cdir, "/read_AllMsg")
#define activeFn str_addva(cdir, "/_active_")

const char* NETADDR = "http://yuva.life/wp-admin/net.php";
// NETADDR could be any address where `net.php` is stored.
//const char* NETADDR = "http://localhost/network.php";

#define PERFORMCURL_FAILED "--SYMBOL_PERFORMCURL_FAILED--"
#define WRITEMSG_FAILED "--SYMBOL_WRITEMSG_FAILED--"


char* serverComm(const char* postData);


char* read_uSend() {
	char* uSend = file_read(uSendDir);
	//uSend[strlen(uSend) - 1] = '\0';
	return uSend;
}


char* read_shkey(const char* uRecv) {
	return file_read(str_addva(shkeyDir, "/", uRecv));
}


size_t curl_writefunc_callback(void* p, size_t size, size_t count, struct string* cResp) {
	size_t newLen = cResp->len + size * count;
	cResp->str = (char*)realloc(cResp->str, newLen + 1);
	if (cResp->str == NULL) { printf("curl_writefunc() failed\n"); exit(1); }
	memcpy(cResp->str + cResp->len, p, size * count);
	cResp->str[newLen] = '\0';
	cResp->len = newLen;
	return size * count;
}


char* __performCurl__(const char* PostData) {
	curl_global_init(CURL_GLOBAL_DEFAULT);
	CURL* curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, NETADDR);
		//struct curl_slist* headers = NULL;
		//headers = curl_slist_append(headers, "Content-Type: application/json");
		//curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, PostData);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writefunc_callback);

		struct string cwfResp; //curl write func : response
		str_init(&cwfResp);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cwfResp); //Address of cwfResp (&cwfResp), not just (cwfResp). Else: Errors!!!!

		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			// unexpected failure
			return (char*)PERFORMCURL_FAILED;
		}
		curl_easy_cleanup(curl);
		curl_global_cleanup();
		return cwfResp.str;  //Never raw json. Just strings.
	}
	else {
		printf("%s Couldn't init performCurl.\n", err);
		exit(1);
	}
}


char* serverComm(const char* postData) {
	//printf("%s %s\n", log, postData);

	for (int i = 0; i < 10; i++) {
		char* serverResponse = __performCurl__(postData);
		if (!str_eq(serverResponse, PERFORMCURL_FAILED)) {
			return serverResponse; //success. break.
		}
		else if (str_eq(serverResponse, PERFORMCURL_FAILED)) {
			printf("%s Connection to server dropped. Retrying...\n", warn);
#ifdef _WIN32
			Sleep(1);
#else
			sleep(1);
#endif
		}
	}
	printf("%s Connection dropped permanently. Check your internet connection.\n", err);
	exit(-1);
}


char* read_AllMsg() {
	char* post = str_addva("--read_AllMsg ", read_uSend());
	return serverComm(post);
}


char* read_uRecvFromMsg(const char* msgText) {
	int firstSpace = str_index(msgText, " ", 0, strlen(msgText));
	return str_slice(msgText, 0, 1, firstSpace);
}


void read_uRecvAll() {
	char* uRecvAll = serverComm(str_addva("--read_uRecvAll ", read_uSend()));
	file_write(uRecvAllFn, uRecvAll);
}


char* read_active() {
	char* activeAll = serverComm(str_addva("--read_active ", read_uSend()));
	file_write(activeFn, activeAll);
	return activeAll;
}


void write_chatroom(const char* uRecv) {
	//printf("----libchatnet.h: Writing chatroom----\n");
	char* uSend = read_uSend();
	char* uRecvFn = serverComm(str_addva("--write_chatroom ", uSend, " ", uRecv));
	file_write(str_addva(shkeyDir, "/", uRecv), uRecvFn);
}


char* write_ThisMsg(const char* msgText) {
	char* uRecv = read_uRecvFromMsg(msgText);

	char* chatroomFn = read_shkey(uRecv);
	//char* chatroomFn = str_addva(uRecv, "-", shkey);

	msgText = str_replace(msgText, uRecv, "", 0, strlen(uRecv));

	char* post = str_addva("--write_ThisMsg ", read_uSend(), " ", chatroomFn, msgText, "\n");

	return serverComm(post);
	// This part needs checking...
}


void write_exit() {
	file_remove(readingAlreadyFn);

	serverComm(str_addva("--write_exit ", read_uSend()));
	printf("%s Exited chatnet network.\n", info);
	exit(0);
}
