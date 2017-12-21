#define _GNU_SOURCE
#include "chatter.h"

bool echo_mode = false;
bool echo_flag = false;
bool echo_running = false;
pthread_t echo_thread;
pthread_attr_t echo_thread_attr;


void process_request(char *username, char *recvbuff){

}



// in the waiting room, users can:
// 1) Create a room
// 2) Join a chat room
// 3) List the chat rooms
// 4) Logout 

//recvbuff contains full message.
void process_wait_room_request(char *username, char *recvbuff){
	int tokens, ret, error_code, connfd;
	char message_data[MAX_RECV];
	char *message_body;
	user_info_t *user;

	lock_user_info(1);
	user = get_user_byname(username);
	unlock_user_info(1);

	if (user == NULL){
		error("process_wait_room_request found request for non-existent user: %s",username);
		assert(false);
	}

	connfd = user->connfd;



	tokens = tokenize(recvbuff,sprn);
	message_body = get_token(recvbuff,sprn,0);

	if ((ret = expect_data(recvbuff,message_data, &error_code,
		4,CREATER, LISTR, JOIN, BYE)) < 0){
		print_error(recvbuff,message_data,error_code);
		send_error(connfd, 60, NULL, true);
		return;
	}

	switch(ret){
		case CREATER: {
			debug("Received C");
			//TODO leftoff
			break;
		}
		case LISTR: {
			break;
		}
		case JOIN: {
			break;
		}
		case BYE: {
			break;
		}
	}

}




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

		    	//get the username of the connected person.
		    	lock_user_info(1);
				user = get_user_byfd(fds[i].fd);
	        	if (user != NULL){
	        		username = user->username;
	        	}

	        	else {
	        		username = "UNKNOWN_USER";
	        		error("can't find user by fd: %d",fds[i].fd);
	        	}
	        	unlock_user_info(1);


		        if (fds[i].revents & POLLIN) {
		        	//read from socket.

		        	/* TODO */
		        	// echo_thread_read(fds[i].fd);
		        	/* TODO */

		        	//print out the username of whom sent to us
		        	

		        	debug("About to read...");
		        	n = recv_data(fds[i].fd,recvbuff);
		        	if (n < 0){
		        		errorw("recv_data returned an error: %s", strerror(errno));
		        		infow("%s disconnected", username);
		        		logout_user(username);
		        	}
		        	if (n == 0){
		        		infow("%s disconnected", username);
		        		//TODO
		        		//logout client
		        		logout_user(username);
		        	}
		        }

		        if (fds[i].revents & POLLHUP) {
		        	infow("%s disconnected", username);
		        	logout_user(username);
		        }


				if (fds[i].revents & POLLRDHUP){
		        	infow("%s disconnected", username);
		        	logout_user(username);
					
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