#ifndef _H_LOGIN
#define _H_LOGIN

#define USERID_LENGTH 50
#define PASSWORD_LENGTH 50

FILE *auth;

void set_userid(int tid, char* user_id);
char* get_userid(int tid);
int register_user(char* user_id, char* passwd);
int authenticate_user(char* user_id, char* passwd);

#endif
