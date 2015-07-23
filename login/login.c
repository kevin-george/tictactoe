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

void change_password(int tid, char *cmd) {
    FILE *infile, *copy_file;
    char new_passwd[PASSWORD_LENGTH], buf[20];
    char line[100];

    sscanf(cmd, "%s %s", buf, new_passwd);

    if ( (copy_file = fopen("./login/copy.dat", "w")) == NULL) {
        my_error("Unable to open copy.dat");
    }

    if ( (infile = fopen("./login/login_details.dat", "r")) == NULL) {
        fclose(copy_file);
        my_error("Unable to open login_details.dat");
    } else {
        char id[USERID_LENGTH], passwd[PASSWORD_LENGTH];
        while (fscanf(infile, "%s %s[^\n]", id, passwd) != EOF) {
            if (strcmp(id, client[tid].user_id) == 0)
                sprintf(line, "%s %s", id, new_passwd);
            else
                sprintf(line, "%s %s", id, passwd);

            fprintf(copy_file, "%s\n", line);
        }
        fclose(infile);
        fclose(copy_file);
        remove("./login/login_details.dat");
        rename("./login/copy.dat", "./login/login_details.dat");
        my_write(client[tid].cli_sock, "Password changed.", 17);
    }
}