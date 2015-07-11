CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -D_POSIX_C_SOURCE=200112L

all:server
server:server.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f server
