#include "chatter.h"



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
		error("send_data encountered an error: %d %s", ret, strerror(ret));
	}
	return ret;
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
// int expect_data(char *recvbuff, char *request_data, int num_verbs, ...){

// 	int index, verb_enum, error_code;
// 	const char *verb;
// 	char *saveptr = NULL;
// 	char *tokenptr;
// 	// const char *delimiter;
// 	char *recv_verb;


// 	//We must use the reentrant version of strtok, strtok_r.
// 	//If we are expecting more data after the verb, then delimit
// 	//the verb via a space. If we are not expecting more data, then
// 	//delimit it via a carriage return "\r\n".
// 	// delimiter = (request_data != NULL) ? space : rn;
// 	recv_verb = strtok_r(recvbuff, space, &saveptr);
// 	debug("recv_verb:%s\n",recv_verb);

// 	if (!strcmp(recv_verb,verbs[ERR])){
// 		//take the rest of the message except for the carriage return
// 		tokenptr = strtok_r(NULL,sprn, &saveptr);

// 		//if there is no carriage return, message is improperly formatted
//     	if (tokenptr == NULL){
//  			return -62;
//     	}

//     	//if the pointer passed is NULL, user doesnt want the message
//     	if (request_data != NULL){
//     		strcpy(request_data,tokenptr);
//     		debug("request data in expect_data: --%s--",request_data);
//     	}

//     	//get the error code
//     	tokenptr = strtok_r(NULL,space,&saveptr);
//     	if (tokenptr != NULL){
//     		error_code = strtol(tokenptr,NULL,10);
//     		if (errno != 0){
// 	    		debug("error_code in expect_data: --%d--",error_code);
// 	    		return error_code;
//     		}
//     	}

//     	return -62;
// 	}


// 	va_list arguments;
// 	va_start (arguments, num_verbs); 
// 	for (index = 0; index < num_verbs; index++){
// 		verb_enum = va_arg(arguments,int);
// 		verb = verbs[verb_enum];

// 		//if the received buffer is equivalent to one of our accepted verbs
// 		if (!strncmp(recv_verb,verb,strlen(recv_verb))) {
// 			break;
// 		}
//     }
//     va_end (arguments);


//     //if we want the rest of the message (correct verb or error regardless)
//     //copy it into the request_data
//     // TODO TODO LEFTOFF
//     if (request_data != NULL){
//     	tokenptr = strtok_r(NULL,sprn, &saveptr);
//     	if (tokenptr == NULL){
//  			return -62;
//     	}
//     	strcpy(request_data,tokenptr);
//     }

//     //if we didn't find the verb, return -1.
//     return index == num_verbs ? -1 : verb_enum;
// }



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
	const char *verb;
	char *saveptr1, *saveptr2;
	char *recv_verb;
	char *error_num;



	//get verb
	recv_verb = strtok_r(recvbuff, space, &saveptr1);
	debug("expect_data recv_verb:%s\n",recv_verb);

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
    		debug("expect_data found error code: %d", *error_code);
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

void handle_error(char *recvbuff, char *message_data, int error_code){
	if (error_code == 62){
		error("Received improperly formatted message: --%s--",recvbuff);
	}
	else {
		if (message_data != NULL){
			error("Received message: %s %s", verbs[ERR] ,message_data);
		}
		else {
			error("Received unexpected message...");
		}
	}
}




void send_error(int connfd, int error, char *message, bool close_connection){
	char sendbuff[MAX_SEND];
	char formatted_message[128];

	if (error < 0)
		error = -error;
	if (message == NULL){
		snprintf(sendbuff,MAX_SEND,"%s %d %s %s", verbs[ERR], error,get_error(error),rn);
	}
	else {
		snprintf(formatted_message,128,get_error(error),message);
		snprintf(sendbuff,MAX_SEND,"%s %d %s %s", verbs[ERR], error,formatted_message,rn);
	}

	send(connfd,sendbuff,strlen(sendbuff),0);
	if (close_connection)
		close(connfd);	
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


int tokenize(char *string, const char *delim){
    char *ptr;
    int i = 0;
    int tokens = 0;
    int length = strlen(delim);

    ptr = string;
    do {
        ptr = strstr(ptr,delim);
        if (ptr != NULL){
            //set the delimeter bytes to zero.
            memset(ptr,0,length);
            ptr += length;
            tokens++;
        }
    
    } while(ptr != NULL);

    return tokens;
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
    else
        return NULL;


}