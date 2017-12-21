#define _GNU_SOURCE
#include "chatter.h"

bool echo_mode = false;
bool echo_flag = false;
bool echo_running = false;
pthread_t echo_thread;
pthread_attr_t echo_thread_attr;


void relieve_user(char *username){
	lock_rooms(1);
	remove_user_from_rooms(username);
	unlock_rooms(1);
	lock_user_info(1);
	logout_user(username);
	unlock_user_info(1);
}

void process_request(char *username, char *recvbuff){

}

void echo_all(int verb, char *message_data){
	user_info_t *cur_user;
	lock_user_info(1);
	cur_user = user_infos;
	while(cur_user != NULL){
		if (cur_user->ready){
			send_data(cur_user->connfd,verb,message_data);
		}
		cur_user = cur_user->next;
	}
	unlock_user_info(1);
}

void echo_all_waiting(int verb, char *message_data){
	user_info_t *cur_user;
	lock_user_info(1);
	cur_user = user_infos;
	while(cur_user != NULL){

		if (cur_user->ready && cur_user->room == NULL){
			send_data(cur_user->connfd,verb,message_data);
		}
		cur_user = cur_user->next;
	}
	unlock_user_info(1);
}

void echo_all_room(room_t *room, int verb, char *message_data){
	room_member_t *room_member;

	if (room == NULL){
		error("echo_all_room: room is NULL");
		return;
	}

	room_member = room->room_members;

	while(room_member != NULL){
		send_data(room_member->user->connfd,ECHO,message_data);
		room_member = room_member->next;
	}
}

void echor_room(room_t *room, int verb, char *message_data){
	room_member_t *room_member;

	if (room == NULL){
		error("echor_room: room is NULL");
		return;
	}

	room_member = room->room_members;

	while(room_member != NULL){
		send_data(room_member->user->connfd,ECHOR,message_data);
		room_member = room_member->next;
	}
}

int echo_user(char *username, char *message, room_t *sender_room){
	user_info_t *user;

	user = get_user_byname(username);
	if (user != NULL){
		if (user->room == sender_room){
			send_data(user->connfd,ECHOP,message);
		}
		else {
			return 30;
		}
		return 0;
	}
	return 30;
}

int echo_kick_user(char *username, char *message){
	user_info_t *user;

	user = get_user_byname(username);
	if (user != NULL){
		send_data(user->connfd,ECHOP,message);
		return 0;
	}
	return 30;
}


// in the waiting room, users can:
// 1) Create a room
// 2) Join a chat room
// 3) List the chat rooms
// 4) Logout 

//recvbuff contains full message.
void process_wait_room_request(char *username, char *recvbuff){
	int tokens, ret, error_code, connfd, room_id;
	char message_data[MAX_RECV];
	char sendbuff[MAX_SEND];
	char *message_body;
	char *dest_username, *saveptr;
	char *room_name, *room_id_str, *password;
	user_info_t *user;
	room_t *room;
	room_member_t *member;

	lock_user_info(1);
	user = get_user_byname(username);
	unlock_user_info(1);

	//That means user disconnected before we got to finish
	if (user == NULL){
		return;
	}

	connfd = user->connfd;



	tokens = tokenize(recvbuff,sprn);
	if (tokens == 0){
		error("tokenize: malformed message");
	}
	message_body = get_token(recvbuff,sprn,0);


	if (user->room != NULL){
		if ((ret = expect_data(recvbuff,message_data, &error_code,
			8,LEAVE,LISTU,TELL,KICK,QUIT,ECHO,ECHOR,ECHOP)) < 0){
			print_error(recvbuff,message_data,error_code);
			send_error(connfd, 60, NULL, false);
			return;
		}
		switch(ret){
			case LEAVE: {

				send_data(connfd,EVAEL,NULL);
				sprintf(sendbuff,"%s has left the room.", username);
				lock_rooms(1);
				echo_all_room(user->room,ECHO,sendbuff);
				remove_user_from_rooms(username);
				unlock_rooms(1);
				return;
			}

			case LISTU: {
				lock_rooms(1);
				list_users(user->room,sendbuff);
				unlock_rooms(1);
				send_data_custom(connfd,sendbuff);
				return;
				break;
			}

			case ECHOR: {
				sprintf(sendbuff,"%s %s",username,message_data);
				lock_rooms(1);
				echor_room(user->room,ECHOR,sendbuff);
				unlock_rooms(1);
				return;

			}

			case TELL: {
				
				dest_username = strtok_r(message_data,space,&saveptr);
				sprintf(sendbuff,"%s %s",username, saveptr);
				lock_user_info(1);
				if (echo_user(dest_username,sendbuff,user->room)){
					send_error(connfd,30,NULL,false);
				}
				unlock_user_info(1);
				return;
			}

			case KICK: {
				bool sanity = false;
				lock_rooms(1);
				member = user->room->room_members;
				while(member != NULL){
					if (member->owner){
						if (member->user != user){
							unlock_rooms(1);
							send_error(connfd,40,NULL,false);
							return;
						}
						else {
							sanity = true;
						}
					}
					member = member->next;
				}
				unlock_rooms(1);

				if (!sanity){
					debug("Room %s has no owner.", user->room->room_name);
					send_error(connfd,40,NULL,false);
				}

				tokens = tokenize(message_data,space);
				if (tokens != 1){
					send_error(connfd, 60, NULL, false);
					return;
				}

				dest_username = get_token(message_data,space,0);

				lock_rooms(1);
				if (remove_room_member(user->room,dest_username)){
					unlock_rooms(1);
					send_error(connfd,41,NULL,false);
					return;
				}

				sprintf(sendbuff,"%s has been kicked out.",dest_username);
				echo_all_room(user->room,ECHO,sendbuff);
				unlock_rooms(1);

				sprintf(sendbuff, "%s kicked you out of the room!", username);
				lock_user_info(1);
				echo_kick_user(dest_username,sendbuff);
				unlock_user_info(1);

				return;
			}

			case QUIT: {
				relieve_user(username);

				sprintf(sendbuff,"%s has logged out of the network.", username);
				echo_all(ECHO,sendbuff);
				sprintf(sendbuff,"%s has left the room.",username);
				lock_rooms(1);
				echo_all_room(user->room,ECHO,sendbuff);
				unlock_rooms(1);
				return;
			}

			// case ECHOP: {
			// 	if (!user->room->private_room){

			// 	}
			// }
		}	
	}

	else {
		if ((ret = expect_data(recvbuff,message_data, &error_code,
			6,CREATER, CREATEP, LISTR, JOIN, JOINP, QUIT)) < 0){
			print_error(recvbuff,message_data,error_code);
			send_error(connfd, 60, NULL, false);
			return;
		}
		switch(ret){
			case CREATER: {
				//get args
				tokens = tokenize(message_data,space);
				if (tokens != 1){
					send_error(connfd, 60, NULL, false);
					return;
				}

				room_name = get_token(message_data,space,0);
				lock_rooms(1);
				ret = create_room(room_name, user, false, NULL);
				unlock_rooms(1);
				if (ret != 0){
					send_error(connfd,ret,NULL,false);
					return;
				}
				else {
					send_data(connfd,RETAERC,room_name);

					sprintf(sendbuff,"New chat room added: %s",room_name);
					echo_all_waiting(ECHO,sendbuff);
				}
				return;
			}
			case CREATEP: {
				tokens = tokenize(message_data,space);
				if (tokens != 2){
					send_error(connfd, 60, NULL, false);
					return;
				}

				room_name = get_token(message_data,space,0);
				password = get_token(message_data,space,1);

				lock_rooms(1);
				ret = create_room(room_name, user, true, password);
				unlock_rooms(1);
				if (ret != 0){
					send_error(connfd,ret,NULL,false);
					return;

				}
				else {
					send_data(connfd,PETAERC,room_name);

					sprintf(sendbuff,"New private chat room added: %s",room_name);
					echo_all_waiting(ECHO,sendbuff);
				}
				return;
			}
			case LISTR: {
				lock_rooms(1);
				ret = list_rooms(sendbuff);
				unlock_rooms(1);
				if (ret == 0){
					sprintf(sendbuff,"RTSIL no_rooms -1\r\n\r\n");
					send_data_custom(connfd,sendbuff);
				}
				else {
					send_data_custom(connfd,sendbuff);
				}
				break;
			}
			case JOIN: {
				tokens = tokenize(message_data,space);
				if (tokens != 1){
					send_error(connfd, 60, NULL, false);
					return;
				}

				room_id_str = get_token(message_data,space,0);
				room_id = strtol(room_id_str,NULL,10);
				if (room_id == 0){
					send_error(connfd, 60, NULL, false);
					return;
				}

				lock_rooms(2);
				room = get_room_by_id(room_id);

				//if no such room
				if (room == NULL){
					unlock_rooms(2);
					send_error(connfd, 20, room_id_str,false);
					return;
				}

				//if room is private (we're using JOIN not JOINP)
				if (room->private_room){
					unlock_rooms(2);
					send_error(connfd,21,room_id_str,false);
					return;
				}

				ret = add_room_member(room,user,NULL);

				if (ret == 0){
					send_data(connfd,NIOJ,room_id_str);
					sprintf(sendbuff,"User %s has joined this room (%s).",username,room->room_name);
					echo_all_room(room,ECHO,sendbuff);
					unlock_rooms(2);

				}
				else {
					unlock_rooms(2);
					send_error(connfd, 61, NULL, false);
					return;
				}
				return;
			}
			case JOINP: {
				tokens = tokenize(message_data,space);
				if (tokens != 2){
					send_error(connfd, 60, NULL, false);
					return;
				}

				room_id_str = get_token(message_data,space,0);
				password = get_token(message_data,space,1);

				room_id = strtol(room_id_str,NULL,10);
				if (room_id == 0){
					send_error(connfd, 60, NULL, false);
					return;
				}

				lock_rooms(2);
				room = get_room_by_id(room_id);

				if (room == NULL){
					unlock_rooms(2);
					send_error(connfd, 20, room_id_str,false);
					return;
				}

				ret = add_room_member(room,user,password);

				if (ret == 0){
					send_data(connfd,PNIOJ,room_id_str);
					sprintf(sendbuff,"User %s has joined this room (%s).",username,room->room_name);
					echo_all_room(room,ECHO,sendbuff);
					unlock_rooms(2);

				}
				else {
					unlock_rooms(2);
					send_error(connfd, 61, NULL, false);
					return;
				}
				return;

			}
			case QUIT: {

				relieve_user(username);

				sprintf(sendbuff,"%s has logged out of the network.", username);
				echo_all(ECHO,sendbuff);
				break;
			}

			case ERR: {
				infow("Got an error from user %s: %d %s",username,error_code,message_data);
				break;
			}
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
	room_t *cur_room, *save;
	// int poll(struct pollfd *fds, nfds_t nfds, int timeout);

	int ret, num_users, i, flags, n;
	echo_running = true;


	debug("Echo thread started... Going into poll loop.");
	while(true){

		lock_rooms(1);
		cur_room = save = rooms;
		while (cur_room != NULL){

			//if room has nobody in it
			if (check_room(cur_room)){
				close_room_by_name(cur_room->room_name);
				break;
			}
			save = cur_room;
			cur_room = cur_room->next;
		}
		unlock_rooms(1);


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
				    error("fcntl (get) in echo thread error: %s", strerror(errno));
				} 

				if (fcntl(fds[num_users].fd, F_SETFL, flags | O_NONBLOCK) < 0) 
				{ 
				    error("fcntl (set) in echo thread error.");
				} 

	    		num_users++;
	    	}

	    	// send_data(user->connfd, NOP, "keep alive");
	    	user = user->next;
	    }
	    unlock_user_info(1);
	    // debug("Echo thread found %d users.", num_users);


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


		        	
		        	//debug("About to read...");
		        	memset(recvbuff,0,MAX_RECV);
		        	n = recv_data(fds[i].fd,recvbuff);
		        	if (n < 0){
		        		errorw("recv_data returned an error: %s", strerror(errno));
		        		infow("%s disconnected", username);

		        		relieve_user(username);
		        	}
		        	if (n == 0){
		        		infow("%s disconnected", username);
		        		
		        		relieve_user(username);
		        	}
		        	else {
		        		//TODO 

		        		process_wait_room_request(username,recvbuff);
		        		//respond to command.
		        	}
		        }

		        if (fds[i].revents & POLLHUP) {
		        	infow("%s disconnected", username);
	        		relieve_user(username);
		        }


				if (fds[i].revents & POLLRDHUP){
		        	infow("%s disconnected", username);
		        	
	        		relieve_user(username);

					
			    }
			}
		}
		

		else {
			//debug("echo thread found nothing on read sockets.");
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