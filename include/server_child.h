#ifndef _H_SERVER
#define _H_SERVER

#include <netinet/in.h>

void start_client(int id, int listenfd, struct sockaddr_in cliaddr, int *p2c_pipe, int *c2p_pipe);
void run_command(char* command, int clientfd, int *p2c_pipe, int *c2p_pipe);

// Commands
void print_help(int fd);



#endif
