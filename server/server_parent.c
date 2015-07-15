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

#include "server_child.h"
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
    int listenfd, maxr_fd, maxr_newcli_fd;
    int cpid;
    char buf[100];
    char **client_names;
    struct sockaddr_in addr, recaddr;
    struct sigaction abc;
    int p2c_fd[20][2], c2p_fd[20][2],  c2p_newcli_fd[20][2];
    fd_set rset_newcli, rset;
    struct timeval tv;

    tv.tv_sec = 0;
    FD_ZERO(&rset);
    FD_ZERO(&rset_newcli);

    abc.sa_handler = sig_chld;
    sigemptyset(&abc.sa_mask);
    abc.sa_flags = 0;

    sigaction(SIGCHLD, &abc, NULL);

    if(argc < 2) {
        my_log("Usage: ./server <port number>.\n");
    }

    client_names = (char**)malloc(sizeof(char*) * CLIENT_SIZE);

    listenfd = my_socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((short)atoi(argv[1]));

    my_bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));

    unsigned int len = sizeof(addr);
    getsockname(listenfd, (struct sockaddr *)&addr, &len);

    printf("\nListening on port %s...\n", argv[1]);

    my_listen(listenfd, 5);

    for(int id = 0; id < CLIENT_SIZE; ++id) {
        pipe(p2c_fd[id]); 
        pipe(c2p_fd[id]);
        pipe(c2p_newcli_fd[id]);
        if((cpid = fork()) == 0) {
            if(id >= 1) {
                //Close existing child pipes
                for(int i = 0; i < id; ++i) {
                    close(p2c_fd[i][0]); close(c2p_fd[i][1]);
                    close(c2p_fd[i][0]); close(c2p_fd[i][1]);
                    close (c2p_newcli_fd[i][0]); close (c2p_newcli_fd[i][1]);
                }
            }
            start_client(id, listenfd, recaddr, p2c_fd[id], c2p_fd[id]);
            exit(0);
        } else if(cpid < 0) {
            my_error("fork failed");
        } else {
            close(p2c_fd[id][0]); close(c2p_fd[id][1]);
            close (c2p_newcli_fd[id][1]);
            FD_SET(c2p_fd[id][0], &rset);
            FD_SET(c2p_newcli_fd[id][0], &rset_newcli);
            maxr_fd = c2p_fd[id][0] + 1;
            maxr_newcli_fd = c2p_newcli_fd[id][0] + 1;
        }
    }

    for ( ; ; ) {
        //my_select(maxr_fd, &rset, NULL, NULL, &tv);
        my_select(maxr_newcli_fd, &rset_newcli, NULL, NULL, &tv);

        for (int i = 0; i < NEW_CLIENT; ++i) {
            if (FD_ISSET(c2p_newcli_fd[i][0], &rset_newcli)) {
                my_read(c2p_newcli_fd[i][0], buf, sizeof(buf));
                client_names[i] = (char*)malloc(sizeof(char)*strlen(buf));
                strcpy(client_names[i], buf);
                printf("client_names[%d]: %s\n", i, client_names[i]);
            }
        }
    }
}




