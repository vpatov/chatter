
#include "chatter.h"





bool echo_mode = false;
int server_port;
int num_chat_rooms;
char *motd;
FILE *output;

pool_t *threadpool;

void spawn_login_thread();
void *login_thread_func(void *arg);
void iam_login();
void iamnew_login();
/*

// All pointers to socket address structures are often cast to pointers
// to this type before use in various functions and system calls:

struct sockaddr {
    unsigned short    sa_family;    // address family, AF_xxx
    char              sa_data[14];  // 14 bytes of protocol address
};


// IPv4 AF_INET sockets:

struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET, AF_INET6
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};

struct in_addr {
    unsigned int s_addr;          // load with inet_pton()
};
*/

char*
inet4_ntop(char *dst, unsigned int addr)
{
    snprintf(dst,INET_ADDRSTRLEN,"%d.%d.%d.%d\n", (addr >> 24) & 0xFF, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF); 
    return dst;
}

void spawn_echo_thread(){

}


// void spawn_login_thread(int connfd){
// 	int r;
// 	pthread_t thread;
// 	pthread_attr_t attr;

// 	pthread_attr_init(&attr);
// 	pthread_create(&thread,&attr,login_thread_func,connfd);				
// }






/*
C > S | ALOHA! \r\n
C < S | !AHOLA \r\n
C > S | IAM <name> \r\n
# <name> is user's name
# Example: IAM cse320 \r\n
*/

void iam_login(int connfd, char *client_username){
	char recvbuff[MAX_RECV];
	char client_password[MAX_PASSWORD];

	info("Client with username: \"%s\" is attempting login.", client_username);
	if (user_exists(client_username)){
		//continue with login
		//prompt for password
	}
}

void iamnew_login(int connfd, char *client_username){
	int ret;
	char recvbuff[MAX_RECV];
	char client_password[MAX_PASSWORD];

	info("Client with username: \"%s\" is attempting login and user creation.", client_username);
	if (user_exists(client_username)){
		//tell user no dice
		//close connection.ERR 01 SORRY <name> \r\n
		send_error(connfd, ERR01, client_username, false);
		send_data(connfd,BYE,NULL);
		close(connfd);
	}
	else {
		send_data(connfd,HINEW,client_username);
		recv_data(connfd,recvbuff);
		if (expect_data(recvbuff,client_password,NULL,1,NEWPASS) < 0){
			send_error(connfd, ERR60, NULL, true);
			return;
		}

		//check password
		if (!check_password(client_password)){
			send_error(connfd, ERR61, NULL, true);
			return;
		}

		//make the user
		if ((ret = create_user(client_username, client_password))){
			send_error(connfd, ret, NULL, true);
			return;
		}

		//successful creation, send HI
		send_data(connfd, HI, client_username);

	}
}


//should receive connected socket as argument.
void *login_thread_func(void *arg){
	thread_arg_t *thread_arg = arg;
	int connfd;
	int ret;
	char client_username[MAX_USERNAME];
	char recvbuff[MAX_RECV];

	connfd = *(int*)(thread_arg->arg);
	info("Waiting to receive: %d", connfd);

	//Receive first ALOHA! from client
	recv(connfd,recvbuff,MAX_RECV,0);
	if (expect_data(recvbuff,NULL,NULL,1,ALOHA) < 0){
		send_error(connfd, ERR60, NULL, true);
		return NULL;
	}


	//Received aloha, proceed to send !AHOLA
	info("aloha received");
	send_data(connfd,AHOLA,NULL);
	// snprintf(sendbuff,MAX_SEND,"%s%s",verbs[AHOLA],rn);
	// send(connfd,sendbuff,strlen(sendbuff),0);


	//Wait for command from client. Either IAM or IAMNEW
	info("Waiting for client to send verb.");
	recv_data(connfd,recvbuff);
	if ((ret = expect_data(recvbuff, client_username, NULL, 2, IAM, IAMNEW)) < 0){
		send_error(connfd,ERR60, NULL, true);
		return NULL;
	}


	if (ret == IAM)
		iam_login(connfd, client_username);
	else if (ret == IAMNEW)
		iamnew_login(connfd, client_username);
	else {
		error("expect_data has bugs in it.");
		assert(false);
	}

	return NULL;
}


void
accept_connections()
{
	int listenfd, *connfd;
	int err;
	struct sockaddr_in listen_sa, *conn_sa;
	socklen_t listen_addrlen, conn_addrlen;
	char client_ip[INET_ADDRSTRLEN];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if ((err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) < 0){
		error("setsockopt failed: %s",strerror(err));
	}

	listen_addrlen = sizeof(listen_sa);
	bzero(&listen_sa,sizeof(listen_sa));
	listen_sa.sin_family = AF_INET;
	listen_sa.sin_port = htons(server_port);
	listen_sa.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(listenfd,(SA *)&listen_sa,sizeof(listen_sa));

	listen(listenfd,50);

	// block on accept() - that's okay
	while(true){
		memset(client_ip,0,INET_ADDRSTRLEN);
		conn_sa = calloc(1,sizeof(*conn_sa));
		conn_addrlen = sizeof(conn_sa);

		//despite the fact that the connection socket descriptor is just 
		//an int,and doesn't absolutely need to be dynamically allocated,
		//I am doing so for the sake of consistency. The alternative is 
		//to cast to it to a void pointer which seems hacky.
		connfd = malloc(sizeof(int));


		*connfd = accept(listenfd,(SA *)conn_sa,&conn_addrlen);

		inet4_ntop(client_ip,conn_sa->sin_addr.s_addr);
		info("Accepted a client connection: %s", client_ip);
		info("Queueing login_thread_func in the threadpool...");
		pool_queue(threadpool,login_thread_func,connfd);
	}
}


/*
	Initialize server's data structures, and start the echo thread.
*/

void 
server_init()
{
	output = stdout;
	threadpool = pool_create(2,4,500,NULL);

	//Initialize locks for the different threads.

    info("Starting server. Currently listening on port: %d", server_port);
}

/* 
	Argument parsing was based on this SO post:
	https://stackoverflow.com/a/24479532/3664123
*/

void parse_args(int argc, char** argv){
	int opt;

    while ((opt = getopt(argc, argv, "ehn")) != -1) {
        switch (opt) {
        case 'h': fprintf(stderr,"%s",server_help_message); exit(EXIT_FAILURE);
        case 'e': echo_mode = true; break;
        case 'n': num_chat_rooms = 5; break;
        default:
        	fprintf(stderr,"%s",server_help_message);
        	exit(EXIT_FAILURE);
        }
    }

    if (num_chat_rooms && optind + 3 != argc){
    	fprintf(stderr,"%s",server_help_message);
    	exit(EXIT_FAILURE);
    }

    else if (!num_chat_rooms && optind + 2 != argc){
    	fprintf(stderr,"%s",server_help_message);
    	exit(EXIT_FAILURE);
    }

    if (num_chat_rooms){
    	num_chat_rooms = atoi(argv[optind++]);
    }

    server_port = atoi(argv[optind++]);
    motd = argv[optind++];

    if (server_port == 0){
    	fprintf(stderr,"%s",server_help_message);
    	exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv){
	parse_args(argc,argv);
    server_init();
    accept_connections();
}
