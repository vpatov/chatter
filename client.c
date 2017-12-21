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
char command[MAX_SEND];

int expected_verb;

void close_conn(){
	close(server_connfd);
	exit(0);
}

void print_nice(char *message){
	printf(KWHT "%s" KNRM "\n", message);
}


void process_server_response(char *recvbuff){
	int tokens, error_code, ret;
	char *message;
	const char *delim;
	char message_data[MAX_MSG_SIZE];
	memset(message_data,0,MAX_MSG_SIZE);


	delim = sprn;
	if (!(strncmp("RTSIL",recvbuff,5))){
		delim = rn;
	}
	if (!(strncmp("UTSIL",recvbuff,5))){
		delim = rn;
	}

	// If we receive multiple messages in the same packet.
	tokens = tokenize(recvbuff,sprn);
	for (int t = 0; t< tokens; t++){
		message = get_token(recvbuff,sprn,t);
		if ((ret = expect_data(message, message_data, &error_code, 
			4, expected_verb, ECHO, ECHOP,ECHOR)) < 0){
			if (message_data[0]){
				print_error(message,message_data,error_code);
			}
			else {
				debug("process_server_response unhandled case: %s", recvbuff);
			}
			return;
		}
		else {

			switch(ret){
				case ECHO: {
					printecho("%s",message_data);
					break;
				}
				case ECHOP: {
					printpriv("%s",message_data);
					break;
				}

				case ECHOR: {
					printechor("%s",message_data);
					break;
				}
				case RETAERC: {
					infow("Received confirmation of room (%s) creation.", message_data);
					break;
				}

				case PETAERC: {
					infow("Received confirmation of private room (%s) creation.", message_data);
					break;
				}

				case NIOJ: {
					infow("Received confirmation of joining room (%s).",message_data);
					break;
				}

				case PNIOJ: {
					infow("Received confirmation of joining private room (%s).",message_data);
					break;
				}

				case RTSIL: {
					print_nice(message_data);
					break;
				}

				case UTSIL: {
					print_nice(message_data);
					break;
				}
			}

		}
	}
}

void send_user_command(int verb, char *arg1, char *arg2){
    char sendbuff[1024];
    // memset(sendbuff,0,1024); //sprintf writes null terminator

    if (arg2 == NULL){
        if (arg1 == NULL){
            sprintf(sendbuff, "%s \r\n",verbs[verb]);
        }
        else {
            sprintf(sendbuff, "%s %s \r\n", verbs[verb], arg1);
        }
    }
    else {
        sprintf(sendbuff, "%s %s %s \r\n", verbs[verb], arg1, arg2);
    }

    expected_verb = verb + 1; //neatest thing ive done in this entire project

    send_data_custom(server_connfd,sendbuff);
    if (verb == QUIT){
        exit(0);
    }

}


void process_user_command(char *stdinbuff){
    int int_arg;
    // char end;
    char *endptr;
    char *command_root, *arg1, *arg2, *saveptr;


    memset(command,0,MAX_SEND); // to be safe
    strcpy(command,stdinbuff);

    command_root = strtok_r(command, sprn, &saveptr);
    if (command_root != NULL){
        if (!strcmp(command_root,"/creater")){
            arg1 = strtok_r(NULL,sprn,&saveptr);
            if (arg1 == NULL){
                printf("%s\n","Incomplete command.");
                return;
            }
            else {
                //send_creater
                send_user_command(CREATER,arg1,NULL);
                return;
            }
        }
        else if (!strcmp(command_root,"/createp")){
            arg1 = strtok_r(NULL,sprn,&saveptr);
            if (arg1 == NULL){
                printf("%s\n","Incomplete command.");
                return;
            }
            else {
                arg2 = strtok_r(NULL,sprn,&saveptr);
                if (arg2 == NULL){
                    printf("%s\n","/createp requires a room name and a password.");
                    return;
                }

                else {
                    //send_createp
                    send_user_command(CREATEP,arg1,arg2);
                    return;
                }
            }
        }
        else if (!strcmp(command_root,"/tell")){
            arg1 = strtok_r(NULL,sprn,&saveptr);
            if (arg1 == NULL){
                printf("%s\n","Incomplete command.");
                return;
            }
            else {
                // arg2 = strtok_r(NULL,sprn,&saveptr);
                arg2 = saveptr;
                if (arg2 == NULL){
                    printf("%s\n","/tell requires a user name and a message.");
                    return;
                }
                else if (*arg2 == 0){
                    printf("%s\n","/tell requires a user name and a message.");
                    return;
                }

                else {
                    //send_tell
                    strip_char(arg2,'\n');
                    send_user_command(TELL,arg1,arg2);
                    return;
                }
            }

        }
        else if (!strcmp(command_root,"/listrooms")){
            //send_listrooms
            send_user_command(LISTR,NULL,NULL);
            return;
        }
        else if (!strcmp(command_root,"/listusers")){
            //send_listusers
            send_user_command(LISTU,NULL,NULL);
            return;
        }
        else if (!strcmp(command_root,"/join")){
            arg1 = strtok_r(NULL,sprn,&saveptr);
            if (arg1 == NULL){
                printf("%s\n","Incomplete command.");
                return;
            }
            else {
                int_arg = strtol(arg1,&endptr,10);

                if (errno != 0 && int_arg == 0) {
                    printf("%s\n","/join must have an integer argument.");
                    return;
                }

                else if (endptr == arg1) {
                    printf("%s\n","/join must have an integer argument.");
                    return;
                }

                else {
                    //send_join
                    send_user_command(JOIN,arg1,NULL);
                    return;
                }

            }


        }
        else if (!strcmp(command_root,"/joinp")){
            arg1 = strtok_r(NULL,sprn,&saveptr);
            if (arg1 == NULL){
                printf("%s\n","Incomplete command.");
                return;
            }
            else {
                int_arg = strtol(arg1,&endptr,10);
                if (errno != 0 && int_arg == 0) {
                    printf("%s\n","/joinp must have an integer argument.");
                    return;
                }

                else if (endptr == arg1) {
                    printf("%s\n","/joinp must have an integer argument.");
                    return;
                }
                else {
                    arg2 = strtok_r(NULL,sprn,&saveptr);
                    if (arg2 == NULL){
                        printf("%s\n","/joinp requires a room id and a password.");
                        return;
                    }

                    else {
                        //send_joinp
                        send_user_command(JOINP,arg1,arg2);
                        return;
                    }
                }
            }
        }

        else if (!strcmp(command_root,"/leave")){
            //send_leave
            send_user_command(LEAVE,NULL,NULL);

            return;
        }
        else if (!strcmp(command_root,"/quit")){
            //send_quit
            send_user_command(QUIT,NULL,NULL);

            return;
        }
        else if (!strcmp(command_root,"/kick")){
            arg1 = strtok_r(NULL,sprn,&saveptr);
            if (arg1 == NULL){
                printf("%s\n","Incomplete command.");
                return;
            }
            else {
                //send_kick
                send_user_command(KICK,arg1,NULL);

                return;
            }
        }

        else if (!strcmp(command_root,"/echo")){
        	arg1 = saveptr;
        	if (arg1 == NULL){
        		printf("%s\n", "/echo needs at least one-non whitespace character to send.");
        		return;
        	}
        	else if (*arg1 == 0){
                printf("%s\n", "/echo needs at least one-non whitespace character to send.");
                return;
            }


            else {
                //send_echo
                strip_char(arg1,'\n');
                send_user_command(ECHOR,arg1,NULL);
                return;
            }
            
        }
    }
    else {
        printf("%s\n","You've entered an invalid command.");
    }
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
				memset(recvbuff,0,MAX_MSG_SIZE);
				ret = recv_data(server_connfd,recvbuff);
				if (ret < 0){
					debug("server returned: %d %s",ret, strerror(ret));
					exit(0);
				}
				if (ret == 0){
					debug("server returned zero.");
					exit(0);
				}
				// debugw("Server sent: %s", recvbuff);

				process_server_response(recvbuff);

				//TODO multiplex on server's response

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
				if (ret > 0){
					stdinbuff[ret] = '\0';
				}
				else {
					infow("Standard input closed. Exiting...");
					close(server_connfd);
					exit(0);
				}
				strip_char(stdinbuff,'\n');


				process_user_command(stdinbuff);

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
	if ((ret = expect_data(message,message_data, &error_code,1,AUTH)) < 0){
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
	promptnl("Please enter your password:");
	scanf("%s",user_password);
	send_data(server_connfd, PASS, user_password);

	//Receive response
	
	recv_data(server_connfd, recvbuff);
	tokens = tokenize(recvbuff,sprn);
	message = get_token(recvbuff,sprn,0);
	if (expect_data(message, message_data, &error_code, 1, HI) < 0){
		// our password may have been wrong
		print_error(recvbuff,message,error_code);
		send_error(server_connfd, 60, NULL, true);
		return;
	}

	info("Client successfully logged in as existing user: %s", username);



	if (tokens > 1){
		message = get_token(recvbuff,sprn,1);
		if (message != NULL){
			if (expect_data(message, message_data, &error_code, 1, ECHO) < 0){
				// our password may have been wrong
				print_error(message,message_data,error_code);
				send_error(server_connfd, 60, NULL, true);
				return;
			}
			printf(KWHT "%s" KNRM "\n" , message_data);
		}
		else {


			// error("Couldn't get MOTD");



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
	if ((ret = expect_data(message,message_data,&error_code,1,HINEW)) < 0){
		print_error(message,message_data,error_code);
		send_error(server_connfd, 60, NULL, true);
		return;
	}

	//We got the HINEW, let's assert the username is the same
	if (strcmp(username,message_data)){
		error("Server responded with wrong username - Abort");
		close(server_connfd);
		return;
	}

	promptnl("%s",password_rules);
	while(true){
		scanf("%s",user_password);
		if (check_password(user_password))
			break;
		else {
			attempts++;
			if (attempts >= 3){
				promptnl("Too many tries. Sober up and try again later.");
				close(server_connfd);
				exit(1);
			}
			promptnl("Password does not meet criterion. Please try again.");
		}
	}

	info("Sending server password: ---%s---", user_password);
	send_data(server_connfd, NEWPASS, user_password);


	recv_data(server_connfd,recvbuff);
	tokens = tokenize(recvbuff,sprn);
	message = get_token(recvbuff,sprn,0);
	if ((ret = expect_data(message,message_data,&error_code,1,HI)) < 0){
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
				print_error(message,message_data,error_code);
				send_error(server_connfd, 60, NULL, true);
				return;
			}
			printf(KWHT "%s" KNRM "\n" , message_data);
		}
		else {
			// error("Couldn't get MOTD");
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

	//info("Received %s from server.", verbs[AHOLA]);

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
	server_sa.sin_addr.s_addr = inet_addr(server_ip);

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
