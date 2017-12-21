#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <signal.h>
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
#include <poll.h>
#include "threadpool.h"
#include "debug.h"

#define SA 				struct sockaddr
#define SAin 			struct sockaddr_in
#define MAX_MSG_SIZE	1024
#define MAX_RECV		MAX_MSG_SIZE
#define MAX_SEND		MAX_MSG_SIZE
#define MAX_CMD			64
#define MAX_USERNAME	32
#define MAX_PASSWORD	32
#define MAX_USERS		64
#define MAX_ROOMS		64








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


/* ----------------- login.c ---------------- */
/* ------------------------------------------ */
extern char motd[MAX_MSG_SIZE];


void iam_login_server(int connfd, char *client_username);
void iamnew_login_server(int connfd, char *client_username);
void *login_thread_func(void *arg);

/* ------------------------------------------ */
/* ------------------------------------------ */



enum verbs_enum {
	ALOHA, AHOLA, IAM, IAMNEW, HI, HINEW, AUTH, 
	PASS, NEWPASS, ERR, BYE, MSG, 
	CREATER, RETAERC,
	CREATEP, PETAERC,
	LISTR, RTSIL,
	LISTU, UTSIL,
	JOIN,NIOJ,
	JOINP,PNIOJ,
	LEAVE,EVAEL,
	KICK,KCIK,
	TELL,LLET,
	ECHO,ECHOP,
	NOP, QUIT
};

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
void init_user_mutexes();
void lock_user_accounts(int track);
void unlock_user_accounts(int track);
void lock_user_info(int track);
void unlock_user_info(int track);

extern pthread_mutex_t room_mutex;
void init_room_mutexes();
void lock_rooms(int track);
void unlock_rooms(int track);
void lock_room_members(int track);
void unlock_room_members(int track);

/* ------------------------------------------ */
/* ------------------------------------------ */





/* --------------- chatter.c ---------------- */
/* ------------------------------------------ */
extern char print_buff[MAX_MSG_SIZE];

int send_data(int connfd, int verb, char *data);
int send_data_custom(int connfd, char *data);
int recv_data(int connfd, char *recvbuff);
int expect_data(char *recvbuff, char *message_data, int *error_code, int num_verbs, ...);
void send_error(int connfd, int error, char *message, bool close_connection);
void print_error(char *recvbuff, char *message_data, int error_code);
int tokenize(char *string, const char *delim);
char *get_token(char *string, const char *delim, int count);
/* ------------------------------------------ */
/* ------------------------------------------ */





/* ----------------- util.c ----------------- */
/* ------------------------------------------ */
void strip_char(char *str, char strip);
unsigned int randint();
char *inet4_ntop(char *dst, unsigned int addr);
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
	bool ready;
	bool in_room;
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
user_info_t* get_user_byfd(int connfd);							//get the user info by the socket fd
user_info_t* get_user_byname(char *username);
int get_num_users();
int mark_user_ready(char *username);
/* ------------------------------------------ */
/* ------------------------------------------ */


/* ----------------- rooms.c ---------------- */
/* ------------------------------------------ */
typedef struct room_member room_member_t;
struct room_member {
	user_info_t *user;
	bool owner;
	room_member_t *next;
};


typedef struct room room_t;
struct room {
	char room_name[MAX_USERNAME];
	bool private_room;
	int room_id;
	room_member_t *room_members;
	char password[MAX_PASSWORD];	//no hashing
	room_t *next;
};


extern room_t *rooms;
extern size_t num_rooms;
extern int room_inc_id;




int create_room(char *room_name, user_info_t *user, bool private_room, char *password);
room_t *get_room(char *room_name);
room_t *get_room_by_id(int room_id);
int free_room_members(room_member_t *room_members);
int add_room_member(room_t *room, user_info_t *user, char *password);
int remove_room_member(room_t *room, char *username);
int remove_user_from_rooms(char *username);
int close_room(room_t *room);
int list_rooms(char *sendbuff);
int check_room(room_t *room);
int close_room_by_name(char *room_name);

/* ------------------------------------------ */
/* ------------------------------------------ */



/* --------------- echo.c ----------------- */
/* ------------------------------------------ */
extern bool echo_mode;
extern bool echo_flag;
extern bool echo_running;

extern pthread_t echo_thread;
extern pthread_attr_t echo_thread_attr;

void spawn_echo_thread();
void *echo_thread_func(void *arg);
void echo_all_room(room_t *room, int verb, char *message_data);

/* ------------------------------------------ */
/* ------------------------------------------ */
