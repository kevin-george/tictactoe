#include <stdio.h>
#include <string.h>

#include "login.h"
#include "utility.h"

void set_userid(char* user_id) {
  strcpy(user_name, user_id);
}

char* get_userid() {
  return user_name;
}

int register_user(char* user_id, char* passwd) {
  auth = fopen("login_details.dat", "a+");
  if(auth == NULL) {
    my_error("Unable to open login_details.dat");
  } else {
    char id[50], pass[50];
    while(fscanf(auth, "%50c[^,]%50c[^\n]", id, pass) != EOF) {
      if(strcmp(id, user_id) == 0) {
        fclose(auth);
        return -1;
      }
    }
    fprintf(auth, "%s,%s\n", user_id, passwd);
    fclose(auth);
    return 0;
  }
  return -1;
}

int authenticate_user(char* user_id, char* passwd) {
  auth = fopen("login_details.dat", "r");
  if(auth == NULL) {
    my_error("Unable to open login_details.dat");
  } else {
    char id[50], pass[50];
    while(fscanf(auth, "%50c[^,]%50c[^\n]", id, pass) != EOF) {
      if(strcmp(id, user_id) == 0 && strcmp(pass, passwd) == 0) {
        fclose(auth);
        return 0;
      }
    }
    fclose(auth);
  }
  return -1;
}
