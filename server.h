#ifndef _H_SERVER
#define _H_SERVER

#include <netinet/in.h>

void sig_chld(int signo);
void help(int fd);
void start_client(int id, int listenfd, struct sockaddr_in cliaddr, int *p2c_pipe, int *c2p_pipe);




#endif
