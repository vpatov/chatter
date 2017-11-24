all: server client threadpool

client:
	gcc -g -Wall -Werror client.c -o client
server:
	gcc -g -Wall -Werror server.c -o server

threadpool:
	gcc -pthread -g -Wall -Werror threadpool.c -o threadpool

clean:
	rm server client threadpool
