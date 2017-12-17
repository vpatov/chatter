#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include "threadpool.h"
#include "debug.h"


#define SA struct sockaddr


int main(){

	int server_connfd, err;
	struct sockaddr_in server_sa;


	socklen_t server_addrlen;


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

	recv(server_connfd, data, 100,0);
	printf("%s\n",data);


}
