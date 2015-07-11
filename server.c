#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "utility.h"

int child_id = 0;

void sig_chld(int signo)
{
  pid_t pid;
  int stat;
  while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) 
    printf("child %d terminated.\n", (int)pid);
  return ;
}

int main(int argc, char * argv[])
{
  int sockfd, rec_sock;
  struct sockaddr_in addr, recaddr;
  struct sigaction abc;
  char buf[100];

  abc.sa_handler = sig_chld;
  sigemptyset(&abc.sa_mask);
  abc.sa_flags = 0;

  sigaction(SIGCHLD, &abc, NULL);

  if(argc < 2) {
    my_log("Usage: ./server <port number>.\n");
  }

  sockfd = my_socket(AF_INET, SOCK_STREAM, 0);

  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_family = AF_INET;
  addr.sin_port = htons((short)atoi(argv[1]));

  my_bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

  unsigned int len = sizeof(addr);
  getsockname(sockfd, (struct sockaddr *)&addr, &len);

  printf("\nListening on port %s...\n", argv[1]);

  my_listen(sockfd, 5);

  for(int i=0; i<20; i++) { 
    child_id++;
    if(fork() == 0) {
      while(1) {
        rec_sock = my_accept(sockfd, (struct sockaddr *)(&recaddr), &len);
        /*
         * What's the purpose of this check?
        if(rec_sock < 0) {
          perror(": accept");
          exit(EXIT_FAILURE);
        }
        */
        printf("Client connected to child %d <machine = %s, port = %x, %x.>\n", child_id, inet_ntoa(recaddr.sin_addr), recaddr.sin_port, ntohs(recaddr.sin_port));

        int num = my_read(rec_sock, buf, 100);
        my_write(rec_sock, buf, num);
        /*
        while((num = read(rec_sock, buf, 100))) {
          if(num == -1)
            continue;
          write(rec_sock, buf, num);
        }
        */
        close(rec_sock);  
      }
    }
  }
  while(1) { 
    int i; 
    wait(&i);
  }
}
