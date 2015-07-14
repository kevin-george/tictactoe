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
#define USERNAME_LENGTH 20

int child_id = 0;

void help(int fd) {
  char full_string[1500];
  char *str = full_string;
  str += sprintf(str, "\nCommands supported:\n"); 
  str += sprintf(str, "  %-24s # List all online users\n", "who");
  str += sprintf(str, "  %-24s # Display user information\n", "stats");
  str += sprintf(str, "  %-24s # List all current games\n", "game");
  str += sprintf(str, "  %-24s # Observe a game\n", "observe <game_num>");
  str += sprintf(str, "  %-24s # Unobserve a game\n", "unobserve");
  str += sprintf(str, "  %-24s # Try to start a game\n", "match <name> <b|w> [t]");
  str += sprintf(str, "  %-24s # Make a move in a game\n", "<A|B|C<1|2|3>");
  str += sprintf(str, "  %-24s # Resign a game\n", "resign");
  str += sprintf(str, "  %-24s # Refresh a game\n", "refresh");   
  str += sprintf(str, "  %-24s # Shout <msg> to every one online\n", "shout <msg>");
  str += sprintf(str, "  %-24s # Tell user <name> message\n", "tell <name> <msg>");
  str += sprintf(str, "  %-24s # Comment on a game when observing\n", "kibitz <msg>");
  str += sprintf(str, "  %-24s # Comment on a game\n", "' <msg>");
  str += sprintf(str, "  %-24s # Quiet mode, no broadcast messages\n", "quiet");
  str += sprintf(str, "  %-24s # Non-quiet mode\n", "nonquiet");
  str += sprintf(str, "  %-24s # No more communication from <id>\n", "block <id>");
  str += sprintf(str, "  %-24s # Allow communication from <id>\n", "unblock <id>");
  str += sprintf(str, "  %-24s # List the header of the mails\n", "listmail");
  str += sprintf(str, "  %-24s # Read the particular mail\n", "readmail <msg_num>");
  str += sprintf(str, "  %-24s # Delete the particular mail\n", "deletemail <msg_num>");
  str += sprintf(str, "  %-24s # Send id a mail\n", "mail <id> <title>");
  str += sprintf(str, "  %-24s # Change your information to <msg>\n", "info <msg>");
  str += sprintf(str, "  %-24s # Change password\n", "passwd <new>");
  str += sprintf(str, "  %-24s # Quit the system\n", "exit");
  str += sprintf(str, "  %-24s # Quit the system\n", "quit");
  str += sprintf(str, "  %-24s # Print this message\n", "help");
  str += sprintf(str, "  %-24s # Print this message\n", "?");
  my_write(fd, full_string, strlen(full_string));
}

void sig_chld(int signo) {
  pid_t pid;
  int stat;
  while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) 
    printf("child %d terminated.\n", (int)pid);
  return ;
}

void start_client(int id, int listenfd, struct sockaddr_in cliaddr, 
                                                int *p2c_fd, int *c2p_fd) {
  const size_t CMD_LEN = 24;
  char cmd[CMD_LEN];
  int cli_sock;
  char logged_in = 'N';
  char username[USERNAME_LENGTH];
  char message[100];
  int command_counter = 0;
  
  socklen_t clilen = sizeof(cliaddr);
  cli_sock = my_accept(listenfd, (struct sockaddr *)(&cliaddr), &clilen); 

  printf("Client connected to child %d <machine = %s, port = %x, %x.>\n", 
      id, inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port, ntohs(cliaddr.sin_port));

  do{
    //First present with a guest login prompt
    if(logged_in == 'N') {
      my_write(cli_sock, "\n\n\nusername(guest):", 19);
      my_read(cli_sock, cmd, CMD_LEN);
      if(cmd[0] == '\0') {
        help(cli_sock);
        strcpy(message, "\nYou login as a guest.\
The only command that you can use is\n'register <username> <password>'");
        my_write(cli_sock, message, strlen(message));
        logged_in = 'Y';
        strcpy(username, "guest"); 
      }
    } else {
      sprintf(message, "\n\n<%s: %d> ", username, command_counter);
      my_write(cli_sock, message, strlen(message));
      my_read(cli_sock, cmd, CMD_LEN);
      if(starts_with(cmd, "register") != 0 && (strcmp(cmd, "exit") != 0 
                                          && strcmp(cmd, "quit") != 0)) {
        strcpy(message, "\nYou are not supposed to do this.\
\nYou can only use 'register <username> <password>' as guest.");
        my_write(cli_sock, message, strlen(message));
      } else {
        //The guest wishes to register a user
        
      }
      command_counter++;
    }
  } while(strcmp(cmd, "exit") != 0 && strcmp(cmd, "quit") != 0);

  close(cli_sock);  
}

int main(int argc, char * argv[]) {
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
      //Why are we skipping id = 0?
      if(id >= 1) {
        //Close existing child pipes
        for(int i = 0; i < id; ++i) {
          close(p2c_fd[i][0]); close(c2p_fd[i][1]);
          close(c2p_fd[i][0]); close(c2p_fd[i][1]);
        }
      }
      pipe(p2c_fd[id]); pipe(c2p_fd[id]);
      start_client(id, listenfd, recaddr, p2c_fd[id], c2p_fd[id]);
      exit(0);
    } else if(cpid < 0) {
      my_error("fork failed");
    } else {
      close(p2c_fd[id][0]); close(c2p_fd[id][1]);
    }
  }

  for ( ; ; ) {

  }
}




