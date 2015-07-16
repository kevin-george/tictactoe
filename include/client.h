#ifndef _H_CLIENT
#define _H_CLIENT

#include <netinet/in.h>

#define USERID_LENGTH 50
#define PASSWORD_LENGTH 50

typedef struct client_t{
	int cli_sock;
	int tid;
	char user_id[USERID_LENGTH];
} client_t;

// pThread
void *start_client(void *arg);


// void start_client(int id, int listenfd, struct sockaddr_in cliaddr, int *p2c_pipe, int *c2p_pipe);
void run_command(int tid, char *cmd);

// Commands
void print_help(int fd);
void register_cmd(int fd, char *cmd);

// Cleanup
void close_client(int tid);


#endif