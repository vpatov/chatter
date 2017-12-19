#include "chatter.h"

const char* server_help_message = 	"Usage:\n./server [-he] [-N num] PORT_NUMBER MOTD\n\n"
									"-e\t\t\tEcho messages received on server's stdout.\n"
									"-h\t\t\tDisplays this help menu.\n"
									"-N num\t\t\tSpecifies maximum number of chat rooms allowed on server. (Default = 5)\n"
									"PORT_NUMBER\t\tPort number to listen on. (Must be non-zero)\n"
									"MOTD\t\t\tMessage to display to the client when they connect.\n";


const char* client_help_message = 	"Usage:\n./client [-hc] NAME SERVER_IP SERVER_PORT\n\n"
									"-c\t\t\tRequests to server to create a new user NAME\n"
									"-h\t\t\tDisplays this help menu.\n"
									"NAME\t\t\tUsername to use when logging in. "
									"If -c option is set, will attempt to create new user with specified name.\n"
									"SERVER_IP\t\t\tIP address of server to connect to.\n"
									"SERVER_PORT\t\tPort number to listen on. (Must be non-zero)\n";


const char* password_rules =		"Please enter a password. (Must be at least 5 characters, "
									"have at least one uppcase letter, one number, and one symbol.\n"
									"Acceptable passwords follow regex: [!@#$0-9a-zA-Z]+\n";

const char *accounts_filename = "accounts";

const char *error_messages[] = {
	"SORRY %s", "USER %s EXISTS", "%s DOES NOT EXIST", "ROOM EXISTS", "MAXIMUM ROOMS REACHED", "ROOM DOES NOT EXIST",
	"USER NOT PRESENT", "NOT OWNER", "INVALID USER", "INVALID OPERATION", "INVALID PASSWORD", "INVALID FORMAT/NO CARRIAGE RETURN","INTERNAL SERVER ERROR"
};

const int error_codes[] = {0, 1, 2, 10, 11, 20, 30, 40, 41, 60, 61, 62, 100};

const char *space = " ";
const char *sprn = " \r\n";
const char *rn = "\r\n";

const char *verbs[] = {
	"ALOHA!", "!AHOLA", "IAM", "IAMNEW","HI", "HINEW", "AUTH", "PASS", "NEWPASS", "ERR", "BYE"
};

// enum verbs{ALOHA_VERB, AHOLA_VERB, IAM_VERB, IAMNEW_VERB, NEWPASS_VERB, ERR_VERB};


int send_data(int connfd, int verb, char *data){
	char sendbuff[MAX_SEND];

	if (data != NULL){
		snprintf(sendbuff,MAX_SEND,"%s %s \r\n",verbs[verb], data);
	}
	else {
		snprintf(sendbuff,MAX_SEND,"%s \r\n",verbs[verb]);
	}

	return send(connfd,sendbuff,strlen(sendbuff),0);
}

int recv_data(int connfd, char *recvbuff){
	return recv(connfd,recvbuff,MAX_RECV,0);
}


//if data is NULL, then it is not expecting data after the verb.
//if data is not NULL, and we receive data after the verb, data 
//points to a buffer to which we copy the data after the verb.
//Accepts a variable length list of verbs. Upon finding one of the verbs,
//it returns the enum index of the verb and populates data with the data.
//returns -1 on error.
int expect_data(char *recvbuff, char *request_data, char **errcode,int num_verbs, ...){

	int index, verb_enum;
	const char *verb;
	char *saveptr = NULL;
	char *tokenptr;
	// const char *delimiter;
	char *recv_verb;


	//We must use the reentrant version of strtok, strtok_r.
	//If we are expecting more data after the verb, then delimit
	//the verb via a space. If we are not expecting more data, then
	//delimit it via a carriage return "\r\n".
	// delimiter = (request_data != NULL) ? space : rn;
	recv_verb = strtok_r(recvbuff, space, &saveptr);
	info("recv_verb:%s\n",recv_verb);


	va_list arguments;
	va_start (arguments, num_verbs); 
	for (index = 0; index < num_verbs; index++){
		verb_enum = va_arg(arguments,int);
		verb = verbs[verb_enum];

		//if the received buffer is equivalent to one of our accepted verbs
		if (!strncmp(recv_verb,verb,strlen(recv_verb))) {
			break;
		}
    }
    va_end (arguments);

    //this means we didn't find any of the verbs we expected.
    if (index == num_verbs){
    	return -1;
    }

    //if we found a verb we expected, point data to the rest of the message
    // TODO TODO LEFTOFF
    if (request_data != NULL){
    	tokenptr = strtok_r(NULL,sprn, &saveptr);
    	if (tokenptr == NULL){
 			return ERR62;
    	}
    	strcpy(request_data,tokenptr);

    }

    return verb_enum;


}





void send_error(int connfd, int error, char *message, bool close_connection){
	char sendbuff[MAX_SEND];
	char formatted_message[128];
	if (message == NULL){
		snprintf(sendbuff,MAX_SEND,"%s %d %s %s", verbs[ERR], error_codes[error],error_messages[error],rn);
	}
	else {
		snprintf(formatted_message,128,error_messages[error],message);
		snprintf(sendbuff,MAX_SEND,"%s %d %s %s", verbs[ERR], error_codes[error],formatted_message,rn);
	}

	send(connfd,sendbuff,strlen(sendbuff),0);
	if (close_connection)
		close(connfd);	
}


//taken from https://stackoverflow.com/a/7775170/3664123
void strip_char(char *str, char strip)
{
    char *p, *q;
    for (q = p = str; *p; p++)
        if (*p != strip)
            *q++ = *p;
    *q = '\0';
}


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
        	return true;
        }

    }

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
	return length >= 5;
}


unsigned int randint(){
	unsigned int rand;
	if (!RAND_bytes((unsigned char*)&rand, sizeof(unsigned int))){
		error("RAND_bytes returned an error.");
	}
	return rand;
}



//assumed that user doesn't exist
//be sure to write newline after writing username and password
int create_user(char *username, char *password){
	FILE *fp;
	int salt;
	unsigned char salted_user[MAX_USERNAME + sizeof(unsigned int)];
	unsigned char hash[SHA_DIGEST_LENGTH];
	char hash_digest[SHA_DIGEST_LENGTH*2]; //2 hexadecimal characters to represent each byte


	if (!check_password(password)){
		return ERR61;
	}

	fp = fopen(accounts_filename,"a");
	if (fp == NULL){
		return ERR100;
	}

	//generate salt for user
	salt = randint();

	snprintf((char*)salted_user,sizeof(salted_user),"%s%d",username,salt);
	SHA1(salted_user, strlen((char*)salted_user), hash);

	for (int i = 0; i < SHA_DIGEST_LENGTH; i++){
		snprintf(hash_digest + i,2,"%02x",hash[i]);
	}



	fprintf(fp,"%s:%u:%s\n",username,salt,hash_digest);
	fclose(fp);

	return 0;


}
