#ifndef _H_CLIENT
#define _H_CLIENT

#include <netinet/in.h>
#include <stdbool.h>

#define USERID_LENGTH 50
#define PASSWORD_LENGTH 50

typedef struct client_t{
	int cli_sock;
	int tid;
	bool is_quiet;
	char user_id[USERID_LENGTH];
} client_t;

// pThread
void *start_client(void *arg);

void run_command(int tid, char *cmd);

void create_mail_file(const char *user_id);

// Commands
void who_cmd(int fd);
void print_help(int fd);
void register_cmd(int fd, char *cmd);
void tell_cmd(int fd, char *cmd);
void shout_cmd(int tid, char *cmd);
void quiet_cmd(int tid);
void nonquiet_cmd(int tid);
void block_cmd(int tid, char *cmd);
void unblock_cmd(int tid, char *cmd);
void mail_cmd(int tid, char *cmd);
void listmail_cmd(int tid);
void readmail_cmd(int tid, char *cmd);
void deletemail_cmd(int tid, char *cmd);

// Cleanup
void close_client(int tid);


#endif
