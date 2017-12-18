
client: client.o threadpool.o
	gcc -pthread -g -Wall client.c -o client client.o threadpool.o -lm
server: server.o threadpool.o
	gcc -pthread -g -Wall server.c -o server sever.o threadpool.o -lm

# concurrent.o: concurrent.c
# 	gcc -pthread -g -Wall -c concurrent.c 

client.o: client.c
	gcc -c client.c

server.o: server.c
	gcc -c server.c

threadpool.o: threadpool.c
	gcc -pthread -g -Wall -c threadpool.c -lm

# threadpool.o: threadpool.c 
# 	gcc -pthread -g -Wall -c threadpool.c -lm 

clean:
	rm server client threadpool.o concurrent.o 
