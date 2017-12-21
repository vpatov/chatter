#include "chatter.h"

const char* server_help_message = 	"Usage:\n./server [-he] [-N num] PORT_NUMBER MOTD CONFIG_FILE\n\n"
									"-e\t\t\tEcho messages received on server's stdout.\n"
									"-h\t\t\tDisplays this help menu.\n"
									"-N num\t\t\tSpecifies maximum number of chat rooms allowed on server. (Default = 5)\n"
									"PORT_NUMBER\t\tPort number to listen on. (Must be non-zero)\n"
									"MOTD\t\t\tMessage to display to the client when they connect.\n"
									"CONFIG_FILE\t\tPath to the configuration file for the threadpool.\n";


const char* client_help_message = 	"Usage:\n./client [-hc] NAME SERVER_IP SERVER_PORT\n\n"
									"-c\t\t\tRequests to server to create a new user NAME\n"
									"-h\t\t\tDisplays this help menu.\n"
									"NAME\t\t\tUsername to use when logging in. "
									"If -c option is set, will attempt to create new user with specified name.\n"
									"SERVER_IP\t\t\tIP address of server to connect to.\n"
									"SERVER_PORT\t\tPort number to listen on. (Must be non-zero)\n";


const char* password_rules =		"Please enter a password. (Must be at least 5 characters, "
									"have at least one uppcase letter, one number, and one symbol.\n"
									"Acceptable passwords follow regex: [!@#$0-9a-zA-Z]+\n";

const char *accounts_filename = "accounts";



const char *space = " ";
const char *sprn = " \r\n";
const char *rn = "\r\n";
const char *r = "\r";

const char *verbs[] = {
	"ALOHA!", "!AHOLA", "IAM", "IAMNEW","HI", "HINEW", "AUTH", 
	"PASS", "NEWPASS", "ERR", "BYE", "MSG", 
	"CREATER", "RETAERC",
	"CREATEP", "PETAERC",
	"LISTR", "RTSIL",
	"LISTU", "UTSIL",
	"JOIN","NIOJ",
	"JOINP","PNIOJ",
	"LEAVE","EVAEL",
	"KICK","KCIK",
	"TELL","LLET",
	"ECHO", "ECHOP","ECHOR",
	"NOP", "QUIT"
};

