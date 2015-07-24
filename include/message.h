#ifndef _H_MESSAGE
#define _H_MESSAGE

void tell_cmd(int tid, char* cmd);
void shout_cmd(int tid, char* cmd);
void quiet_cmd(int tid);
void nonquiet_cmd(int tid);
void block_cmd(int tid, char* cmd);
void unblock_cmd(int tid, char* cmd);
void mail_cmd(int tid, char* cmd);
void listmail_cmd(int tid);
void readmail_cmd(int tid, char* cmd);
void deletemail_cmd(int tid, char* cmd);
void create_mail_file(const char* user_id);
void passwd_cmd(int tid, char* cmd);
void observe_cmd(int tid, char *cmd);
void unobserve_cmd(int tid);
void kibitz_cmd(int tid, char *cmd);
void comment_cmd(int tid, char *cmd);
void check_messages(int tid);
void info_cmd(int tid, char *cmd);
void check_quiet(int tid);
#endif
