#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include "threadpool.h"
#include "debug.h"


#define SA struct sockaddr

void connect_to_server();

int server_connfd;
struct sockaddr_in server_sa;
socklen_t server_addrlen;


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
	}

	char data[100];

	// recv(server_connfd, data, 100,0);
	snprintf(data,100,"notaloha");
	send(server_connfd,data,100,0);

	recv(server_connfd,data,100,0);
	printf("%s\n",data);
}


int main(){

	connect_to_server();


}
