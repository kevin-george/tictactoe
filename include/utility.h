#ifndef _UTILITY_H
#define _UTILITY_H

#include <stdlib.h>
#include <stddef.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>

// MISC
void my_error(char *err);
void my_log(char *log);

// IPC 
int my_read(int fd, void *buff,size_t nbytes);
int my_write(int fd,const void *buff,size_t nbytes);

// Sockets
int my_socket(int domain, int type, int protocol);
int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int my_listen(int socket, int backlog);
int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int my_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

// I/O Multiplex
int my_select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);

// File
int my_close(int fd);

// String
int starts_with(char* target, const char* search);

#endif
