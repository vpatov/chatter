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
#include <fcntl.h>
#include "threadpool.h"
#include "debug.h"

#define SA 				struct sockaddr
#define SAin 			struct sockaddr_in
#define MAX_RECV		1024
#define MAX_SEND		1024
#define MAX_USERNAME	32
#define MAX_PASSWORD	32
#define MAX_USERS		512






/* ----------------- vars.c ----------------- */
/* ------------------------------------------ */
extern const char *server_help_message;
extern const char *client_help_message;
extern const char *password_rules;
extern const char *space;
extern const char *rn;
extern const char *r;
extern const char *sprn;
extern const char *accounts_filename;

// #define ERROR_MSG(error_code) error_messages[error_code]
// enum errors_enum {ERR00, ERR01, ERR02, ERR10, ERR11, ERR20, ERR30, ERR40, ERR41, ERR60, ERR61, ERR62, ERR100};
// extern const char *error_messages[];
// extern const int error_codes[];







enum verbs_enum {ALOHA, AHOLA, IAM, IAMNEW, HI, HINEW, AUTH, PASS, NEWPASS, ERR, BYE, MSG};
extern const char *verbs[];
/* ------------------------------------------ */
/* ------------------------------------------ */




/* ---------------- errors.c ---------------- */
/* ------------------------------------------ */

typedef struct error_message error_message_t;
struct error_message
{
    const char *msg;
    int code;
};

extern const error_message_t error_messages[];
extern const size_t num_errors;

const char * get_error(int err_code);


/* ------------------------------------------ */
/* ------------------------------------------ */







/* ---------------- locks.c ----------------- */
/* ------------------------------------------ */
extern pthread_mutex_t user_account_mutex;
extern pthread_mutex_t user_info_mutex;
/* ------------------------------------------ */
/* ------------------------------------------ */





/* --------------- chatter.c ---------------- */
/* ------------------------------------------ */
int send_data(int connfd, int verb, char *data);
int recv_data(int connfd, char *recvbuff);
int expect_data(char *recvbuff, char *request_data, int num_verbs, ...);
int expect_data2(char **recvptr, char *message_data, int *error_code, int num_verbs, ...);
void send_error(int connfd, int error, char *message, bool close_connection);
void handle_error(char *recvbuff, char *message_data, int error_code);
/* ------------------------------------------ */
/* ------------------------------------------ */





/* ----------------- util.c ----------------- */
/* ------------------------------------------ */
void strip_char(char *str, char strip);
unsigned int randint();
/* ------------------------------------------ */
/* ------------------------------------------ */





/* ----------------- auth.c ----------------- */
/* ------------------------------------------ */
typedef struct user_account user_account_t; 					
struct user_account {
	char username[MAX_USERNAME+1];
	char salt[(sizeof(int)*2) + 1];
	char password_hash[(SHA_DIGEST_LENGTH*2)+1];
};
extern user_account_t user_accounts[MAX_USERS];					// in-memory representation of user database
extern size_t num_user_accounts;								// how many are valid and should be considered


typedef struct user_info user_info_t;							
struct user_info {
	char username[MAX_USERNAME+1];
	int connfd;
	user_info_t *next;
};
extern user_info_t *user_infos;									// logged in users, and their socket descriptors


bool check_password(char *password);							// check if password fits criterion

bool user_exists(char *username);								// does such a user exist in database?
int create_user(char* username, char* password);				// create a new user in the database
int fill_user_slot(char *username);								// create temporary user in database during user creation
int release_user_slot(char *username);							// upon user creation failure, release the slot
int authenticate_user(char *username, char *password);			// check the user's password against database

user_account_t* get_user_accounts();							// read in all users from database to memory
int free_user_accounts();										// zero out the user account structs
int write_user_accounts();										// write the user accounts in memory to disk

user_info_t* login_user(char *username, int connfd);			//login the user
int logout_user(char *username);								//logout the user, close the connection if not closed
user_info_t* user_logged_in(char *username);					//see if user is logged in
/* ------------------------------------------ */
/* ------------------------------------------ */

