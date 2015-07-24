CC=gcc
CFLAGS=-g -Wall -std=c99 -pedantic -lpthread
INCLUDES=-I./include

all: server_exec

server_exec: server/server.c server/client.c login/login.c common/utility.c messaging/message.c gameplay/game.c common/microtime.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

clean:
	rm -f server_exec

