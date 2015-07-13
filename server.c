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
#include <string.h>

#include "server.h"
#include "utility.h"


#define CLIENT_SIZE 20
int child_id = 0;



int main(int argc, char * argv[])
{
    int listenfd;
    int cpid;
    struct sockaddr_in addr, recaddr;
    struct sigaction abc;
    int p2c_fd[20][2], c2p_fd[20][2];

    abc.sa_handler = sig_chld;
    sigemptyset(&abc.sa_mask);
    abc.sa_flags = 0;

    sigaction(SIGCHLD, &abc, NULL);

    if(argc < 2) {
        my_log("Usage: ./server <port number>.\n");
    }

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
        if((cpid = fork()) == 0) {
            if (id >= 1) {
                // Close existing child pipes
                for (int i = 0; i < id; ++i) {
                    close(p2c_fd[id][0]); close(c2p_fd[id][1]);
                    close(c2p_fd[id][0]); close(c2p_fd[id][1]);
                }
            }
            pipe(p2c_fd[id]); pipe(c2p_fd[id]);
            start_client(id, listenfd, recaddr, p2c_fd[id], c2p_fd[id]);
        } else if (cpid < 0) {
            my_error("fork failed");
        } else {
            close(p2c_fd[id][0]); close(c2p_fd[id][1]);
        }
    }

    for ( ; ; ) {

    }
}

void start_client(int id, int listenfd, struct sockaddr_in cliaddr, int *p2c_fd, int *c2p_fd) {
    const size_t CMD_LEN = 24;
    char cmd[CMD_LEN];
    int cli_sock;
    socklen_t clilen = sizeof(cliaddr);

    cli_sock = my_accept(listenfd, (struct sockaddr *)(&cliaddr), &clilen); 

    printf("Client connected to child %d <machine = %s, port = %x, %x.>\n", 
        id, inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port, ntohs(cliaddr.sin_port));

    help(cli_sock);

    do {
        // Get game command
        int n = my_read(cli_sock, cmd, CMD_LEN); 


    } while(strcmp(cmd, "exit") != 0 && strcmp(cmd, "quit") != 0);


    close(cli_sock);  
    exit(0);
}

void help(int fd) {
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

void sig_chld(int signo)
{
  pid_t pid;
  int stat;
  while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) 
    printf("child %d terminated.\n", (int)pid);
  return ;
}
