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




const char *VERBS[] = {
	"ALOHA!", "!AHOLA", "IAM", "IAMNEW", "NEWPASS", "ERR"
};

// enum verbs{ALOHA_VERB, AHOLA_VERB, IAM_VERB, IAMNEW_VERB, NEWPASS_VERB, ERR_VERB};


int send_data(int connfd, int verb, char *data){
	char sendbuff[MAX_SEND];

	if (data != NULL){
		snprintf(sendbuff,MAX_SEND,"%s %s\r\n",VERBS[verb], data);
	}
	else {
		snprintf(sendbuff,MAX_SEND,"%s\r\n",VERBS[verb]);
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
int expect_data(char *recvbuff, char *databuff, int num_verbs, ...){

	int index;
	const char *verb;
	char *saveptr = NULL;
	char *recv_verb;

	

	//We must use the reentrant version of strtok, strtok_r.
	//get the first space-delimited token from the received data,
	//which should be equivalent to some verb.
	recv_verb = strtok_r(recvbuff, " ", &saveptr);
	printf("recv_verb:%s\n",recv_verb);


	va_list arguments;
	va_start (arguments, num_verbs); 
	for (index = 0; index < num_verbs; index++){
		verb = VERBS[va_arg(arguments,int)];

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

    //if we found a verb we expected, point data to the rest of the message, and strip the '\r\n'.
    // TODO TODO LEFTOFF

	if (databuff == NULL){
		// int strncmp ( const char * str1, const char * str2, size_t num );
		char s[256];
		strcpy(s, "one two three");
		char* token = strtok(s, " ");
		while (token) {
		    printf("token: %s\n", token);
		    token = strtok(NULL, " ");
		}
	}
}



	// snprintf(expect,MAX_RECV,"%s%s",aloha_verb,rn);
	// if (strcmp(recvbuff,expect)){

	// 	snprintf(sendbuff,MAX_SEND,"%s: Expected \"%s\" to be first message upon connection.%s", err_verb, aloha_verb,rn);
	// 	send(connfd,sendbuff,strlen(sendbuff),0);
	// 	close(connfd);
	// 	return NULL;
	// }


