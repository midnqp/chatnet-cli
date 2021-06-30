#include "libchatnet.h"


void __printfMsgExample__() {printf("%s[Example]%s %sNameOfUser%s The first word in every message is the receiver's username. In this case the receiver is %s'NameOfUser'%s\n", GRN, R0, CYN, R0, CYN, R0);}


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
	char* activeAll = file_read(activeFn);
	char* _uRecv_ = str_addva("[+] ", uRecv, "\n");
	if (str_index(activeAll, _uRecv_, 0, strlen(activeAll)) != -1) {
		// uRecv is active
		if (str_isalpha(uRecv)) return 1; //true. Perfect!
		else return 0;
	}
	else return 0;
}


int main () {
	init_chatnet();
	
	
	if (! file_exists(readingAlreadyFn)) {
		notice("read_AllMsg");
		file_write(readingAlreadyFn, "");
		while (1) {
			char* MsgAll = read_AllMsg();
			if (str_eq(MsgAll, "")) {	
				//failed, no msg recvd
				sleep(1);
				//printf("--nothing received\n");
			}
			else {
				printf("%s", MsgAll);
				strcpy(MsgAll, "");
			}
		}
	}
	else {
		notice("write_ThisMsg");
		char* uSend = read_uSend();
		while (1) {
			char* msgText = input(str_addva(uSend," >> "));
			char* uRecv = read_uRecvFromMsg(msgText);
			int canWrite = true;

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
		}
	}
}
