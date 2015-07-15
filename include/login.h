#ifndef _H_LOGIN
#define _H_LOGIN

char user_name[20];
FILE *auth;

void set_userid(char* user_id);
char* get_userid();
int register_user(char* user_id, char* passwd);
int authenticate_user(char* user_id, char* passwd);

#endif
