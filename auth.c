#include "chatter.h"

user_account_t user_accounts[MAX_USERS];
size_t num_user_accounts = 0;

user_info_t *user_infos;


bool user_exists(char *username){
	FILE *fp;
	char *line, *cur_username, *saveptr;
	size_t length = 0;
	size_t bytes_read = 0;


	fp = fopen(accounts_filename,"r");
	if (fp == NULL){
		return false;
	}

	while ((bytes_read = getline(&line, &length, fp)) != -1) {
        strip_char(line,'\n');
        cur_username = strtok_r(line,":",&saveptr);
        if (cur_username == NULL){
        	error("user accounts file is malformed.");
        	exit(1);
        }
        if (!strcmp(username,cur_username)){
        	fclose(fp);
        	if (line)
    			free(line);
        	return true;
        }

    }
    if (line)
    	free(line);

    fclose(fp);
    return false;
}




/*
passwords must meet the f ollowing criteria to be valid:
Must be at least 5 characters in leng th
Must contain at least 1 uppercase character
Must contain at least 1 symbol character
Must contain at least 1 number character
*/
bool check_password(char *password){
	int length, upper_count, symbol_count, number_count;
	char *p;

	length = upper_count = symbol_count = number_count = 0;
	for (p = password; *p != '\0'; p++){
		if (*p >= 'A' && *p <= 'Z')
			upper_count++;
		if (*p >= '0' && *p <= '9')
			number_count++;
		if (*p == '!' || *p == '@' || *p == '#' || *p == '$')
			symbol_count++;


		//if character is outside of the acceptable range ([!@#$0-9a-zA-Z])
		if (!((*p >= 48 && *p <= 57) || (*p >=64 && *p <= 90) || (*p >= 97 && *p <= 122) || *p == 33 || *p == 35 || *p == 36)){
			return false;
		}
		length++;
	}
	if (!(upper_count && number_count && symbol_count))
		return false;
	return length >= 5;
}




int fill_user_slot(char *username){
	FILE *fp;
	char blank_digest[SHA_DIGEST_LENGTH*2];



	fp = fopen(accounts_filename,"a");
	if (fp == NULL){
		return -100;
	}


	for (int i = 0; i < SHA_DIGEST_LENGTH; i++){
		snprintf(blank_digest + i,2,"%02x",0);
	}

	fprintf(fp,"%s:%08x:%s\n",username,0,blank_digest);
	fclose(fp);

	return 0;
}

int release_user_slot(char *username){
	user_account_t *cur_user;
	size_t user_index = 0;

	if (get_user_accounts() == NULL){
		error("Couldn't get user accounts from file.");
		return -100;
	}

	

	while (user_index < num_user_accounts){
		cur_user = &user_accounts[user_index++];
		if (!strcmp(cur_user->username,username)){
			cur_user->username[0] = '\0';
		}
	}
	return 0;	
}

int authenticate_user(char *username, char *password){
	user_account_t *cur_user;
	size_t user_index = 0;
	unsigned char salted_password[MAX_PASSWORD + (sizeof(unsigned int) * 2)];
	unsigned char hash[SHA_DIGEST_LENGTH];
	char hash_digest[SHA_DIGEST_LENGTH*2]; //2 hexadecimal characters to represent each byte

	if (get_user_accounts() == NULL){
		error("Couldn't get user accounts from file.");
		return -100;
	}

	while (user_index < num_user_accounts){
		cur_user = &user_accounts[user_index++];

		//we found our user. Hash the password and salt and compare the hashes
		if (!strcmp(cur_user->username,username)){
			//get user's salted password
			snprintf((char*)salted_password,sizeof(salted_password),"%s%s",password,cur_user->salt);

			//hash the salted password
			info("hashing salted_password: %s",salted_password);
			SHA1(salted_password, strlen((char*)salted_password), hash);

			//get the printable digest
			for (int i = 0; i < SHA_DIGEST_LENGTH*2; i++){
				snprintf(hash_digest + i,2,"%02x",hash[i]);
			}

			//compare it to the database
			if (!strcmp(hash_digest,cur_user->password_hash)){
				return 0;
			}
			else {
				return -61;
			}	
		}
	}

	return -100;


}



user_account_t* get_user_accounts(){
	FILE *fp;
	size_t user_index = 0;
	user_account_t *cur_user;
	char *line, *cur_username, *cur_salt, *cur_hash,*saveptr;
	size_t length = 0;
	size_t bytes_read = 0;


	fp = fopen(accounts_filename,"r");
	if (fp == NULL){
		return false;
	}

	while ((bytes_read = getline(&line, &length, fp)) != -1) {
        strip_char(line,'\n');
        cur_username = strtok_r(line,":",&saveptr);
        if (cur_username == NULL){
        	error("user accounts file is malformed.");
        	fclose(fp);
        	return NULL;
        }
        cur_salt = strtok_r(NULL,":",&saveptr);
        if (cur_salt == NULL){
        	error("user accounts file is malformed.");
        	fclose(fp);
        	return NULL;
        }
        cur_hash = strtok_r(NULL,":",&saveptr);
        if (cur_hash == NULL){
        	fclose(fp);
        	error("user accounts file is malformed.");
        	return NULL;
        }


        cur_user = &user_accounts[user_index++];
        strcpy(cur_user->username,cur_username);
        strcpy(cur_user->salt,cur_salt);
        strcpy(cur_user->password_hash,cur_hash);

    }
    num_user_accounts = user_index;
    if (line)
    	free(line);

    fclose(fp);
    return user_accounts;
}



int write_user_accounts(){
	FILE *fp;
	user_account_t *cur_user;
	size_t user_index = 0;



	fp = fopen(accounts_filename,"w");
	if (fp == NULL){
		error("couldn't open accounts file.");
		return -100;
	}


	
	while (user_index < num_user_accounts){
		cur_user = &user_accounts[user_index++];
		if (!strlen(cur_user->username)){
			continue;
		}
		fprintf(fp,"%s:%s:%s\n",cur_user->username,cur_user->salt,cur_user->password_hash);
	}

	fclose(fp);
	return 0;

}


int free_user_accounts(){
	for (size_t user_index = 0; user_index < num_user_accounts; user_index++){
		memset(&user_accounts[user_index],0,sizeof(user_account_t));
	}
	num_user_accounts = 0;
	return 0;	
}



//we created a temporary user slot earlier - read entire file and
//change that slot
//be sure to write newline after writing username and password
int create_user(char *username, char *password){
	user_account_t *cur_user;
	int salt;
	size_t user_index = 0;
	unsigned char salted_password[MAX_PASSWORD + (sizeof(unsigned int) * 2)];
	unsigned char hash[SHA_DIGEST_LENGTH];
	char hash_digest[SHA_DIGEST_LENGTH*2]; //2 hexadecimal characters to represent each byte


	if (!check_password(password)){
		return -61;
	}

	//generate salt for user
	salt = randint();

	snprintf((char*)salted_password,sizeof(salted_password),"%s%08x",password,salt);
	info("hashing salted_password: %s",salted_password);

	SHA1(salted_password, strlen((char*)salted_password), hash);

	for (int i = 0; i < SHA_DIGEST_LENGTH*2; i++){
		snprintf(hash_digest + i,2,"%02x",hash[i]);
	}


	if (get_user_accounts() == NULL){
		error("Couldn't get user accounts from file.");
		return -100;
	}

	while (user_index < num_user_accounts){
		cur_user = &user_accounts[user_index++];

		//we found our user, copy in the appropriate data into the
		//struct.
		if (!strcmp(cur_user->username,username)){
			snprintf(cur_user->salt,(sizeof(int)*2)+1,"%08x",salt);
			snprintf(cur_user->password_hash,sizeof(hash_digest)+1,"%s",hash_digest);
			break;
		}
	}

	write_user_accounts();
	free_user_accounts();

	return 0;


}




user_info_t* login_user(char *username, int connfd){
	user_info_t *new_user;

	new_user = calloc(1,sizeof(user_info_t));
	strcpy(new_user->username,username);

	new_user->connfd = connfd;

	new_user->next = user_infos;
	user_infos = new_user;

	return new_user;

}

int logout_user(char *username){
	user_info_t *user, *prev;

	user = user_infos;
	prev = NULL;

	while(user != NULL){
		if (!strcmp(username,user->username)){
			if (prev != NULL)
				prev->next = user->next;
			else{
				user_infos = user->next;
			}

			//close the connection if its not already closed.
			if (fcntl(user->connfd, F_GETFD) == EBADF){
				debug("User (%s) connection is already closed", username);
			}
			else {
				debug("Closing (%s) connection socket.", username);
				close(user->connfd);
			}

			free(user);
			return 0;
		}
		prev = user;
		user = user->next;
	}
	return -1;
}




user_info_t* user_logged_in(char *username){
	user_info_t *user;

	user = user_infos;
	while (user != NULL){
		if (!strcmp(username,user->username)){
			return user;
		}
		user = user->next;
	}
	
	return NULL;
}
