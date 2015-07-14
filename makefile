CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -D_POSIX_C_SOURCE=200112L
INCLUDES=-I./include
CPATH=./common

all: server_exec

server_exec: server/server_parent.c server/server_child.c $(CPATH)/utility.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

clean:
	rm -f server_exec

