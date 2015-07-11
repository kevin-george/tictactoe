CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -D_POSIX_C_SOURCE=200112L
INCLUDES=-I./ -I./Common
CPATH=./Common

all: server

server: server.c $(CPATH)/utility.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

clean:
	rm -f server

