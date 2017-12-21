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


//assumes room with given name doesn't exist already
int create_room(char *room_name, user_info_t *user, bool private_room){
	room_t *new_room, *check_room;
	room_member_t *first_user;

	check_room = get_room(room_name);
	if (check_room != NULL){
		error("create_room: room with name %s already exists.", room_name);
		return -1;
	}

	//allocate structs
	new_room = calloc(1,sizeof(room_t));
	first_user = calloc(1,sizeof(room_member_t));

	//point the room_member_t to the user_info_t
	first_user->user = user;
	first_user->owner = true;
	first_user->next = NULL;

	//copy in the fields into the room struct
	strcpy(new_room->room_name,room_name);
	new_room->private_room = private_room;
	new_room->room_members = first_user;

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

int free_room_members(room_member_t *room_members){
	room_member_t *save;

	while(room_members != NULL){
		save = room_members;
		room_members = room_members->next;
		free(save);
	}

	return 0;
}

int add_room_member(room_t *room, char *username){
	user_info_t *user;
	room_member_t *new_room_member;
	user = get_user_byname(username);

	if (user == NULL){
		error("Cannot add user %s to room because user doesn't exist.", username);
		return 1;
	}

	//allocate and populate the new room_member struct
	new_room_member = calloc(1,sizeof(room_member_t));
	new_room_member->owner = false;
	new_room_member->user = user;

	//add it to the linked list
	new_room_member->next = room->room_members;
	room->room_members = new_room_member;

	return 0;
}


int remove_room_member(room_t *room, char *username){
	room_member_t *cur_member, *prev;

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

			free(cur_member);
			return 0;
		}
		prev = cur_member;
		cur_member = cur_member->next;
	}

	error("remove room member couldn't find room member with username %s", username);
	return 1;
}

int close_room(char *room_name){
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



