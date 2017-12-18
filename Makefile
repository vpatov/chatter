

CXX = gcc
CXXFLAGS = -Wall -g -pthread

# ****************************************************
# Targets needed to bring the executable up to date

all: server client

server: server.o threadpool.o 
	$(CXX) $(CXXFLAGS) -o server server.o threadpool.o -lm

server.o: server.c threadpool.h
	$(CXX) $(CXXFLAGS) -c server.c

client: client.o threadpool.o 
	$(CXX) $(CXXFLAGS) -o client client.o threadpool.o -lm

client.o: client.c threadpool.h
	$(CXX) $(CXXFLAGS) -c client.c

threadpool.o: threadpool.h








# all: client server

# client: 
# 	gcc -pthread -Wall -g client.c -o client.o threadpool.o -lm
# server: 
# 	gcc -Wall -g -pthread server.c -o server threadpool.o -lm


# # concurrent.o: concurrent.c
# # 	gcc -pthread -g -Wall -c concurrent.c 

# client.o: client.c
# 	gcc -c client.c

# server.o: server.c
# 	gcc -c server.c

# threadpool.o: threadpool.c
# 	gcc -c threadpool.c

# # threadpool.o: threadpool.c 
# # 	gcc -pthread -g -Wall -c threadpool.c -lm 

# clean:
# 	rm server client threadpool.o server.o client.o



