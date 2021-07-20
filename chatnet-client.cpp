// File: chatnet-client.cpp
// The wrapper over lib-chatnet-client to provide a user interface.


#include "./lib-chatnet-client.cpp"


#define _MBCS
#define CHATNETKW "chatnet"

#define _chatnet "[chatnet]"
#define chatnet MGN _chatnet R0

#define _exmp "[Example]"
#define exmp _MGN _exmp _R0


void notice(const char* what);
void __printfAllCmds__() {
	char* uSend = read_uSend();
	printf(
		"%s------------%sCHATNET COMMANDS%s------------%s\n"
		"%s[Commands]%s    %s[Description]%s\n"
		"list      List all active users in the network\n"
		"read      Starts reading all your messages\n"
		//"write     Starts input prompt for writing messages\n"
		"exit      Exits from the Chatnet network\n"
		"\n"
		"%s\n"
		//"%s >> chatnet list\n"
		//"%s >> chatnet read\n"
		//"%s >> chatnet write\n"
		//"%s >> chatnet exit\n"

		, BLU, GRN, BLU, R0
		, MGN, R0, MGN, R0





		, exmp
		//, uSend
		//, uSend
		//, uSend
		//, uSend
		);
		notice("example");
		printf("%s----------------------------------------%s\n\n\n\n", BLU, R0);	
	free(uSend);
}

int chatnet_execCmd(char* msgText);
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
		if (!file_exists(readingAlreadyFn)) {
			exit(0);
		}

		char* MsgAll = read_AllMsg();
		if (str_eq(MsgAll, "")) {
			//failed, no msg recvd
#ifdef _WIN32
			//Sleep(1);
#else
			//sleep(1);
#endif
			//printf("--nothing received\n");
		}
		else printf("%s", MsgAll);
		free(MsgAll);
	}
}


void chatnet_write() {
	notice("write_ThisMsg");
	char* uSend = read_uSend();

	while (1) {
		char* add = str_addva(uSend, " >> ");
		char* msgText = input(add);
		// Every baby is born Muslim (submitting to the best words of Allah).
		int canWrite = true;
		int retExec = 0; //non-exit retval


		char* uRecv = read_uRecvFromMsg(msgText);
		if (str_eq(uRecv, CHATNETKW)) {
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
			notice("example");
			canWrite = false;
		}


		char* uRecvKey = str_addva(shkeyDir, "/", uRecv);
		if (canWrite == true 
			&& file_exists(uRecvKey) == false)
		{
			notice("write_chatroom");
			write_chatroom(uRecv);
		}


		if (canWrite == true) write_ThisMsg(msgText);

		
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

	int lenKeyword = (int)strlen(CHATNETKW);
	char* cmd = str_slice(msgText, lenKeyword + 1, 1, strlen(msgText));

	if (str_eq("exit", cmd)) {
		free(cmd);
		return 1;
	}
	else if (str_eq("list", cmd)) {
		char* actives = read_active();
		printf("%s", actives); 
		free(actives);
	}
	else if (str_eq("read", cmd)) chatnet_read();
	//else if (str_eq("write", cmd)) chatnet_write();
	
	free(cmd);
	return 0;
}





void notice(const char* about) {
	// see that's how you encapsulate in C, object-orientation in C
	if (str_eq(about, "write_chatroom")) {
		printf("%s Creating new chatroom...\n", info);
	}

	else if (str_eq(about, "example")) {
		printf(
		"%s[Example]%s\n"
		"YourName >> chatnet list\n"
		_GRY "[+] midnqp\n[+] Muhammad\n[+] OtherPeople" _R0
		"YourName >> chatnet read\n"
		_GRY "[info] CHATNET now starts to receive all your incoming messages\n" 
		"[midnqp] Hello, YourName!\n[Muhammad] Hey, I just sent you a message!\n" _R0
		"YourName >> midnqp Hi, MidnQP!\n"
		"YourName >> Muhammad Yes, I've received your message!\n"

		"%sDo notice: %s The first word in every message must be the receiver's name. \n"
		"%sDo notice: %s After running `chatnet read`, you need to launch another CHATNET instance to write messages.\n"
		, MGN, R0, CYN, R0, CYN, R0);
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
		printf("%s Do not close this windows\n", warn);

		printf("%s Receiving all incoming messages in this window\n", info);
	}

	else if (str_eq(about, "inactive_invalid_uRecv")) {
		printf("%s[]%s Either uRecv %s(the first word in your message)%s is invalid %s(contains anything other than alphabet)%s, or the recipient user is %sinactive%s right now.\n", RED, R0, GRY, R0, GRY, R0, GRY, R0);
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
