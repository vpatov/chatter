
SRC = $(wildcard *.c)
OBJ = threadpool.o chatter.o auth.o utils.o vars.o errors.o locks.o rooms.o login.o echo.o


CXX = gcc
CXXFLAGS = -Wall -g
LDFLAGS = -pthread
LIBS = -lm -lssl -lcrypto

all: server client

server: server.o $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o server server.o $(OBJ)  $(LIBS)

client: client.o $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o client client.o $(OBJ)  $(LIBS)

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm *.o client server 


