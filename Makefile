all: server client threadpool

client:
	gcc -Wall -Werror client.c -o client
server:
	gcc -Wall -Werror server.c -o server

threadpool:
	gcc -Wall -Werror threadpool.c -o threadpool

clean:
	rm server client
