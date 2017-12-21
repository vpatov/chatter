#include "chatter.h"






int server_port;
int num_chat_rooms;
FILE *output;

pool_t *threadpool;

void spawn_login_thread();
void *echo_thread_func(void *arg);





void *check_user_connections_func(void *arg){
	user_info_t *user;
	int err;
	socklen_t err_size = sizeof(err);

// getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);

	while(true){
		lock_user_info(1);
		user = user_infos;
		while (user != NULL){

			getsockopt(user->connfd, SOL_SOCKET, SO_ERROR, &err, &err_size);
			debug("check_user_connections thread. %s socket state: %d %s", user->username, err, strerror(err));

			user = user->next;
		}
		unlock_user_info(1);
		sleep(2);
	}
}


void
accept_connections()
{
	int listenfd, *connfd;
	int err, flag;
	struct sockaddr_in listen_sa, *conn_sa;
	socklen_t listen_addrlen, conn_addrlen;
	char client_ip[INET_ADDRSTRLEN];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if ((err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) < 0){
		error("setsockopt failed: %s",strerror(err));
	}

	listen_addrlen = sizeof(listen_sa);
	bzero(&listen_sa,sizeof(listen_sa));
	listen_sa.sin_family = AF_INET;
	listen_sa.sin_port = htons(server_port);
	listen_sa.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(listenfd,(SA *)&listen_sa,sizeof(listen_sa));

	listen(listenfd,50);

	// block on accept() - that's okay
	while(true){
		memset(client_ip,0,INET_ADDRSTRLEN);
		conn_sa = calloc(1,sizeof(*conn_sa));
		conn_addrlen = sizeof(conn_sa);

		//despite the fact that the connection socket descriptor is just 
		//an int,and doesn't absolutely need to be dynamically allocated,
		//I am doing so for the sake of consistency. The alternative is 
		//to cast to it to a void pointer which seems hacky.
		connfd = malloc(sizeof(int));


		*connfd = accept(listenfd,(SA *)conn_sa,&conn_addrlen);

		

		inet4_ntop(client_ip,conn_sa->sin_addr.s_addr);

		flag = 1;
        err = setsockopt(*connfd,IPPROTO_TCP,TCP_NODELAY,&flag,sizeof(int)); 
        if (err < 0){
        	errorw("setsockopt returned an error: %d %s",err, strerror(err));
        }
		infow("Accepted a client connection: %s", client_ip);
		pool_queue(threadpool,login_thread_func,connfd);
	}
}


/*
	Initialize server's data structures, and start the echo thread.
*/

void 
server_init()
{
	output = stdout;
	init_user_mutexes();
	init_room_mutexes();
	pthread_attr_init(&echo_thread_attr);

	threadpool = pool_create(2,4,500,NULL);

	// pool_queue(threadpool,check_user_connections_func,NULL);


    infow("Starting server. Currently listening on port: %d", server_port);
}

/* 
	Argument parsing was based on this SO post:
	https://stackoverflow.com/a/24479532/3664123
*/

void parse_args(int argc, char** argv){
	int opt;
	char *parse_motd;

    while ((opt = getopt(argc, argv, "ehn")) != -1) {
        switch (opt) {
        case 'h': fprintf(stderr,"%s",server_help_message); exit(EXIT_FAILURE);
        case 'e': echo_mode = true; break;
        case 'n': num_chat_rooms = 5; break;
        default:
        	fprintf(stderr,"%s",server_help_message);
        	exit(EXIT_FAILURE);
        }
    }

    if (num_chat_rooms && optind + 3 != argc){
    	fprintf(stderr,"%s",server_help_message);
    	exit(EXIT_FAILURE);
    }

    else if (!num_chat_rooms && optind + 2 != argc){
    	fprintf(stderr,"%s",server_help_message);
    	exit(EXIT_FAILURE);
    }

    if (num_chat_rooms){
    	num_chat_rooms = atoi(argv[optind++]);
    }

    server_port = atoi(argv[optind++]);
    parse_motd = argv[optind++];

    int j = 0;
    for (int i = 0; i < strlen(parse_motd); i++){
    	if (parse_motd[i] == '\\' && parse_motd[i+1] == 'n'){
    		motd[j++] = '\n';
    		i++;
    	}
    	else {
    		motd[j++] = parse_motd[i];
    	}
    }

    if (server_port == 0){
    	fprintf(stderr,"%s",server_help_message);
    	exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv){
	parse_args(argc,argv);
    server_init();
    accept_connections();
}
