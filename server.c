/*
Usage: 
./server [-he] PORT_NUMBER MOTD

-e 				Echo messages received on server's stdout.
-h 				Displays help menu & returns EXIT_SUCCESS.
PORT_NUMBER		Port number to listen on.
MOTD			Message to display to the client when they connect.

*/



#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "threadpool.h"
#include "debug.h"

#define SA 			struct sockaddr
#define SAin 		struct sockaddr_in
#define MAX_RECV	1024
#define MAX_SEND	1024

const char* help_message = 	"Usage:\n./server [-he] PORT_NUMBER MOTD\n\n"
							"-e\t\t\tEcho messages received on server's stdout.\n"
							"-h\t\t\tDisplays help menu & returns EXIT_SUCCESS.\n"
							"PORT_NUMBER\t\tPort number to listen on. (Must be non-zero)\n"
							"MOTD\t\t\tMessage to display to the client when they connect.\n";

const char* aloha = 	"ALOHA!";
const char* ahola = 	"!AHOLA";
const char* iam =		"IAM";
const char* iamnew = 	"IAMNEW";
const char* newpass = 	"NEWPASS";
const char* err =		"ERR";

bool echo_mode = false;
int server_port;
char *motd;
FILE *output;

pool_t *threadpool;

void spawn_login_thread();
void *login_thread_func(void *arg);
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





// TODO 1) handle this login protocol between client and server.
// TODO 2) Create the shared data structures
// TODO 3) Try to create a generic enforcement of the ALOHA protocol.
// TODO 4) Related to #2 - check to make sure that no user with given name exists already.
// TODO 5) In chat rooms, assign each different user a unique color.
/*
C > S | ALOHA! \r\n
C < S | !AHOLA \r\n
C > S | IAM <name> \r\n
# <name> is user's name
# Example: IAM cse320 \r\n
*/

//should receive connected socket as argument.
void *login_thread_func(void *arg){
	thread_arg_t *thread_arg = arg;
	int connfd;
	char recvbuff[MAX_RECV];
	char sendbuff[MAX_SEND];

	connfd = *(int*)(thread_arg->arg);

	printf("Waiting to receive: %d\n", connfd);

	//Receive first ALOHA! from client
	recv(connfd,recvbuff,MAX_RECV,0);


	//Received something other than "ALOHA!", so send an error.
	if (strcmp(recvbuff,aloha)){

		snprintf(sendbuff,MAX_SEND,"%s: Expected \"%s\" to be first message upon connection.\r\n", err, aloha);
		send(connfd,sendbuff,strlen(sendbuff),0);
		close(connfd);
		return NULL;
	}

	else {
		info("aloha received.");
	}

	return NULL;

}


void
accept_connections()
{
	int listenfd, *connfd;
	void *err;
	struct sockaddr_in listen_sa, *conn_sa;
	socklen_t listen_addrlen, conn_addrlen;
	char client_ip[INET_ADDRSTRLEN];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

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
		info("Spawning login thread...");	
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
    info("Starting server. Port: %d", server_port);
}

/* 
	Argument parsing was based on this SO post:
	https://stackoverflow.com/a/24479532/3664123
*/

void parse_args(int argc, char** argv){
	int opt;

    while ((opt = getopt(argc, argv, "eh")) != -1) {
        switch (opt) {
        case 'h': fprintf(stderr,"%s",help_message); exit(EXIT_FAILURE);
        case 'e': echo_mode = true; break;
        default:
        	fprintf(stderr,"%s",help_message);
        	exit(EXIT_FAILURE);
        }
    }

    if (optind + 2 != argc){
    	fprintf(stderr,"%s",help_message);
    	exit(EXIT_FAILURE);
    }

    server_port = atoi(argv[optind++]);
    motd = argv[optind++];

    if (server_port == 0){
    	fprintf(stderr,"%s",help_message);
    	exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv){
	parse_args(argc,argv);
    server_init();
    accept_connections();
}
