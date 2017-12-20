#include "chatter.h"


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