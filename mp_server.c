#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "utility.h"

#define CLIENT_SIZE 20
int child_id = 0;


void sig_chld(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) 
        printf("child %d terminated.\n", (int)pid);
}

int main(int argc, char * argv[]) {
    // maxi is highest index in client array that is in use
    // maxfd is the current value of the first argument to select
    int maxi, maxfd, listenfd, connfd, sockfd;
    int nready, client[CLIENT_SIZE];
    int i, n;
    char buf[100];
    struct sockaddr_in servaddr, cliaddr;
    struct sigaction abc;
    fd_set rset, allset;
    socklen_t clilen;

    abc.sa_handler = sig_chld;
    sigemptyset(&abc.sa_mask);
    abc.sa_flags = 0;

    sigaction(SIGCHLD, &abc, NULL);

    if(argc < 2) {
        printf("Usage: ./server <port number>.\n");
        exit(1);
    }

    listenfd = my_socket(AF_INET, SOCK_STREAM, 0);

    memset(&servaddr, 0 , sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((short)atoi(argv[1]));

    my_bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    printf("\nListening on port %s...\n", argv[1]);
    my_listen(listenfd, 5);

    maxfd = listenfd;   // initialize
    maxi = -1;          // index into client[] array 
    for (i = 0; i < CLIENT_SIZE; ++i)
        client[i] = -1;

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    for ( ; ;) {
        rset = allset;
        nready = my_select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &rset)) {  // new client connection
            clilen = sizeof(cliaddr);
            connfd = my_accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
            printf("client accepted: %d\n", connfd);

            for (i = 0; i < CLIENT_SIZE+3; ++i)   // stdin,out,err + client size
                if (client[i] < 0) {
                    client[i] = connfd; // save descriptor
                    break;
                }
            if (i == CLIENT_SIZE+3)
                my_error("too many clients");
            FD_SET(connfd, &allset);    // add new descriptor to set
            if (connfd > maxfd)
                maxfd = connfd; // for select
            if (i > maxi)
                maxi = i;   // max index in client[] array
            if (--nready <= 0)
                continue;   // no more readable descriptors
        }
        for (i = 0; i <= maxi; ++i) {
            if ((sockfd = client[i]) < 0)
                continue;
            if (FD_ISSET(sockfd, &rset)) {
                if ((n = my_read(sockfd, buf, 100)) == 0) {
                    // connection closed by client 
                    my_close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                } else
                    my_write(sockfd, buf, n);

                if (--nready <= 0)
                    break;      // no more readable descriptors
            }
        }
    }
 }
