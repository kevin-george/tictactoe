#ifndef _H_LOGIN
#define _H_LOGIN

#define  USERNAME_LENGTH 50
#define PASSWORD_LENGTH 50

char user_name[USERNAME_LENGTH];
FILE *auth;

void set_userid(char* user_id);
char* get_userid();
int register_user(char* user_id, char* passwd);
int authenticate_user(char* user_id, char* passwd);

#endif
