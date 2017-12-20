#define _GNU_SOURCE
#include "chatter.h"





bool echo_mode = false;
bool echo_flag = false;
bool echo_running = false;
pthread_t echo_thread;
pthread_attr_t echo_thread_attr;

int server_port;
int num_chat_rooms;
char *motd;
FILE *output;

pool_t *threadpool;

void spawn_login_thread();
void *login_thread_func(void *arg);
void *echo_thread_func(void *arg);
void iam_login();
void iamnew_login();




void spawn_echo_thread(){
	int r;

	if (!echo_flag){
		return;
	}

	if (echo_running){
		return;
	}

	r = pthread_create(&echo_thread,&echo_thread_attr,echo_thread_func,NULL);
	if (r != 0){
		debug("pthread_create of echo_thread failed: %d %s",r,strerror(r));
		return;
	}	


}


//multiplex on all client connections.
void *echo_thread_func(void *arg){
	char recvbuff[MAX_RECV];
	char sendbuff[MAX_SEND];
	struct pollfd fds[MAX_USERS];
	char *username;
	int timeout_ms = 2000;
	user_info_t *user;
	// int poll(struct pollfd *fds, nfds_t nfds, int timeout);

	int ret, num_users, i, flags, n;
	echo_running = true;



	


	debug("Echo thread started... Going into poll loop.");
	while(true){

		// get a list of all the users logged in, to multiplex on.
		lock_user_info(1);
	    user = user_infos;
	    num_users = 0;
	    while(user != NULL){
	    	//if the user is logged in and ready to be multiplexed on
	    	if (user->ready){
	    		fds[num_users].fd = user->connfd;
	   		 	fds[num_users].events = POLLIN;
	   		 	/* Set socket to non-blocking */ 

				if ((flags = fcntl(fds[num_users].fd, F_GETFL, 0)) < 0){ 
				    error("fcntl (get) in echo thread error: %s", strerror(flags));
				} 

				if (fcntl(fds[num_users].fd, F_SETFL, flags | O_NONBLOCK) < 0) 
				{ 
				    error("fcntl (set) in echo thread error.");
				} 

	    		num_users++;
	    	}
	    	user = user->next;
	    }
	    unlock_user_info(1);


	    //ignore the other descriptors.
	    for (i = num_users; i < MAX_USERS; i++){
	    	fds[i].fd = -1;
	    }


		ret = poll(fds, num_users, timeout_ms);
		if (ret > 0) {
		    for (i=0; i<num_users; i++) {
		    	if (fds[i].fd < 0){
		    		debug("continuing");
		    		continue;
		    	}
		        if (fds[i].revents & POLLIN) {
		        	//read from socket.

		        	/* TODO */
		        	// echo_thread_read(fds[i].fd);
		        	/* TODO */

		        	//print out the username of whom sent to us
		        	user = get_user_byfd(fds[i].fd);
		        	if (user != NULL){
		        		username = user->username;
		        	}
		        	else {
		        		error("can't find user by fd.");
		        		exit(1);
		        	}

		        	debug("About to read...");
		        	n = recv_data(fds[i].fd,recvbuff);
		        	if (n < 0){
		        		error("recv_data returned an error: %d %s", n, strerror(n));
		        	}
		        	if (n == 0){
		        		debug("Client disconnected");
		        		//TODO
		        		//logout client
		        		logout_user(username);
		        	}
		        	debug("i: %d, fd: %d", i, fds[i].fd);
		        	debug("Username %s sent: %s",username, recvbuff);

		        }

		        if (fds[i].revents & POLLHUP) {
		        	/* A hangup has occurred on device number i. */
		        	debug("Received a hangup poll.");

		        }

		        if (fds[i].revents & POLLHUP){
					debug("Client disconnected.");
					exit(0);
				}
				if (fds[i].revents & POLLRDHUP){
					debug("Client disconnected.");
					exit(0);
			    }
			}
		}
		

		else {
			debug("echo thread found nothing on read sockets.");
		}


		if (!echo_flag){
			echo_running = false;
			debug("No users, echo thread is leaving...");
			return NULL;
		}
	}
	echo_running = false;
	return NULL;
}

void *check_user_connections_func(void *arg){
	user_info_t *user;
	int err;
	socklen_t err_size = sizeof(err);

// getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);

	while(true){
		lock_user_info(1);
		user = user_infos;
		while (user != NULL){

			getsockopt(user->connfd, SOL_SOCKET, SO_ERROR, &err, &err_size);
			debug("check_user_connections thread. %s socket state: %d %s", user->username, err, strerror(err));

			user = user->next;
		}
		unlock_user_info(1);
		sleep(2);
	}
}

//TODO configure locks for user authentication

void iam_login(int connfd, char *client_username){
	char recvbuff[MAX_RECV];
	int tokens;
	char *message;
	int error_code, ret;
	char message_data[MAX_RECV];

	info("Client with username: \"%s\" is attempting login.", client_username);

	lock_user_info(1);
	if (user_logged_in(client_username)){
		send_error(connfd, 0, NULL, false);
		send_data(connfd, BYE, NULL);
		close(connfd);
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

			handle_error(recvbuff,message_data,error_code);
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
			echo_flag = true;

			unlock_user_info(4);

			unlock_user_accounts(3);

			spawn_echo_thread();
			return;

		}

		else {
			lock_user_info(5);
			logout_user(client_username);
			unlock_user_info(5);

			unlock_user_accounts(3);

			send_error(connfd, 61, NULL, true);
			return;
		}
	}

	else {
		unlock_user_info(2);
		unlock_user_accounts(2);
		send_error(connfd, 2, client_username, false);
		send_data(connfd,BYE,NULL);
		close(connfd);
		return;
	}
}

void iamnew_login(int connfd, char *client_username){
	int tokens;
	char *message;
	char recvbuff[MAX_RECV];
	int error_code, ret;
	char message_data[MAX_RECV];

	info("Client with username: \"%s\" is attempting login and user creation.", client_username);

	lock_user_info(1); 
	if (user_logged_in(client_username)){
		send_error(connfd, 0, NULL, false);
		send_data(connfd, BYE, NULL);
		close(connfd);
		unlock_user_info(1);
		return;
	}
	unlock_user_info(1);

	if (strlen(client_username) < 2){
		info("Username is too short. Sending ERR00 and closing connection.");
		send_error(connfd, 0, NULL, false);
		send_data(connfd,BYE,NULL);
		close(connfd);
		return;
	}
	// if the user exists already
	lock_user_accounts(2);
	if (user_exists(client_username)){
		//tell user no dice
		//close connection.ERR 01 SORRY <name> \r\n
		info("User %s already exists. Sending ERR01 and closing connection.", client_username);
		send_error(connfd, 1, client_username, false);
		send_data(connfd,BYE,NULL);
		close(connfd);
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

		lock_user_info(3);
		mark_user_ready(client_username);
		unlock_user_info(3);

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
	recv_data(connfd,recvbuff);
	tokens = tokenize(recvbuff,sprn);
	message = get_token(recvbuff,sprn,0);
	if ((ret = expect_data(recvbuff, message_data, &error_code,2, IAM, IAMNEW)) < 0){
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
	init_user_mutexes();
	pthread_attr_init(&echo_thread_attr);

	threadpool = pool_create(2,4,500,NULL);

	// pool_queue(threadpool,check_user_connections_func,NULL);


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
