#include "chatter.h"

char motd[MAX_MSG_SIZE];

void iam_login_server(int connfd, char *client_username){
	char recvbuff[MAX_RECV];
	int tokens;
	char *message;
	int error_code, ret;
	char message_data[MAX_RECV];

	info("Client with username: \"%s\" is attempting login.", client_username);

	lock_user_info(1);
	if (user_logged_in(client_username)){
		send_error(connfd, 0, NULL, true);
		unlock_user_info(1);
		return;
	}

	lock_user_accounts(2);
	if (user_exists(client_username)){
		//prompt client to send password
		send_data(connfd,AUTH,client_username);
		info("User exists, waiting for client to send password.");



		login_user(client_username,connfd);
		unlock_user_info(1);
		unlock_user_accounts(2);


		recv_data(connfd,recvbuff);

		lock_user_accounts(3);
		tokens = tokenize(recvbuff,sprn);
		message = get_token(recvbuff,sprn,0);
		if ((ret = expect_data(recvbuff,message_data,&error_code,1,PASS)) < 0){

			print_error(recvbuff,message_data,error_code);
			send_error(connfd, 60, NULL, true);
			logout_user(client_username);
			unlock_user_accounts(3);

			return;
		}

		//if user is authenticated successfully
		if (authenticate_user(client_username,message_data) == 0){
			send_data(connfd, HI, client_username);


			lock_user_info(4);
			mark_user_ready(client_username);

			unlock_user_info(4);

			unlock_user_accounts(3);

			//send message of the day
			send_data(connfd,ECHO,motd);
			echo_flag = true;
			
			spawn_echo_thread();

			return;

		}

		else {
			lock_user_info(5);
			send_error(connfd, 61, NULL, true);
			logout_user(client_username);
			unlock_user_info(5);

			unlock_user_accounts(3);

			return;
		}
	}

	else {
		unlock_user_info(2);
		unlock_user_accounts(2);
		send_error(connfd, 2, client_username, true);
		return;
	}
}



void iamnew_login_server(int connfd, char *client_username){
	int tokens;
	char *message;
	char recvbuff[MAX_RECV];
	int error_code, ret;
	char message_data[MAX_RECV];

	info("Client with username: \"%s\" is attempting login and user creation.", client_username);

	lock_user_info(1); 
	if (user_logged_in(client_username)){
		send_error(connfd, 1, client_username, true);
		unlock_user_info(1);
		return;
	}
	unlock_user_info(1);

	if (strlen(client_username) < 2){
		info("Username is too short. Sending ERR00 and closing connection.");
		send_error(connfd, 0, NULL, true);
		return;
	}
	// if the user exists already
	lock_user_accounts(2);
	if (user_exists(client_username)){
		//tell user no dice
		//close connection.ERR 01 SORRY <name> \r\n
		info("User %s already exists. Sending ERR01 and closing connection.", client_username);
		send_error(connfd, 1, client_username, true);
		unlock_user_accounts(2);
		return;
	}

	else {
		unlock_user_accounts(2);

		fill_user_slot(client_username);

		send_data(connfd,HINEW,client_username);

		recv_data(connfd,recvbuff);
		tokens = tokenize(recvbuff,sprn);
		message = get_token(recvbuff,sprn,0);
		//expecting NEWPASS verb
		if ((ret = expect_data(recvbuff,message_data,&error_code,1,NEWPASS)) < 0){
			print_error(recvbuff,message_data,error_code);
			release_user_slot(client_username);
			send_error(connfd, 60, NULL, true);
			return;
		}

		//check password
		if (!check_password(message_data)){
			release_user_slot(client_username);
			send_error(connfd, 61, NULL, true);
			return;
		}

		//make the user
		if ((ret = create_user(client_username, message_data))){
			release_user_slot(client_username);
			send_error(connfd, ret, NULL, true);
			return;
		}

		//successful creation, send HI
		//if no bytes send, might be disconnected. don't log them in
		if (send_data(connfd, HI, client_username) == 0){
			return;
		}

		lock_user_info(3);
		login_user(client_username,connfd);
		mark_user_ready(client_username);
		unlock_user_info(3);

		//send message of the day

		send_data(connfd,ECHO,motd);
		echo_flag = true;
		spawn_echo_thread();
		return;
		//process_loop();

	}
}


//should receive connected socket as argument.
void *login_thread_func(void *arg){
	thread_arg_t *thread_arg = arg;
	int connfd, ret, error_code, tokens;
	char message_data[MAX_RECV];
	char recvbuff[MAX_RECV];
	char *message;


	connfd = *(int*)(thread_arg->arg);
	info("Waiting to receive: %d", connfd);

	//Receive first ALOHA! from client
	recv_data(connfd,recvbuff);
	tokens = tokenize(recvbuff,sprn);
	message = get_token(recvbuff,sprn,0);

	if ((ret = expect_data(recvbuff,NULL,&error_code,1,ALOHA)) < 0){
		print_error(recvbuff,NULL,error_code);
		send_error(connfd, 60, NULL, true);
		return NULL;
	}


	//Received aloha, proceed to send !AHOLA
	//info("aloha received");
	send_data(connfd,AHOLA,NULL);
	// snprintf(sendbuff,MAX_SEND,"%s%s",verbs[AHOLA],rn);
	// send(connfd,sendbuff,strlen(sendbuff),0);


	//Wait for command from client. Either IAM or IAMNEW
	//info("Waiting for client to send verb.");
	recv_data(connfd,recvbuff);
	tokens = tokenize(recvbuff,sprn);
	message = get_token(recvbuff,sprn,0);
	if ((ret = expect_data(recvbuff, message_data, &error_code,2, IAM, IAMNEW)) < 0){
		print_error(recvbuff,message_data,error_code);
		send_error(connfd,60, NULL, true);
		return NULL;
	}


	if (ret == IAM)
		iam_login_server(connfd, message_data);
	else if (ret == IAMNEW)
		iamnew_login_server(connfd, message_data);
	else {
		error("expect_data has bugs in it.");
		assert(false);
	}

	return NULL;
}



