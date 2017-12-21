#include "chatter.h"


// typedef struct room_member room_member_t;
// struct room_member {
// 	user_info_t *user;
// 	room_member_t *next;
// };


// typedef struct room room_t;
// struct room {
// 	char room_name[MAX_USERNAME];
// 	bool private_room;
// 	room_member_t *room_members;
// 	room_t *next;
// };

room_t *rooms;
size_t num_rooms = 0;
int room_inc_id = 1; // easier to start from 1 because of strtol on client side


//checks if room exists first
int create_room(char *room_name, user_info_t *user, bool private_room, char *password){
	room_t *new_room, *check_room;
	room_member_t *first_user;

	check_room = get_room(room_name);
	if (check_room != NULL){
		debugw("create_room: room with name %s already exists.", room_name);
		return 10;
	}

	if (num_rooms >= MAX_ROOMS){
		debugw("create_room: maximum number of rooms reached already.");
		return 11;
	}

	//allocate structs
	new_room = calloc(1,sizeof(room_t));
	first_user = calloc(1,sizeof(room_member_t));

	//point the room_member_t to the user_info_t
	first_user->user = user;
	first_user->owner = true;
	first_user->next = NULL;

	user->in_room = true;

	//copy in the fields into the room struct
	strcpy(new_room->room_name,room_name);
	new_room->private_room = private_room;
	new_room->room_members = first_user;
	new_room->room_id = room_inc_id++;

	if (private_room){
		strcpy(new_room->password, password);
	}

	//add the room to the linked list
	new_room->next = rooms;
	rooms = new_room;

	return 0;

}


//int destroy_room
//int join_room


// "get a room" - lol
room_t *get_room(char *room_name){
	room_t *cur_room;

	cur_room = rooms;
	while (cur_room != NULL){
		if (!strcmp(cur_room->room_name, room_name)){
			return cur_room;
		}

		cur_room = cur_room->next;
	}

	return NULL;
}

room_t *get_room_by_id(int room_id){
	room_t *cur_room;

	cur_room = rooms;
	while (cur_room != NULL){
		if (cur_room->room_id == room_id){
			return cur_room;
		}

		cur_room = cur_room->next;
	}

	return NULL;
}

int free_room_members(room_member_t *room_members){
	room_member_t *save;

	while(room_members != NULL){
		save = room_members;
		room_members->user->in_room = false;
		room_members = room_members->next;
		free(save);
	}

	return 0;
}

int add_room_member(room_t *room, user_info_t *user, char *password){
	room_member_t *new_room_member;

	if (user == NULL){
		error("Cannot add null user.");
		return 100;
	}


	if (room->private_room){
		if (strcmp(room->password,password)){
			return 61;
		}
	}

	//allocate and populate the new room_member struct
	new_room_member = calloc(1,sizeof(room_member_t));
	new_room_member->owner = false;
	new_room_member->user = user;

	user->in_room = true;

	//add it to the linked list
	new_room_member->next = room->room_members;
	room->room_members = new_room_member;

	return 0;
}


int remove_room_member(room_t *room, char *username){
	room_member_t *cur_member, *prev;
	room_t *iter_room, *prev_room;
	char message_data[MAX_SEND];

	prev = NULL;
	cur_member = room->room_members;

	while(cur_member != NULL){
		if (!strcmp(cur_member->user->username,username)){

			if (prev == NULL){
				room->room_members = cur_member->next;
			}
			else {
				prev->next = cur_member->next;
			}

			sprintf(message_data,"User %s has left room %d %s.",cur_member->user->username,room->room_id,room->room_name);
			echo_all_room(room,ECHO,message_data);

			cur_member->user->in_room = false;
			free(cur_member);

			//set the owner to the next user
			if (room->room_members != NULL){
				room->room_members->owner = true;
			}


			// otherwise, nobody in it, close the room
			else {
				prev_room = NULL;
				iter_room = rooms;
				while(iter_room != NULL){
					if (iter_room == room){
						if (prev_room == NULL){
							rooms = iter_room->next;
						}
						else {
							prev_room->next = iter_room->next;
						}

						free_room_members(iter_room->room_members);	
						free(iter_room);
						return 0;
					}

					prev_room = iter_room;
					iter_room = iter_room->next;
				}
			}
			return 0;
		}
		prev = cur_member;
		cur_member = cur_member->next;
	}

	// debug("remove room member couldn't find room member with username %s", username);
	return 1;
}

//happens on logout
int remove_user_from_rooms(char *username){
	room_t *cur_room;
	int ret;

	cur_room = rooms;
	while (cur_room != NULL){

		ret = remove_room_member(cur_room,username);
		if (ret == 0){
			return 0;
		}

		cur_room = cur_room->next;
	}
	infow("User %s was not found in any rooms.", username);
	return 1;
}

int list_rooms(char *sendbuff){
	char room_info[64];
	room_t *cur_room;
	int num_rooms;
	int length;
	int offset;

	memset(room_info,0,64);
	num_rooms = 0;
	sprintf(sendbuff,"RTSIL " );
	offset = strlen("RTSIL ");

	cur_room = rooms;
	while (cur_room != NULL){
		sprintf(room_info,"%s %d\r\n",cur_room->room_name, cur_room->room_id);
		length = strlen(room_info);
		sprintf(sendbuff + offset,"%s",room_info);
		offset+=length;
		cur_room = cur_room->next;
		num_rooms++;
	}
	sprintf(sendbuff+offset,"\r\n");
	return num_rooms;
}

// void list_users(char *sendbuff){
// 	char user_info[64];
// 	room_t *cur_room;
// 	int length;
// 	int offset;

// 	memset(room_info,0,64);

// 	sprintf(sendbuff,"RTSIL " );
// 	offset = strlen("RTSIL ");

// 	cur_room = rooms;
// 	while (cur_room != NULL){
// 		sprintf(room_info,"%s %d \r\n",cur_room->room_name, cur_room->room_id);
// 		length = strlen(room_info);
// 		sprintf(sendbuff + offset,"%s",room_info);
// 		offset+=length;
// 		cur_room = cur_room->next;
// 	}
// 	sprintf(sendbuff+offset,"\r\n");
// }


int check_room(room_t *room){
	if (room->room_members == NULL){
		return 1;
	}
	return 0;
}

int close_room_by_name(char *room_name){
	room_t *cur_room, *prev;

	cur_room = rooms;
	prev = NULL;
	while (cur_room != NULL){
		if (!strcmp(cur_room->room_name, room_name)){

			//if the room of interest is first in the list, have to change
			//the global rooms pointer as well
			if(prev == NULL){
				rooms = cur_room->next;
			}
			else {
				prev->next = cur_room->next;
			}

			free_room_members(cur_room->room_members);	
			free(cur_room);
			return 0;
		}

		prev = cur_room;
		cur_room = cur_room->next;
	}

	error("Tried to remove rooom (%s) that doesn't exist.", room_name); 
	return 1;
}


// int close_room(room_t *room){
// 	room_t *cur_room, *prev;

// 	cur_room = rooms;
// 	prev = NULL;
// 	while (cur_room != NULL){
// 		if (!strcmp(cur_room->room_name, room_name)){

// 			//if the room of interest is first in the list, have to change
// 			//the global rooms pointer as well
// 			if(prev == NULL){
// 				rooms = cur_room->next;
// 			}
// 			else {
// 				prev->next = cur_room->next;
// 			}

// 			free_room_members(cur_room->room_members);	
// 			free(cur_room);
// 			return 0;
// 		}

// 		prev = cur_room;
// 		cur_room = cur_room->next;
// 	}

// 	error("Tried to remove rooom (%s) that doesn't exist.", room_name); 
// 	return 1;
// }


