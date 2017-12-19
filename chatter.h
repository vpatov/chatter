#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "threadpool.h"
#include "debug.h"

#define SA 			struct sockaddr
#define SAin 		struct sockaddr_in
#define MAX_RECV	1024
#define MAX_SEND	1024


#define ERR_WRONG_VERB 	-100
#define ERR_NO_RN		-101

//server's constant strings
extern const char *server_help_message;

//client's constant strings
extern const char *client_help_message;


extern const char *aloha_verb;
extern const char *ahola_verb;
extern const char *iam_verb;
extern const char *iamnew_verb;
extern const char *newpass_verb;
extern const char *err_verb;
extern const char *rn;


enum verbs{ALOHA, AHOLA, IAM, IAMNEW, NEWPASS, ERR};
extern const char *VERBS[];

