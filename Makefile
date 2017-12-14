all: server client threadpool concurrent.o

client: client.c
	gcc -g -Wall client.c -o client
server: server.c
	gcc -g -Wall server.c -o server

concurrent.o: concurrent.c
	gcc -pthread -g -Wall -c concurrent.c 

threadpool: threadpool.c concurrent.o
	gcc -pthread -g -Wall threadpool.c -o threadpool concurrent.o -lm

clean:
	rm server client threadpool concurrent.o
