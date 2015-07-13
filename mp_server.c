#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "server.h"
#include "utility.h"

#define CLIENT_SIZE 20
int child_id = 0;



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
    int cpids[CLIENT_SIZE];
    int p2c_pipe[20][2], c2p_pipe[20][2];   // parent-2-child, child-2-parent

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

    // Prefork processes
    for (int id = 0; id < CLIENT_SIZE; ++id) {
        if ((cpids[id] = fork()) == 0) {
            if (id >= 1) {     // Close exisiting child pipes
                for (int j = 0; j < id; ++j) {
                    close(p2c_pipe[id][0]); close(p2c_pipe[id][1]);
                    close(c2p_pipe[id][0]); close(c2p_pipe[id][1]);
                }
            }
            pipe(p2c_pipe[id]); pipe(c2p_pipe[id]);
            start_client(id, p2c_pipe[id], c2p_pipe[id]);
        } else if (cpids[id] == -1) {
            my_error("fork failed");
        } else {
            close(p2c_pipe[id][0]); close(c2p_pipe[id][1]);
        }
    }

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
                char str_sockfd[3];
                sprintf(str_sockfd, "%d", sockfd);
                printf("Write - %s to client - %d\n", str_sockfd, i);
                write(p2c_pipe[i][1], (void*)str_sockfd, strlen(str_sockfd));
                break;
            }
        }
        break;
    }
 }

void sig_chld(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) 
        printf("child %d terminated.\n", (int)pid);
}

void start_client(int id, int *p2c_pipe, int *c2p_pipe) {
    char sockfd[3];
    close(c2p_pipe[0]);
    close(p2c_pipe[1]);
    // TODO: This isn't blocking for some reason
    // Block child process till connection is made
    read(p2c_pipe[0], (void*)sockfd, sizeof(char)*2);
    printf("start_client - sockfd:%s sockfd:%d\n", sockfd, atoi(sockfd));

    //show_commands(atoi(sockfd));
    exit(0);
}

void show_commands(int fd) {
    char str[70];
    sprintf(str, "\nCommands supported:\n"); 
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # List all online users\n", "who");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Display user information\n", "stats");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # List all current games\n", "game");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Observe a game\n", "observe <game_num>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Unobserve a game\n", "unobserve");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Try to start a game\n", "match <name> <b|w> [t]");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Make a move in a game\n", "<A|B|C<1|2|3>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Resign a game\n", "resign");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Refresh a game\n", "refresh");   
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Shout <msg> to every one online\n", "shout <msg>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Tell user <name> message\n", "tell <name> <msg>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Comment on a game when observing\n", "kibitz <msg>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Comment on a game\n", "' <msg>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Quiet mode, no broadcast messages\n", "quiet");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Non-quiet mode\n", "nonquiet");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # No more communication from <id>\n", "block <id>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Allow communication from <id>\n", "unblock <id>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # List the header of the mails\n", "listmail");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Read the particular mail\n", "readmail <msg_num>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Delete the particular mail\n", "deletemail <msg_num>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Send id a mail\n", "mail <id> <title>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Change your information to <msg>\n", "info <msg>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Change password\n", "passwd <new>");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Quit the system\n", "exit");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Quit the system\n", "quit");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Print this message\n", "help");
    my_write(fd, str, strlen(str));
    sprintf(str, "  %-24s # Print this message\n", "?");
    my_write(fd, str, strlen(str));
}
