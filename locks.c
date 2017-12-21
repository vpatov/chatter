#include "chatter.h"


/* ----------- User info and accounts ------------ */
/* ----------------------------------------------- */

pthread_mutex_t user_account_mutex;
pthread_mutex_t user_info_mutex;

void init_user_mutexes(){
	pthread_mutexattr_t user_account_mutex_attr, user_info_mutex_attr;

	pthread_mutexattr_init(&user_account_mutex_attr);
	pthread_mutexattr_init(&user_info_mutex_attr);

	pthread_mutexattr_settype(&user_account_mutex_attr,PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutexattr_settype(&user_info_mutex_attr,PTHREAD_MUTEX_ERRORCHECK);
		
	pthread_mutex_init(&user_account_mutex, &user_account_mutex_attr);   
	pthread_mutex_init(&user_info_mutex, &user_info_mutex_attr);  
}

void lock_user_accounts(int track){
	pthread_mutex_lock(&user_account_mutex);
}

void unlock_user_accounts(int track){
	pthread_mutex_unlock(&user_account_mutex);
}

void lock_user_info(int track){
	pthread_mutex_lock(&user_info_mutex);
}

void unlock_user_info(int track){
	pthread_mutex_unlock(&user_info_mutex);
}

/* ----------------------------------------------- */
/* ----------------------------------------------- */




/* ------- Room structs and room members --------- */
/* ----------------------------------------------- */

pthread_mutex_t room_mutex;

void init_room_mutexes(){
	pthread_mutexattr_t room_mutex_attr;

	pthread_mutexattr_init(&room_mutex_attr);

	pthread_mutexattr_settype(&room_mutex_attr,PTHREAD_MUTEX_ERRORCHECK);
		
	pthread_mutex_init(&room_mutex, &room_mutex_attr);   
}

void lock_rooms(int track){
	pthread_mutex_lock(&room_mutex);
}

void unlock_rooms(int track){
	pthread_mutex_unlock(&room_mutex);
}


/* ----------------------------------------------- */
/* ----------------------------------------------- */