// File: chatnet-client.cpp
// The wrapper over lib-chatnet-client to provide a user interface.


#include "./lib-chatnet-client.cpp"


#define _MBCS
#define CHATNETKW "chatnet"

#define _chatnet "[chatnet]"
#define chatnet MGN _chatnet R0

#define _exmp "[Example]"
#define exmp _MGN _exmp _R0


void __printfAllCmds__() {
	char* uSend = read_uSend();
	printf(
		"%s--------%sCHATNET COMMANDS%s--------%s\n"
		"%sCommands%s    %sDescription%s\n"
		"list        List all active users in the network\n"
		"read        Starts reading all your messages\n"
		"write       Starts input prompt for writing messages\n"
		"exit        Exits from the Chatnet network\n"
		"\n"
		"%s\n"
		"%s >> chatnet list\n"
		"%s >> chatnet read\n"
		"%s >> chatnet write\n"
		"%s >> chatnet exit\n"
		"%s--------------------------------%s\n\n\n\n"

		, BLU, GRN, BLU, R0
		, GRY, R0, GRY, R0





		, exmp, uSend
		, uSend
		, uSend
		, uSend
		, BLU, R0);
	free(uSend);
}

void notice(const char* what);
void __printfMsgExample__();
void chatnet_execCmd(char* msgText);
int isActive_Valid_uRecv(const char* uRecv);

void chatnet_init() {

	if (!dir_exists(cdir)) {
#ifdef _WIN32
		CreateDirectory(cdir, NULL);
#else
		mkdir(cdir, 0755);
#endif
	}


	if (!file_exists(uSendDir)) {
		char* uSend = input("[Init] Enter username: ");
		file_write(uSendDir, uSend);
		free(uSend);
	}


	__printfAllCmds__();


	//TODO ASCII Art
	//if (! file_exists(uRecvAllFn)) file_write(uRecvAllFn, "");
}


void chatnet_read() {
	notice("read_AllMsg");
	file_write(readingAlreadyFn, "");
	while (1) {
		if (!file_exists(readingAlreadyFn)) exit(0);

		char* MsgAll = read_AllMsg();
		if (str_eq(MsgAll, "")) {
			//failed, no msg recvd
#ifdef _WIN32
			Sleep(1);
#else
			sleep(1);
#endif
			//printf("--nothing received\n");
		}
		else printf("%s", MsgAll);
	}
}


void chatnet_write() {
	notice("write_ThisMsg");
	char* uSend = read_uSend();

	while (1) {
		char *add = str_addva(uSend, " >> ");
		char* msgText = input(add);
		int canWrite = true; // Every baby is born Muslim (submitting to the best words of Allah).

		char* uRecv = read_uRecvFromMsg(msgText);
		if (str_eq(uRecv, CHATNETKW)) {
			chatnet_execCmd(msgText);
			canWrite = false;
		}


		if (canWrite == true && isActive_Valid_uRecv(uRecv) == false) { //true & true
			notice("inactive_invalid_uRecv");
			__printfMsgExample__();
			canWrite = false;
		}
		if (canWrite == true && file_exists(str_addva(shkeyDir, "/", uRecv)) == false) {
			notice("write_chatroom");
			write_chatroom(uRecv);
		}
		if (canWrite == true) write_ThisMsg(msgText);

		
		// freeing for each time
		free(uSend);
		free(add);
		free(msgText);
	}

	// The needs to break in order to free the memory it contained.
	// We can't just exit, without freeing memory!
}


void chatnet_execCmd(char* msgText) {
	char* actives = read_active();
	free(actives);

	int lenKeyword = (int)strlen(CHATNETKW);
	//int nextSpace = str_index(msgText, " ", lenKeyword + 1, strlen(msgText));
	char* cmd = str_slice(msgText, lenKeyword + 1, 1, strlen(msgText));
	//printf("%s Executing chatnet-native command: %s\n", info, cmd);

	if (str_eq("exit", cmd)) write_exit();
	else if (str_eq("list", cmd)) printf("%s", read_active());
	else if (str_eq("read", cmd)) chatnet_read();
	else if (str_eq("write", cmd)) chatnet_write();
	free(cmd);
}



void __printfMsgExample__() { printf("%s[Example]%s %sNameOfUser%s The first word in every message is the receiver's username. In this case the receiver is %s'NameOfUser'%s\n", MGN, R0, CYN, R0, CYN, R0); }


void notice(const char* about) {
	// see that's how you encapsulate in C, object-orientation in C
	if (str_eq(about, "write_chatroom")) {
		printf("%s Creating new chatroom...\n", info);
	}
	else if (str_eq(about, "read_AllMsg")) {
		printf("%s Please run Chatnet.exe again to write messages.\n", warn);
		__printfMsgExample__();
		printf("%s Starting to receive all messages in this terminal window.\n", info);
	}
	else if (str_eq(about, "inactive_invalid_uRecv")) {
		printf("%s[Message not send]%s Either uRecv %s(the first word in your message)%s is invalid %s(contains anything other than alphabet)%s, or the recipient user is %sinactive%s right now.\n", RED, R0, GRY, R0, GRY, R0, GRY, R0);
	}
}


int isActive_Valid_uRecv(const char* uRecv) {
	/*char* uRecvAll = file_read(uRecvAllFn);
	if (str_index(uRecvAll, str_addva("_", uRecv, "_"), 0, strlen(uRecvAll)) != -1)
		return 1; //true
	else return 0;
	*/
	char* activeAll = read_active();
	char* _uRecv_ = str_addva("[+] ", uRecv, "\n");
	if (str_index(activeAll, _uRecv_, 0, strlen(activeAll)) != -1) {
		// uRecv is active
		if (str_isalpha(uRecv)) return 1; //true. Perfect!
		else return 0;
	}
	else return 0;
}


int main() {
#ifdef _WIN32
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
		return GetLastError();

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
		return GetLastError();

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
		return GetLastError();
#endif





	chatnet_init();

	/*
	if (! file_exists(readingAlreadyFn)) {
		chatnet_read();
	}
	else chatnet_write();
	*/
	chatnet_write();
}
