/**
 * File :: Chatnet Client
 * The wrapper over lib-chatnet-client to provide a user interface.
 *
 * Copyright (C) 2021 Muhammad Bin Zafar <midnightquantumprogrammer@gmail.com>
 * Licensed under GPLv2
 **/
#pragma once
#include "lib-chatnet-client.h"


#define CHATNET_CMD_KW "chatnet"
int chatnet_execCmd(char* msgText);
int isActive_Valid_uRecv(const char* uRecv);
void notice(const char* what);
void chatnet_sleep(int seconds);



void chatnet_sleep(int seconds) {
	#ifdef _WIN32
	Sleep(seconds);
	#else 
	sleep(seconds);
	#endif
}

void chatnet_init() {
	notice("init");
	

	if (!dir_exists(cdir)) {
		#ifdef _WIN32
		CreateDirectory(cdir, NULL);
		#else
		mkdir(cdir, 0755);
		#endif
	}


	if (!file_exists(uSendDir)) {
		char* uSend = input(init " Enter username: ");
		file_write(uSendDir, uSend);
		free(uSend);
	}


	notice("cmd-not-found");
}


void chatnet_read() {
	notice("read_AllMsg");
	file_write(readingAlreadyFn, "");
	while (1) {
		if (!file_exists(readingAlreadyFn)) {
			printf("%s Exited chatnet network.\n", info);
			exit(0); //no leaks
		}

		char* MsgAll = read_AllMsg();
		if (str_eq(MsgAll, "")) {
			// No msg recvd
		}
		else printf("%s", MsgAll);
		free(MsgAll);
	}
}


void chatnet_write() {
	notice("read off write on");
	char* uSend = read_uSend();

	while (1) {
		char* add = str_addva("\033[94m", uSend, _R0, " ", "\033[91m", ">", "\033[94m", ">", "\033[92m", ">", _R0, " ");
		char* msgText = input(add);
		// Every baby is born Muslim (submitting to the best words of Allah).
		int canWrite = true;
		int retExec = 0; //non-exit retval


		char* uRecv = read_uRecvFromMsg(msgText);
		if (str_eq(uRecv, CHATNET_CMD_KW)) {
			retExec = chatnet_execCmd(msgText);
			if (retExec == 1) {
				//Preparing for exit
				//Doesn't actually exit
				//Lets the loop break.
				write_exit();
			};
			canWrite = false;
		}


		if (canWrite == true 
			&& isActive_Valid_uRecv(uRecv) == false) 
		{
			notice("inactive_invalid_uRecv");
			//notice("example");
			canWrite = false;
		}


		char* uRecvKey = str_addva(shkeyDir, "/", uRecv);
		if (canWrite == true 
			&& file_exists(uRecvKey) == false)
		{
			notice("write_chatroom");
			write_chatroom(uRecv);
		}


		if (canWrite == true) {
			char* tmp = write_ThisMsg(msgText);
			free(tmp);
		}

		
		// freeing for each time
		free(add);
		free(msgText);
		free(uRecv);
		free(uRecvKey);

		if (canWrite == false && retExec == 1) break;
	}

	free(uSend);
}


int chatnet_execCmd(char* msgText) {
	// If cmd is 'exit' return 1
	// Else always return 0

	char* actives = read_active();
	free(actives);

	int lenKeyword = (int)strlen(CHATNET_CMD_KW);
	char* cmd = str_slice(msgText, lenKeyword + 1, 1, strlen(msgText));

	if (str_eq("example", cmd)) {
		free(cmd);
		notice("example");
	}
	else if (str_eq("exit", cmd)) {
		free(cmd);
		return 1;
	}
	else if (str_eq("list", cmd)) {
		char* actives = read_active();
		printf("%s", actives); 
		free(cmd);
		free(actives);
	}
	else if (str_eq("read", cmd)) {
		free(cmd);
		chatnet_read(); //You can exit if you want.
	}
	
	else {
		notice("cmd-not-found");
		free(cmd);
	}
	return 0;
}





void notice(const char* about) {
	if (str_eq(about, "write_chatroom")) {
		printf("%s Creating new chatroom...\n", info);
	}


	else if (str_eq(about, "init")) {
		printf("\n");
#ifdef _WIN32
printf("%s\
   _____ _    _       _______ _   _ ______ _______ \n\
  / ____| |  | |   /\\|__   __| \\ | |  ____|__   __|\n\
 | |    | |__| |  /  \\  | |  |  \\| | |__     | |   \n\
 | |    |  __  | / /\\ \\ | |  | . ` |  __|    | |   \n\
 | |____| |  | |/ ____ \\| |  | |\\  | |____   | |   \n\
  \\_____|_|  |_/_/    \\_\\_|  |_| \\_|______|  |_|   \n\
%s\
", _GRN, _R0);
#else
printf("%s\
░█████╗░██╗░░██╗░█████╗░████████╗███╗░░██╗███████╗████████╗ \n\
██╔══██╗██║░░██║██╔══██╗╚══██╔══╝████╗░██║██╔════╝╚══██╔══╝ \n\
██║░░╚═╝███████║███████║░░░██║░░░██╔██╗██║█████╗░░░░░██║░░░ \n\
██║░░██╗██╔══██║██╔══██║░░░██║░░░██║╚████║██╔══╝░░░░░██║░░░ \n\
╚█████╔╝██║░░██║██║░░██║░░░██║░░░██║░╚███║███████╗░░░██║░░░ \n\
░╚════╝░╚═╝░░╚═╝╚═╝░░╚═╝░░░╚═╝░░░╚═╝░░╚══╝╚══════╝░░░╚═╝░░░ %s\n", _GRN, _R0);
#endif
		printf("A hidden computer network under a single-file at any website.\n\
        Original Author : Muhammad Bin Zafar  @MidnQP\n\
        Repository      : www.github.com/MidnQP/TerminalChat \n\
\n\
");
		chatnet_sleep(1);
		printf(
		"%sCommands%s    %sDescription%s\n"
		"list        List all active users\n"
		"read        Start receiving all incoming messages\n"
		"exit        Exit from CHATNET\n"
		"example     Learn how to use this software\n"
		"\n\n"

		, MGN, R0, MGN, R0);	
	}

	
	else if (str_eq(about, "cmd-not-found")) {
		printf(init " Type " _GRY "chatnet example" _R0 
		", and press " _GRY "Enter" _R0 
		" - to see a tutorial!\n"
		);
	}

	else if (str_eq(about, "example")) { printf(
		_GRY "---------------------%sExample%s-----------------------" _R0 "\n"
		"YourName >> chatnet list\n"
		_GRY "[+] friend1\n[+] friend2\n[+] friend3\n[+] YourName\n" _R0

		"YourName >> chatnet read\n"
		_GRY "[Info] CHATNET now starts to receive all your incoming messages\n" 
		"[friend1] Hello, YourName!\n[friend2] I just sent my first message!!\n" _R0
		
		"YourName >> friend1 Peace be upon you!\n"
		"YourName >> friend2 Yes, I received. But, gotta go now.\n"
		"Yourname >> chatnet exit\n"
		"%s[Info] Exited CHATNET%s\n\n"

		"%sNotice in example:%s The first word in every message must be the receiver's name. \n"
		"%sNotice in example:%s After running `chatnet read`, you need to launch another CHATNET.exe instance to write messages.\n"
		_GRY "--------------------------------------------------" _R0 "\n"

		, MGN, _GRY
		, GRY, R0
		, CYN, R0
		, CYN, R0);
	}


	else if (str_eq(about, "read on write off")) {		
		printf("%s WRITE_MODE deactivated\n", info);
		printf("%s READ_MODE activated\n", info);
	}


	else if (str_eq(about, "read off write on")) {
		printf("%s READ_MODE deactivated\n", info);
		printf("%s WRITE_MODE activated\n", info);
	}


	else if (str_eq(about, "read_AllMsg")) {
		notice("read on write off");

		printf("%s You cannot send messages using this window anymore\n", warn);
		printf("%s Please launch another CHATNET.exe to write messages\n", warn);
		printf("%s Do not close this window\n", warn);

		printf("%s Receiving all incoming messages in this window\n", info);
	}


	else if (str_eq(about, "inactive_invalid_uRecv")) {
		printf("%s Receiving-user must be " _GRY "the first word in your message." _R0 " Is the person listed active?\n", err);
	}
}


int isActive_Valid_uRecv(const char* uRecv) {
	char* activeAll = read_active();
	char* _uRecv_ = str_addva("[+] ", uRecv, "\n");
	if (str_index(activeAll, _uRecv_, 0, strlen(activeAll)) != -1) {
		// uRecv is active
		if (str_isalpha(uRecv)) {
			free(activeAll);
			free(_uRecv_);
			return 1; //true. Perfect!
		}
		else {
			free(activeAll);
			free(_uRecv_);
			return 0;
		}
	}
	else {
		free(activeAll);
		free(_uRecv_);
		return 0;
	}
}


int main() {
	#ifdef _WIN32
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE) return GetLastError();

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode)) return GetLastError();

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode)) return GetLastError();
	#endif





	chatnet_init();
	chatnet_write();
}
