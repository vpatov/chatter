all: server client threadpool

client:
	gcc -g -Wall client.c -o client
server:
	gcc -g -Wall server.c -o server

threadpool:
	gcc -pthread -g -Wall threadpool.c -o threadpool -lm

clean:
	rm server client threadpool
