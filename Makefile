all: server client threadpool

client: client.c
	gcc -g -Wall client.c -o client
server: server.c
	gcc -g -Wall server.c -o server

threadpool: threadpool.c
	gcc -pthread -g -Wall threadpool.c -o threadpool -lm

clean:
	rm server client threadpool
