#define _GNU_SOURCE
#include "chatter.h"



#define SA struct sockaddr
// extern const char aloha[];

void connect_to_server();
void iam_login();
void iamnew_login();
void main_communication();

int server_connfd;
int server_port;
bool create_user_mode;
struct sockaddr_in server_sa;
socklen_t server_addrlen;
FILE *output;


char *server_ip;
char *username;
char recvbuff[MAX_RECV];
char sendbuff[MAX_SEND];
char stdinbuff[MAX_SEND];



void close_conn(){
	close(server_connfd);
	exit(0);
}

void print_motd(char *message){
	printf(KWHT "%s" KNRM "\n", message);
}

void main_communication(){
	
	struct pollfd fds[2]; //multiplex on server and on stdin
	int timeout_ms = 1000;
	int ret;

	fds[0].fd = server_connfd;
	fds[0].events = POLLIN;
	fds[1].fd = fileno(stdin);
	fds[1].events = POLLIN;

	while(true){
		ret = poll(fds, 2, timeout_ms);
		if (ret > 0) {
			if (fds[0].revents & POLLIN){
				ret = recv_data(server_connfd,recvbuff);
				if (ret < 0){
					debug("server returned: %d %s",ret, strerror(ret));
					exit(0);
				}
				if (ret == 0){
					debug("server returned zero.");
					exit(0);
				}
				debug("received from server: %s", recvbuff);
			}

			if (fds[0].revents & POLLHUP){
				infow("Server disconnected.");
				exit(0);
			}
			if (fds[0].revents & POLLRDHUP){
				infow("Server disconnected.");
				exit(0);


			}
			if (fds[1].revents & POLLIN){
				ret = read(fileno(stdin), stdinbuff, MAX_SEND);
				debug("Client ret: %d", ret);
				if (ret > 0){
					stdinbuff[ret] = '\0';
				}
				else {
					infow("Standard input closed. Exiting...");
					close(server_connfd);
					exit(0);
				}
				strip_char(stdinbuff,'\n');


				// TODO switch on commands

				send_data(server_connfd,MSG,stdinbuff);
			}

		}

	//multiplex on server output and user input 
	}

}

void iam_login_client(){
	int ret, error_code, tokens;
	char *message;
	char message_data[MAX_RECV];
	char user_password[MAX_PASSWORD];

	send_data(server_connfd, IAM, username);

	recv_data(server_connfd,recvbuff);
	tokens = tokenize(recvbuff,sprn);
	message = get_token(recvbuff,sprn,0);
	//expect an AUTH from server.
	if ((ret = expect_data(recvbuff,message_data, &error_code,1,AUTH)) < 0){
		print_error(recvbuff,message_data,error_code);
		send_error(server_connfd, 60, NULL, true);
		return;
	}

	//We got the AUTH, let's assert the username is the same
	if (strcmp(username,message_data)){
		error("Server responded with wrong username - Abort");
		close_conn();
		return;
	}

	//Prompt user for password, and send it to server
	prompt("Please enter your password:");
	scanf("%s",user_password);
	send_data(server_connfd, PASS, user_password);

	//Receive response
	
	recv_data(server_connfd, recvbuff);
	tokens = tokenize(recvbuff,sprn);
	message = get_token(recvbuff,sprn,0);
	if (expect_data(recvbuff, message_data, &error_code, 1, HI) < 0){
		// our password may have been wrong
		print_error(recvbuff,message_data,error_code);
		send_error(server_connfd, 60, NULL, true);
		return;
	}

	info("Client successfully logged in as existing user: %s", username);



	if (tokens > 1){
		message = get_token(recvbuff,sprn,1);
		if (message != NULL){
			if (expect_data(message, message_data, &error_code, 1, ECHO) < 0){
				// our password may have been wrong
				print_error(recvbuff,message_data,error_code);
				send_error(server_connfd, 60, NULL, true);
				return;
			}
			printf(KWHT "%s" KNRM "\n" , message_data);
		}
		else {
			error("get_token returned NULL. Could not get MOTD");
		}
	}

	


	main_communication();

}


void iamnew_login_client(){
	int ret, error_code, tokens;
	char message_data[MAX_RECV];
	char user_password[MAX_PASSWORD];
	char *message;
	int attempts = 0;

	send_data(server_connfd, IAMNEW, username);

	recv_data(server_connfd,recvbuff);
	tokens = tokenize(recvbuff,sprn);
	message = get_token(recvbuff,sprn,0);	
	//expect a HINEW <username> from server.
	if ((ret = expect_data(recvbuff,message_data,&error_code,1,HINEW)) < 0){
		print_error(recvbuff,message_data,error_code);
		send_error(server_connfd, 60, NULL, true);
		return;
	}

	//We got the HINEW, let's assert the username is the same
	if (strcmp(username,message_data)){
		error("Server responded with wrong username - Abort");
		close(server_connfd);
		return;
	}

	//Prompt user for password
	fprintf(output,"%s",password_rules);
	while(true){
		scanf("%s",user_password);
		if (check_password(user_password))
			break;
		else {
			attempts++;
			if (attempts >= 3){
				fprintf(output,"Too many tries. Sober up and try again later.\n");
				close(server_connfd);
				exit(1);
			}
			fprintf(output,"Password does not meet criterion. Please try again.\n");
		}
	}

	info("Sending server password: ---%s---", user_password);
	send_data(server_connfd, NEWPASS, user_password);


	recv_data(server_connfd,recvbuff);
	tokens = tokenize(recvbuff,sprn);
	message = get_token(recvbuff,sprn,0);
	if ((ret = expect_data(recvbuff,message_data,&error_code,1,HI)) < 0){
		print_error(recvbuff, message_data, error_code);
		send_error(server_connfd, 60, NULL, true);
		return;
	}

	info("Client successfully logged in as new user: %s", username);

	
	if (tokens > 1){
		message = get_token(recvbuff,sprn,1);
		if (message != NULL){
			if (expect_data(message, message_data, &error_code, 1, ECHO) < 0){
				// our password may have been wrong
				print_error(recvbuff,message_data,error_code);
				send_error(server_connfd, 60, NULL, true);
				return;
			}
			printf(KWHT "%s" KNRM "\n" , message_data);
		}
		else {
			error("get_token returned NULL. Could not get MOTD");
		}
	}


	//We are now connected
	main_communication();


}

void login_protocol()
{
	int error_code;
	//send ALOHA! to server
	send_data(server_connfd, ALOHA, NULL);

	//receive response from server -should be !AHOLA
	recv_data(server_connfd,recvbuff);
	if (expect_data(recvbuff,NULL,&error_code,1,AHOLA) < 0){
		send_error(server_connfd, 60, NULL, true);
		return;
	}

	info("Received %s from server.", verbs[AHOLA]);

	if (create_user_mode){
		iamnew_login_client();
	}
	else {
		iam_login_client();
	}
}

void
connect_to_server()
{

	int err;

	server_connfd = socket(AF_INET, SOCK_STREAM, 0);

	server_addrlen = sizeof(server_sa);
	bzero(&server_sa,sizeof(server_sa));
	server_sa.sin_family = AF_INET;
	server_sa.sin_port = htons(server_port);
	server_sa.sin_addr.s_addr = inet_addr("127.0.0.1");

	err = connect(server_connfd,(SA *)&server_sa,server_addrlen);

	if (err){
		fprintf(stderr, "connect returned an error: %s", strerror(errno));
		return;
	}

	login_protocol();

	
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
	parse_args(argc, argv);
	output = stdout;
	info("Initiated client. Creating user? %d, Username: %s, server_port: %d, server_ip: %s", 
		create_user_mode, username, server_port, server_ip);
	connect_to_server();


}
