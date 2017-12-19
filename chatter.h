#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include "threadpool.h"
#include "debug.h"

#define SA 				struct sockaddr
#define SAin 			struct sockaddr_in
#define MAX_RECV		1024
#define MAX_SEND		1024
#define MAX_USERNAME	32
#define MAX_PASSWORD	32

extern const char *server_help_message;
extern const char *client_help_message;
extern const char *password_rules;
extern const char *space;
extern const char *rn;
extern const char *sprn;
extern const char *accounts_filename;

// ERR Code
// Message
//00 SORRY <name>
// 01 USER <name> EXISTS
// 02 <name> DOES NOT EXIST
// 10 ROOM EXISTS
// 11 MAXIMUM ROOMS REACHED
// 20 ROOM DOES NOT EXIST
// 30 USER NOT PRESENT
// 40 NOT OWNER
// 41 INVALID USER
// 60 INVALID OPERATION
// 61 INVALID PASSWORD

#define ERROR_MSG(error_code) error_messages[error_code]

enum errors_enum {ERR00, ERR01, ERR02, ERR10, ERR11, ERR20, ERR30, ERR40, ERR41, ERR60, ERR61, ERR62, ERR100};
extern const char *error_messages[];
extern const int error_codes[];

enum verbs_enum {ALOHA, AHOLA, IAM, IAMNEW, HI, HINEW, AUTH, PASS, NEWPASS, ERR, BYE};
extern const char *verbs[];


int send_data(int connfd, int verb, char *data);
int recv_data(int connfd, char *recvbuff);
int expect_data(char *recvbuff, char **request_data, char **errcode, int num_verbs, ...);
void send_error(int connfd, int error, char *message, bool close_connection);
bool user_exists(char *username);
bool check_password(char *password);
int create_user(char* username, char* password);


// 