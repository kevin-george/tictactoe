#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "utility.h"


/*-----------------------------------------------------------*/
/*                          MISC                             */
/*-----------------------------------------------------------*/

void my_error(char *msg) {
  if (errno) 
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  else 
    fprintf(stderr, "%s\n", msg);

  // Track errors
  my_log(msg);

  exit(EXIT_FAILURE);
}

void my_log(char *msg) {
  time_t c_time = time(NULL);        // Timestamp
  FILE *log_file = fopen("log.txt", "a+");

  // Avoid recursion
  if (!log_file) {
    perror("Error: Opening log.txt");
    exit(EXIT_FAILURE);
  }

  // Timestamp: Error Message
  fprintf(log_file, "%s - %s\n", asctime(localtime(&c_time)), msg); 

  fclose(log_file);
}
/*-----------------------------------------------------------*/
/*                            IPC                            */
/*-----------------------------------------------------------*/


int my_read(int fd, void *vptr, size_t n) {
  size_t nleft;
  int nread;
  char *ptr;

  ptr = vptr;
  nleft = n;
  while (nleft > 0) {
    if ((nread = read(fd, ptr, nleft)) < 0) {
      if (errno == EINTR){
        printf("EINTR\n");
        nread = 0;   // and call read() again
      } else
        return -1;
    } else if (nread == 0)
      break;           // EOF

    nleft -= nread;
    ptr += nread;
    if (*(ptr-2) == '\r') {  // Read appends \r\n 
      *(ptr-2) = '\0';
      break;
    }
  }
  /*char *ptr2 = vptr;
  for (int i = 0; i < strlen(ptr2); ++i)
    printf("%i:%c\n", i, ptr2[i]);
  printf("size:%lu\n", strlen(ptr2));
  */ 
  return (n - nleft);
}

int my_write(int fd, const void *vptr, size_t n) {
  size_t nleft;
  int nwritten;
  const char *ptr;


  ptr = vptr;
  nleft = n;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0 && errno == EINTR)
        nwritten = 0;   // and call write() again
      else
        return -1;      // error
    }

    nleft -= nwritten;
    ptr += nwritten;
  }
/*
  char *ptr2 = vptr;
  for (int i = 0; i < strlen(ptr2); ++i)
    printf("%i:%c\n", i, ptr2[i]);
  printf("size:%lu\n", strlen(ptr2));
*/
  return n;
}


/*-----------------------------------------------------------*/
/*                          SOCKETS                          */
/*-----------------------------------------------------------*/

int my_socket(int domain, int type, int protocol) {
  int fd;
  if ((fd = socket(domain, type, protocol)) == -1)
    my_error("Socket: Socket creation failed");

  return fd;
}

int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  int fd;
  while((fd = accept(sockfd, addr, addrlen)) == -1) {
    if (errno == EINTR) {
      printf("errno = %d\n", errno);
      continue;
    } else {
      my_error("Socket Accept failed");
    }
  }

  return fd;
}

int my_listen(int socket, int backlog) {
  if (listen(socket, backlog) == -1)
    my_error("Socket: Listen failed");

  return 0;
}

int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  if (bind(sockfd, addr, addrlen) == -1)
    my_error("Socket: Bind failed");

  return 0;
}

int my_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  if (getsockname(sockfd, addr, addrlen) == -1)
    my_error("Socket: Getsockname failed");

  return 0;
}


/*-----------------------------------------------------------*/
/*                       I/O Multiplex                       */
/*-----------------------------------------------------------*/

int my_select(int nfds, fd_set *rfds, fd_set *wfds,
    fd_set *efds, struct timeval *to) {
  int num_fd;
  if ((num_fd = select(nfds, rfds, wfds, efds, to)) == -1)
    my_error("Select failed");

  return num_fd;
}

int my_close(int fd) {
  if (close(fd) == -1)
    my_error("close failed");
  return 0;
}

/*-----------------------------------------------------------*/
/*          String utility function - starts_with            */
/*-----------------------------------------------------------*/

int starts_with(char *target, const char *search) {
   if(strncmp(target, search, strlen(search)) == 0) 
     return 0;
   return -1;
}
