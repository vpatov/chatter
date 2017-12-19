
#include "chatter.h"



#define SA struct sockaddr
// extern const char aloha[];

void connect_to_server();

int server_connfd;
int server_port;
bool create_user_mode;
struct sockaddr_in server_sa;
socklen_t server_addrlen;

char *server_ip;
char *username;
char recvbuff[MAX_RECV];
char sendbuff[MAX_SEND];
char expect[MAX_RECV];

// TODO 1) handle this login protocol between client and server.
// TODO 2) Create the shared data structures
// TODO 3) Try to create a generic enforcement of the ALOHA protocol.
// TODO 4) Client: parse command line args
/*
C > S | ALOHA! \r\n
C < S | !AHOLA \r\n
C > S | IAM <name> \r\n
# <name> is user's name
# Example: IAM cse320 \r\n
*/



void
connect_to_server()
{

	int err;

	server_connfd = socket(AF_INET, SOCK_STREAM, 0);

	server_addrlen = sizeof(server_sa);
	bzero(&server_sa,sizeof(server_sa));
	server_sa.sin_family = AF_INET;
	server_sa.sin_port = htons(9876);
	server_sa.sin_addr.s_addr = inet_addr("127.0.0.1");

	err = connect(server_connfd,(SA *)&server_sa,server_addrlen);

	if (err){
		fprintf(stderr, "connect returned an error: %s", strerror(errno));
		return;
	}

	//send ALOHA! to server
	snprintf(sendbuff,MAX_SEND,"%s%s",aloha_verb,rn);
	send(server_connfd,sendbuff,strlen(sendbuff),0);

	//receive response from server
	recv(server_connfd,recvbuff,MAX_RECV,0);

	//Received something other than "!AHOLA", so print an error and terminate.
	snprintf(expect,MAX_RECV,"%s%s",ahola_verb,rn);
	if (strcmp(recvbuff,expect )){
		error("Expected to receive \"%s\" upon connection.", ahola_verb);
		close(server_connfd);
		exit(1);
	}

	info("Received %s from server.", ahola_verb);



}


void iam(){

}


void iamnew(){

}

/*
./client [-h] [-c] NAME SERVER_IP SERVER_PORT
-h
Displays this help menu, and returns
EXIT_SUCCESS.
-c
Requests to server to create a new user
NAME
This is the username to display when chatting.
SERVER_IP
The ipaddress of the server to connect to.
SERVER_PORT
The port to connect to.
*/


void parse_args(int argc, char** argv){
	int opt;

    while ((opt = getopt(argc, argv, "ch")) != -1) {
        switch (opt) {
        case 'h': fprintf(stderr,"%s",client_help_message); exit(EXIT_FAILURE);
        case 'c': create_user_mode = true; break;
        default:
        	fprintf(stderr,"%s",client_help_message);
        	exit(EXIT_FAILURE);
        }
    }

    if (optind + 3 != argc){
    	fprintf(stderr,"%s",client_help_message);
    	exit(EXIT_FAILURE);
    }

   	username  = argv[optind++];
   	server_ip = argv[optind++];
    server_port = atoi(argv[optind++]);

    if (server_port == 0){
    	fprintf(stderr,"%s",client_help_message);
    	exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv){
	printf("aloha enum: %d", aloha);
	printf("index 0 of verbs: %s",VERBS[0]);

	parse_args(argc, argv);
	exit(0);
	
	connect_to_server();


}
