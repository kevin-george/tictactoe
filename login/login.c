#include <stdio.h>
#include <string.h>

#include "login.h"
#include "server.h"
#include "utility.h"

void set_userid(int tid, char* user_id) {
  strncpy(client[tid].user_id, user_id, USERID_LENGTH);
}

char* get_userid(int tid) {
  return client[tid].user_id;
}

int register_user(char* user_id, char* passwd) {
  auth = fopen("./login/login_details.dat", "a+");
  if(auth == NULL) {
    my_error("Unable to open login_details.dat");
  } else {
    char id[50], pass[50];
    while(fscanf(auth, "%s %s[^\n]", id, pass) != EOF) {
      if(strcmp(id, user_id) == 0) {
        fclose(auth);
        return -1;
      }
    }
    fprintf(auth, "%s %s\n", user_id, passwd);
    fclose(auth);
    return 0;
  }
  return -1;
}

int authenticate_user(char* user_id, char* passwd) {
  auth = fopen("./login/login_details.dat", "r");
  if(auth == NULL) {
    my_error("Unable to open login_details.dat");
  } else {
    char id[USERID_LENGTH], pass[PASSWORD_LENGTH];
    while(fscanf(auth, "%s %s\n", id, pass) != EOF) {
      if(strcmp(id, user_id) == 0 && strcmp(pass, passwd) == 0) {
        fclose(auth);
        return 0;
      }
    }
    fclose(auth);
  }
  return -1;
}
