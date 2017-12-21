#include "chatter.h"
char print_buff[MAX_MSG_SIZE];



void encode_buff(char *buff, int n){
	int pi, ri;

	pi = ri = 0;

	while (buff[ri] && ri < n){
		if (buff[ri] == '\r'){
			print_buff[pi++] = '\\';
			print_buff[pi++] = 'r';
		}
		else if (buff[ri] == '\n'){
			print_buff[pi++] = '\\';
			print_buff[pi++] = 'n';
		}
		else {
			print_buff[pi++] = buff[ri];
		}

		ri++;
	}
	print_buff[pi] = '\0';

}

void echo_recv(char *recvbuff, int n){
	encode_buff(recvbuff, n);
	recvecho("\"%s\"", print_buff);
}

void echo_send(char *sendbuff, int n){
	encode_buff(sendbuff, n);
	sendecho("\"%s\"", print_buff);
}

int send_data(int connfd, int verb, char *data){
	char sendbuff[MAX_SEND];
	int ret;
	if (data != NULL){
		snprintf(sendbuff,MAX_SEND,"%s %s \r\n",verbs[verb], data);
	}
	else {
		snprintf(sendbuff,MAX_SEND,"%s \r\n",verbs[verb]);
	}


	ret = send(connfd,sendbuff,strlen(sendbuff),0);
	if (ret < 0){
		error("send_data: %s", strerror(errno));
	}

	if (ret == 0){
		error("send_data wrote 0 bytes.");
	}

	else {
		if (echo_mode){
			echo_send(sendbuff,ret);
		}
	}
	return ret;
}

int send_data_custom(int connfd, char *data){
	int ret;

	ret = send(connfd,data,strlen(data),0);
	if (ret < 0){
		error("send_data_custom: %s", strerror(errno));
	}

	if (ret == 0){
		error("send_data_custom wrote 0 bytes.");
	}

	else {
		if (echo_mode){
			echo_send(data,ret);
		}
	}
	return ret;
}


int recv_data(int connfd, char *recvbuff){
	int ret;

	ret = recv(connfd,recvbuff,MAX_RECV,0);

	if (ret > 0 && echo_mode){
		echo_recv(recvbuff,ret);
	}

	//debug("RECV_DATA BUFF: %s",recvbuff);

	return ret;

}


/*
	Expects one message. No carriage returns.

	char **recvptr - 	a pointer to a pointer that points to the recvbuff used to recv into.
					 	This pointer gets modified by strtok to point to different parts of the
					 	recv buffer as tokens gets parsed

	char *message_data	a pointer to a buffer to copy the message data into. If NULL, don't copy.

	int *error_code		a pointer to an integer, which is set to the error code if an error is found
						should only be interpreted if value returned is < 0, or if ERR verb is returned

	int num_verbs		the amount of different verbs we're interested in/accepting

	...					variable length list of the verb enums we're interested in

*/
//returns -1 on unexpected errors, returns verb otherwise. 
int expect_data(char *recvbuff, char *message_data, int *error_code, int num_verbs, ...){
	int index, verb_enum;
	char store_buff[MAX_MSG_SIZE];
	const char *verb;
	char *saveptr1, *saveptr2;
	char *recv_verb;
	char *error_num;
	char *endptr;

	endptr = NULL;
	strcpy(store_buff,recvbuff);

	//get verb
	recv_verb = strtok_r(store_buff, space, &saveptr1);
	// debug("expect_data recv_verb:%s\n",recv_verb);

	if (recv_verb == NULL){
		*error_code = 100;
		return -1;
	}

	//if the verb is ERR
	if (!strcmp(recv_verb,verbs[ERR])){

		//GET ERROR CODE
		error_num = strtok_r(saveptr1, space, &saveptr2);
		if (error_num != NULL){
			*error_code = strtol(error_num,NULL,10);
    		if (errno != 0){
	    		debug("error_code in strtol in expect_data: %d %s",errno, strerror(errno));
	    		return -1;
    		}
    		// debug("expect_data found error code: %d", *error_code);
		}
		//if we couldn't find error code
		else {
			*error_code = 62;
			return -1;
		}

		//if user interested in message_data, copy what's in the rest of the token into message_data
		if (message_data != NULL)
			strcpy(message_data,saveptr2);

		return -ERR;
	}



	//if the verb is not ERR, see if its one of the ones we want.
	va_list arguments;
	va_start (arguments, num_verbs); 
	for (index = 0; index < num_verbs; index++){
		verb_enum = va_arg(arguments,int);
		verb = verbs[verb_enum];

		//if the verb we got is one of the ones we want
		if (!strncmp(recv_verb,verb,strlen(recv_verb))) {
			break;
		}
    }
    va_end (arguments);

    //if interested in message, copy it.
    if (message_data != NULL){
		strcpy(message_data,saveptr1);
    }

    return index == num_verbs ? -1 : verb_enum;

}

// void print_messagE(int verb, char *message_data)

void print_error(char *recvbuff, char *message_data, int error_code){
	if (error_code == 62){
		error("Received improperly formatted message: --%s--",recvbuff);
	}
	else {
		if (message_data != NULL){
			errorp("%s",message_data);
		}
		else {
			error("Received unexpected message...");
			assert(false);
		}
	}
}




void send_error(int connfd, int error, char *message, bool close_connection){
	char sendbuff[MAX_SEND];
	char formatted_message[128];
	int ret;

	if (error < 0)
		error = -error;
	if (message == NULL){
		snprintf(sendbuff,MAX_SEND,"%s %d %s %s", verbs[ERR], error,get_error(error),sprn);
	}
	else {
		snprintf(formatted_message,128,get_error(error),message);
		snprintf(sendbuff,MAX_SEND,"%s %d %s %s", verbs[ERR], error,formatted_message,sprn);
	}

	
	ret = send(connfd,sendbuff,strlen(sendbuff),0);

	if (echo_mode){
		echo_send(sendbuff,ret);
	}
	if (close_connection){
		send_data(connfd,BYE,NULL);
		close(connfd);	
	}
}




char *my_strtok(char *string, const char *delim, char **saveptr){
	char *ptr;
	int i = 0;

	ptr = strstr(string,delim);
	if (ptr != NULL){
		for (i = 0; i < sizeof(delim) / sizeof(char); i++){
			*(ptr + i) = 0;
		}
	}
	else {
		return NULL;
	}

	*saveptr = (ptr + i);
	return ptr;
}


char *get_token(char *string, const char *delim, int count){
    int length = strlen(delim);
    char *ptr = string;


    for (int i = 0; i < count; i++){
        while(*ptr){
            ptr++;
        }
        ptr+=length;
    }
    if (*ptr)
        return ptr;
    else{
    	debug("get_token is returning NULL");
        return NULL;
    }


}

int tokenize(char *string, const char *delim){
    char *ptr, *save;
    int tokens = 0;
    int length = strlen(delim);

    ptr = save = string;
    do {
        ptr = strstr(ptr,delim);
        if (ptr != NULL){
            //set the delimeter bytes to zero.
            memset(ptr,0,length);
            ptr += length;
            save = ptr;
            tokens++;
        }
        else if (*save)
        	tokens++;

    
    } while(ptr != NULL);

    return tokens;
}