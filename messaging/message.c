#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>

#include "login.h"
#include "utility.h" 
#include "server.h"
#include "client.h"
#include "message.h"
#include "game.h"

void tell_cmd(int tid, char *cmd) {
    int i, recv_cli_sock;
    char buf[20], buf2[30];
    char recv_id[USERID_LENGTH];
    char msg[MSG_LENGTH];
    char send_msg[MSG_LENGTH];

    sscanf(cmd, "%s %s", buf, recv_id);
    
    

    for(i = 0; i < CLIENT_SIZE; ++i) {     // check if client exists
        if(strcmp(recv_id, client[i].user_id) == 0) {
            recv_cli_sock = client[i].cli_sock;
            break;
        }
    }

    if(i == CLIENT_SIZE) {     // client does not exist
        sprintf(msg, "User %s is not online", recv_id);
        my_write(client[tid].cli_sock, msg, strlen(msg));
    } else {
        //sprintf(send_msg, "%s: %s", recv_id, msg);
        sprintf(buf2, "tell %s ", recv_id);
        sprintf(send_msg, "%s: ", client[tid].user_id);
        strcat(send_msg, (cmd + strlen(buf2)));
        my_write(recv_cli_sock, send_msg, strlen(send_msg));
    }
}

void shout_cmd(int tid, char *cmd) {
    char send_msg[MSG_LENGTH];

    // Format message
    sprintf(send_msg, "!shout! *%s*: ", client[tid].user_id);
    strcat(send_msg, (cmd + strlen("shout ")));

    for(int i = 0; i < CLIENT_SIZE; ++i) {
        if(client[i].cli_sock != -1 && client[i].is_quiet == false)
            my_write(client[i].cli_sock, send_msg, strlen(send_msg));
    }
}

void quiet_cmd(int tid) {
    client[tid].is_quiet = true;
    my_write(client[tid].cli_sock, "Enter quiet mode.", 17);
}

void nonquiet_cmd(int tid) {
    client[tid].is_quiet = false;
    my_write(client[tid].cli_sock, "Enter nonquiet mode.", 20);
}

void block_cmd(int tid, char *cmd) {    // Not tested
    FILE *in;
    char buf[20], block_path[100];
    char user_id[USERID_LENGTH], msg[MSG_LENGTH];
    bool is_blocked = false;

    sscanf(cmd, "%s %s", buf, user_id);     // Extract userid to block
    sprintf(block_path, "./block/%s.dat", client[tid].user_id);

    if ( (in = fopen(block_path, "a+")) == NULL) {
        my_error("Unable to open block file");
    } else {
        char id[USERID_LENGTH];
        while(fscanf(in, "%s[^\n]", id) != EOF) {
            if(strcmp(id, user_id) == 0) {
                sprintf(msg, "%s was blocked before.", user_id);
                my_write(client[tid].cli_sock,msg, strlen(msg));
                is_blocked = true;
                break;
            }
        }

        if (is_blocked == false)
            fprintf(in, "%s\n", user_id);
    }

    fclose(in);
}

void unblock_cmd(int tid, char *cmd) {  // Not tested
    FILE *in, *c_in;
    char buf[20], block_path[100], copy_path[100];
    char user_id[USERID_LENGTH], msg[MSG_LENGTH];
    bool is_blocked = false;

    sscanf(cmd, "%s %s", buf, user_id);     // Extract userid to block
    sprintf(block_path, "./block/%s.dat", client[tid].user_id);
    sprintf(copy_path, "./block/copy_%s.dat", client[tid].user_id);
    
    if ( (c_in = fopen(copy_path, "w")) == NULL)
        my_error("Unable to open block file copy");

    if ( (in = fopen(block_path, "r+")) == NULL) {
        fclose(c_in);
        my_error("Unable to open block file");
    } else {
        char id[USERID_LENGTH];
        while (fscanf(in, "%s[^\n]", id) != EOF) {
            if (strcmp(id, user_id) == 0) {
                fseek(in, strlen(id)+1, SEEK_CUR);
                fgets(id, strlen(id)+1, in);
                sprintf(msg, "User %s unblocked.", user_id);
                my_write(client[tid].cli_sock, msg, strlen(msg));
                is_blocked = true;
            } else {
                fprintf(c_in, "%s\n", id);
            }
        }

        if (is_blocked == false) {
            sprintf(msg, "User %s was not blocked", user_id);
            my_write(client[tid].cli_sock,msg, strlen(msg));
        }
        
        fclose(c_in);
        fclose(in);

        remove(block_path);
        rename(copy_path, block_path);
    }
}

void mail_cmd(int tid, char *cmd) {
    FILE *mail_file, *user_file;
    char buf[20], mail_path[100]; 
    char recv_id[USERID_LENGTH], subject[30];
    char msg[MSG_LENGTH];
    int recv_cli_sock = -1;
    bool user_exists = false, user_is_online = false;

    sscanf(cmd, "%s %s", buf, recv_id);
    strcpy(subject, cmd + strlen(buf) + strlen(recv_id)+2);

    // check if user is online
    for (int i = 0; i < CLIENT_SIZE; ++i) {      
        if (client[i].cli_sock != -1) {
            if (strcmp(recv_id, client[i].user_id) == 0) {
                user_exists = true;
                user_is_online = true;
                recv_cli_sock = client[i].cli_sock;
                break; 
            }
        }
    }
    
    // If user is not online, check database
    if (user_exists == false) {
        if( (user_file = fopen("./login/login_details.dat", "r")) == NULL) {
            my_error("Unable to open login_details.dat");
        } else {
            char id[USERID_LENGTH], pass[PASSWORD_LENGTH];
            while(fscanf(user_file, "%s %s\n", id, pass) != EOF) {
                if(strcmp(id, recv_id) == 0) {
                    user_exists = true;
                }
            }
            fclose(user_file);
        }
    }

    // Send message
    if (user_exists == true) {
        sprintf(msg, "Please input mail body, finishing with '.' at the begining of a line\n");
        my_write(client[tid].cli_sock, msg, strlen(msg));

        my_mread(client[tid].cli_sock, msg, MSG_LENGTH);   // get body

        // Store message in mail database
        sprintf(mail_path, "./mail/%s.dat", recv_id);
        if ( (mail_file = fopen(mail_path, "a+")) == NULL) {
            my_error("Unable to open mail file");
        } else {
            char temp[MSG_LENGTH], temp2[30], temp3[MSG_LENGTH];
            char *time_stamp;
            int index = 0;
            time_t now = time(NULL);
            time_stamp = asctime(localtime(&now));

            while(fscanf(mail_file, "%s[^\n]", temp) != EOF) {
                sscanf(temp, "%s %s", temp2, temp3);
                if (strcmp(temp2, "index") == 0)
                    ++index;
            }

            //fprintf(mail_file, "index %d\n", index);
            fprintf(mail_file, "status N\n");
            fprintf(mail_file, "user %s\n", client[tid].user_id);
            fprintf(mail_file, "subject \"%s\"\n", subject);
            fprintf(mail_file, "timestamp %s\n", time_stamp);
            fprintf(mail_file, "body %s\n", msg);

            my_write(client[tid].cli_sock, "Message sent", strlen("Message sent"));
            
            if (user_is_online == true)
               my_write(recv_cli_sock, "A new message just arrived.\n", 28);

            fclose(mail_file);
        }
    } else {
        sprintf(msg, "%s is not a user.", recv_id);
        my_write(client[tid].cli_sock, msg, strlen(msg));
    }

}

void listmail_cmd(int tid) {
    FILE *infile;
    int index = 0;
    char mail_path[100], header[500]; 
    char buf[30], buf2[30], line[100];
    char *str = header;

    sprintf(mail_path, "./mail/%s.dat", client[tid].user_id);

    if ( (infile = fopen(mail_path, "r")) == NULL) {
        my_error("Unable to open mail file");
    } else {
        if (fgets(line, 100, infile) != NULL) {
            str += sprintf(str, "%s", line);
        }
        while (fgets(line, 100, infile) != NULL) {
            sscanf(line, "%s %s", buf, buf2);
            if (strcmp(buf, "status") == 0) {
                str += sprintf(str, "  %-2d %-1s ", index++, buf2);
            } else if (strcmp(buf, "user") == 0) {
                str += sprintf(str, "%-10s ", buf2);
            } else if (strcmp(buf, "subject") == 0) {
                line[strlen(line) -1] = ' ';
                str += sprintf(str, "%-30s ", line+strlen(buf)+1);
            } else if (strcmp(buf, "timestamp") == 0) {
                str += sprintf(str, "%s", line+strlen(buf)+1);
            }
            strcpy(buf, " "); 
        }

        my_write(client[tid].cli_sock, header, strlen(header));
        fclose(infile);
    }
}

void readmail_cmd(int tid, char *cmd) {
    FILE *infile, *cfile;
    char buf[30], line[100];
    char msg[MSG_LENGTH], mail_path[50];
    char copy_path[50];
    int index, itr = -1;

    sscanf(cmd, "%s %d", buf, &index);      // extract mail index
    sprintf(mail_path, "./mail/%s.dat", client[tid].user_id);
    sprintf(copy_path, "./mail/copy_%s.dat", client[tid].user_id);

    if ( (cfile = fopen(copy_path, "a+")) == NULL)
        my_error("Unable to open copy readmail file");

    if ( (infile = fopen(mail_path, "r")) == NULL) {
        my_error("Unable to open readmail");
    } else {
        fgets(line, 100, infile);
        fprintf(cfile, "%s", line);

        while (fgets(line, 100, infile) != NULL) {
            sscanf(line, "%s", buf);
            if (strcmp(buf, "status") == 0)
                ++itr;
            if (index == itr) {
                if (strcmp(buf, "status") == 0) {
                    // write new itr value
                    sprintf(line, "status Y\n");
                } else if (strcmp(buf, "body") == 0) {
                    strcpy(msg, (line+strlen("body")));
                    fprintf(cfile, "%s", line);
                    while (fpeek(infile) != '.') {
                        fgets(line, 100, infile);
                        strcat(msg, " ");
                        strcat(msg, line);
                        fprintf(cfile, "%s", line);
                    } 
                    fgets(line, 100, infile);
                    strcat(msg, " ");
                    strcat(msg, line);

                    my_write(client[tid].cli_sock, msg, strlen(msg));
                }
                fprintf(cfile, "%s", line);
            } else 
                fprintf(cfile, "%s", line);

            
        }

        fclose(infile);
        fclose(cfile);
        remove(mail_path);
        rename(copy_path, mail_path);
    }
}

void deletemail_cmd(int tid, char *cmd) {
    FILE *infile, *cfile;
    char mail_path[50], copy_path[50];
    char buf[30], line[100];
    int index, itr = -1;

    sscanf(cmd, "%s %d", buf, &index);

    sprintf(mail_path, "./mail/%s.dat", client[tid].user_id);
    sprintf(copy_path, "./mail/copy_%s.dat", client[tid].user_id);

    if ( (cfile = fopen(copy_path, "a+")) == NULL)
        my_error("Unable to open copy readmail file");

    if ( (infile = fopen(mail_path, "r")) == NULL) {
        my_error("Unable to open readmail");
    } else {
        fgets(line, 100, infile);
        fprintf(cfile, "%s", line);

        while (fgets(line, 100, infile) != NULL) {
            sscanf(line, "%s", buf);
            if (strcmp(buf, "status") == 0)
                ++itr;
            if (index != itr)
                fprintf(cfile, "%s", line);
        }
        my_write(client[tid].cli_sock, "Message deleted", 15);
        fclose(infile);
        fclose(cfile);
        remove(mail_path);
        rename(copy_path, mail_path);
    }
}

void create_mail_file(const char *user_id) {
    FILE *outfile;
    char path[100];

    sprintf(path, "messaging/%s.dat", user_id);

    if((outfile = fopen(path, "w")) == NULL)
        my_error("Unable to open mail file");
    else
        fprintf(outfile, "Your messages:\n");

    fclose(outfile);
}

void observe_cmd(int tid, char *cmd) {

    if (client[tid].is_observing == false) {
        int inst_idx;
        char buf[10], msg[50];
        int observer_count;
        sscanf(cmd, "%s %d", buf, &inst_idx);

        observer_count = instances[inst_idx].observer_count++;
        instances[inst_idx].observers[observer_count][0] = tid;

        client[tid].observe_match_num = inst_idx;
        client[tid].is_observing = true;

        sprintf(msg, "Observing game %d", inst_idx);
        my_write(client[tid].cli_sock, msg, strlen(msg));
    } else {
        my_write(client[tid].cli_sock, "Unobserve before observing another game", 39);
    }
}

void unobserve_cmd(int tid) {
    char msg[50]; 
    int match_num = client[tid].observe_match_num;
    int observer_count = instances[match_num].observer_count;
    if (client[tid].is_observing == true) {
        for (int i = 0; i < observer_count; ++i) {
            if (instances[match_num].observers[i][0] == tid) {
                instances[match_num].observers[i][0] = -1;
                client[tid].observe_match_num = -1;
                client[tid].is_observing = false;
            }
        }
        sprintf(msg, "Unobserving game %d", match_num);
        my_write(client[tid].cli_sock, msg, strlen(msg));
    } else {
        // write to user
        my_write(client[tid].cli_sock, "You are not observing anything", 30);
    }
}

void kibitz_cmd(int tid, char *cmd) {
    if (client[tid].is_observing == true) {
        int observe_match_num = client[tid].observe_match_num;
        int player1_tid = instances[observe_match_num].player1_tid;
        int player2_tid = instances[observe_match_num].player2_tid;
        char msg[MSG_LENGTH];
        sprintf(msg, "Kibitz* %s:", client[tid].user_id);
        strcat(msg, (cmd + strlen("kibitz ")));

        my_write(client[player1_tid].cli_sock, msg, strlen(msg));
        my_write(client[player2_tid].cli_sock, msg, strlen(msg));

        for (int i = 0; i < instances[observe_match_num].observer_count; ++i) {
            my_write(client[instances[observe_match_num].observers[i][0]].cli_sock, msg, strlen(msg));
        }
    } else {
        my_write(client[tid].cli_sock, "You are not observing a game", 28);
    }
}

void comment_cmd(int tid, char *cmd) {
    if (client[tid].game_on == true) {
        int match_num = client[tid].game_id;
        int player1_tid = instances[match_num].player1_tid;
        int player2_tid = instances[match_num].player2_tid;
        char msg[MSG_LENGTH];
        sprintf(msg, "'* %s:", client[tid].user_id);

        strcat(msg, (cmd + strlen("' ")));

        my_write(client[player1_tid].cli_sock, msg, strlen(msg));
        my_write(client[player2_tid].cli_sock, msg, strlen(msg));

        for (int i = 0; i < instances[match_num].observer_count; ++i) {
            my_write(client[instances[match_num].observers[i][0]].cli_sock, msg, strlen(msg));
        }
    } else {
        my_write(client[tid].cli_sock, "You are not playing a game", 26);
    }
}

void check_messages(int tid) {
    FILE *infile;
    char line[100], buf[30], buf2[30];
    char msg[MSG_LENGTH], mail_path[50];
    int num_unread = 0;

    sprintf(mail_path, "./mail/%s.dat", client[tid].user_id);

    if ( (infile = fopen(mail_path, "r")) == NULL) {
        my_error("Unable to open readmail");
    } else {
        fgets(line, 100, infile);

        while (fgets(line, 100, infile) != NULL) {
            sscanf(line, "%s %s", buf, buf2);
            if (strcmp(buf, "status") == 0) {
                if (strcmp(buf2, "N") == 0) {
                    ++num_unread;
                }
            }
        }
        if (num_unread == 0) {
            sprintf(msg, "You have no unread messages.");
            my_write(client[tid].cli_sock, msg, strlen(msg));
        } else {
            sprintf(msg, "You have %d unread messages.", num_unread);
            my_write(client[tid].cli_sock, msg, strlen(msg));
        }
        fclose(infile);
    }
}














