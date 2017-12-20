
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


//TODO configure locks for user authentication

void iam_login(int connfd, char *client_username){
	char recvbuff[MAX_RECV];
	char *recvptr = recvbuff;
	int error_code, ret;
	char message_data[MAX_RECV];

	info("Client with username: \"%s\" is attempting login.", client_username);

	if (user_logged_in(client_username)){
		send_error(connfd, 3, client_username, false);
		send_data(connfd, BYE, NULL);
		close(connfd);
		return;
	}

	if (user_exists(client_username)){
		//prompt client to send password
		send_data(connfd,AUTH,client_username);
		info("User exists, waiting for client to send password.");

		recvptr = recvbuff;
		recv_data(connfd,recvbuff);
		if ((ret = expect_data2(&recvptr,message_data,&error_code,1,PASS)) < 0){
			handle_error(recvbuff,message_data,error_code);
			send_error(connfd, 60, NULL, true);
			return;
		}

		//if user is authenticated successfully
		if (authenticate_user(client_username,message_data) == 0){
			send_data(connfd, HI, client_username);
			login_user(client_username,connfd);
		}

		else {
			send_error(connfd, 61, NULL, true);
			return;
		}
	}

	else {
		send_error(connfd, 2, NULL, false);
		send_data(connfd,BYE,NULL);
		close(connfd);
		return;
	}
}

void iamnew_login(int connfd, char *client_username){
	char recvbuff[MAX_RECV];
	char *recvptr = recvbuff;
	int error_code, ret;
	char message_data[MAX_RECV];

	info("Client with username: \"%s\" is attempting login and user creation.", client_username);

	// if the user exists already
	if (user_exists(client_username)){
		//tell user no dice
		//close connection.ERR 01 SORRY <name> \r\n
		info("User %s already exists. Sending ERR01 and closing connection.", client_username);
		send_error(connfd, 1, client_username, false);
		send_data(connfd,BYE,NULL);
		close(connfd);
	}

	else {
		fill_user_slot(client_username);

		send_data(connfd,HINEW,client_username);
		recvptr = recvbuff;
		recv_data(connfd,recvbuff);
		//expecting NEWPASS verb
		if ((ret = expect_data2(&recvptr,message_data,&error_code,1,NEWPASS)) < 0){
			handle_error(recvbuff,message_data,error_code);
			release_user_slot(client_username);
			send_error(connfd, 60, NULL, true);
			return;
		}

		//check password
		if (!check_password(message_data)){
			release_user_slot(client_username);
			send_error(connfd, 61, NULL, false);
			send_data(connfd,BYE,NULL);
			close(connfd);
			return;
		}

		//make the user
		if ((ret = create_user(client_username, message_data))){
			release_user_slot(client_username);
			send_error(connfd, ret, NULL, true);
			return;
		}

		//successful creation, send HI
		send_data(connfd, HI, client_username);
		login_user(client_username,connfd);

	}
}


//should receive connected socket as argument.
void *login_thread_func(void *arg){
	thread_arg_t *thread_arg = arg;
	int connfd, ret, error_code;
	char message_data[MAX_RECV];
	char recvbuff[MAX_RECV];
	char *recvptr = recvbuff;


	connfd = *(int*)(thread_arg->arg);
	info("Waiting to receive: %d", connfd);

	//Receive first ALOHA! from client
	recv(connfd,recvbuff,MAX_RECV,0);
	if ((ret = expect_data2(&recvptr,NULL,&error_code,1,ALOHA)) < 0){
		handle_error(recvbuff,NULL,error_code);
		send_error(connfd, 60, NULL, true);
		return NULL;
	}


	//Received aloha, proceed to send !AHOLA
	info("aloha received");
	send_data(connfd,AHOLA,NULL);
	// snprintf(sendbuff,MAX_SEND,"%s%s",verbs[AHOLA],rn);
	// send(connfd,sendbuff,strlen(sendbuff),0);


	//Wait for command from client. Either IAM or IAMNEW
	info("Waiting for client to send verb.");
	recvptr = recvbuff;
	recv_data(connfd,recvbuff);
	if ((ret = expect_data2(&recvptr, message_data, &error_code,2, IAM, IAMNEW)) < 0){
		handle_error(recvbuff,message_data,error_code);
		send_error(connfd,60, NULL, true);
		return NULL;
	}


	if (ret == IAM)
		iam_login(connfd, message_data);
	else if (ret == IAMNEW)
		iamnew_login(connfd, message_data);
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
