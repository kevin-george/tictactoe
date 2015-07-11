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
    printf("Usage: ./server <port number>.\n");
    exit(EXIT_FAILURE);
  }

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror(": Can't get socket");
    exit(EXIT_FAILURE);
  }

  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_family = AF_INET;
  addr.sin_port = htons((short)atoi(argv[1]));

  if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror(": bind");
    exit(EXIT_FAILURE);
  }

  unsigned int len = sizeof(addr);
  if(getsockname(sockfd, (struct sockaddr *)&addr, &len) < 0) {
    perror(": can't get name");
    exit(EXIT_FAILURE);
  }

  printf("\nListening on port %s...\n", argv[1]);

  if(listen(sockfd, 5) < 0) {
    perror(": bind");
    exit(EXIT_FAILURE);
  }

  for(int i=0; i<20; i++) { 
    child_id++;
    if(fork() == 0) {
      while(1) {
        while((rec_sock = accept(sockfd, (struct sockaddr *)(&recaddr), &len)) < 0) {
          if(errno == EINTR) {
            printf("errno = %d\n", errno);
            continue; 
          } else {
            perror(":accept error");
            exit(EXIT_FAILURE);
          }
        }

        if(rec_sock < 0) {
          perror(": accept");
          exit(EXIT_FAILURE);
        }

        printf("Client connected to child %d <machine = %s, port = %x, %x.>\n", child_id, inet_ntoa(recaddr.sin_addr), recaddr.sin_port, ntohs(recaddr.sin_port));

        int num;
        while((num = read(rec_sock, buf, 100))) {
          if(num == -1)
            continue;
          write(rec_sock, buf, num);
        }
        close(rec_sock);  
      }
    }
  }
  while(1) { 
    int i; 
    wait(&i);
  }
}
