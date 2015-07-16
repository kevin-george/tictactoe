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
#include <stdbool.h>

#include "server_child.h"
#include "utility.h"

#define CLIENT_SIZE 20
#define MSG_SIZE 500

int child_id = 0;

void sig_chld(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) 
        printf("child %d terminated.\n", (int)pid);
}

int main(int argc, char * argv[]) {
    int listenfd, maxr_fd;
    int cpid, nready;
    char msg[MSG_SIZE];
    char **clients;
    bool is_connected[CLIENT_SIZE] = { false };
    struct sockaddr_in addr, recaddr;
    struct sigaction abc;
    int p2c_fd[20][2], c2p_fd[20][2];
    fd_set rset, mset;
    struct timeval tv;

    tv.tv_sec = 5;
    FD_ZERO(&mset);

    abc.sa_handler = sig_chld;
    sigemptyset(&abc.sa_mask);
    abc.sa_flags = 0;

    sigaction(SIGCHLD, &abc, NULL);

    if(argc < 2) {
        my_log("Usage: ./server <port number>.\n");
    }

    clients = (char**)malloc(sizeof(char*) * CLIENT_SIZE);

    listenfd = my_socket(AF_INET, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
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
        if((cpid = fork()) == 0) {
            /*if(id >= 1) {
                //Close existing child pipes
                for(int i = 0; i < id; ++i) {
                    close(p2c_fd[i][0]); close(c2p_fd[i][1]);
                    close(c2p_fd[i][0]); close(c2p_fd[i][1]);
                }
            }*/
            close(p2c_fd[id][1]); close(c2p_fd[id][0]);
            start_client(id, listenfd, recaddr, p2c_fd[id], c2p_fd[id]);
            exit(0);
        } else if(cpid < 0) {
            my_error("fork failed");
        } else {
            FD_SET(c2p_fd[id][0], &mset);
            maxr_fd = c2p_fd[id][0] + 1;
        }
    }

    for (int i = 0; i < CLIENT_SIZE; ++i) {
        close(p2c_fd[i][0]); close(c2p_fd[i][1]);
    }


    for ( ; ; ) {
        rset = mset;
        nready = my_select(maxr_fd, &rset, NULL, NULL, &tv );
        printf("nready:%d\n", nready);
        for (int i = 0; i < CLIENT_SIZE && nready > 0; ++i) {
            if (nready > 0) {
                if (FD_ISSET(c2p_fd[i][0], &rset)) {
                    printf("c2p_fd[%d][0] set\n", i);
                    if (is_connected[i] == false) {
                        printf("id:%d read:%d\n", i, c2p_fd[i][0]);
                        my_read(c2p_fd[i][0], msg, sizeof(char)*MSG_SIZE);      // map username to child process
                        clients[i] = (char*)malloc(sizeof(char)*strlen(msg));
                        strcpy(clients[i], msg);
                        printf("clients[%d]: %s\n", i, clients[i]);
                        is_connected[i] = true;
                    } else {
                        my_read(c2p_fd[i][0], msg, sizeof(char)*MSG_SIZE);
                        printf("id:%d name:%s msg:%s\n", i, clients[i], msg);
                    }
                    --nready;
                }
            } else
                continue;
        }
    }
}




