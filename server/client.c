#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "server.h"
#include "client.h"
#include "utility.h"
#include "login.h"

#define CMD_LENGTH 100
#define MSG_LENGTH 500

void print_help(int fd) {
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

void create_mail_file(const char *user_id) {
    FILE *outfile;
    char path[100];

    sprintf(path, "./mail/%s.dat", user_id);

    if ( (outfile = fopen(path, "w")) == NULL)
        my_error("Unable to open mail file");
    else
        fprintf(outfile, "Your messages:\n");

    fclose(outfile);
}

void register_cmd(int fd, char *cmd) {
    char buf[20];
    char user_id[USERID_LENGTH], passwd[PASSWORD_LENGTH];
    char msg[MSG_LENGTH];

    sscanf(cmd, "%s %s %s", buf, user_id, passwd);
    if(register_user(user_id, passwd) == 0) {
        strcpy(msg, "\nRegistration Completed!");
        my_write(fd, msg, strlen(msg));
        create_mail_file(user_id);
    } else {
        strcpy(msg, "\nRegistration Failed!");
        my_write(fd, msg, strlen(msg));
        my_error("Registration Failed!");
    }
}

void who_cmd(int fd) {
    char send_msg[MSG_LENGTH];
    char users[400] = "";
    int num_users = 0;

    for(int i = 0; i < CLIENT_SIZE; ++i) {
        if(client[i].cli_sock != -1) {
            strcat(users, client[i].user_id);
            strcat(users, " ");
            ++num_users;
        }
    }

    sprintf(send_msg, "Total %d user(s) online:\n%s\n", num_users, users);
    my_write(fd, send_msg, strlen(send_msg));
}

void tell_cmd(int fd, char *cmd) {
    int i, recv_cli_sock;
    char buf[20];
    char recv_id[USERID_LENGTH];
    char msg[MSG_LENGTH];
    char send_msg[MSG_LENGTH];

    sscanf(cmd, "%s %s %s", buf, recv_id, msg);

    for(i = 0; i < CLIENT_SIZE; ++i) {     // check if client exists
        if(strcmp(recv_id, client[i].user_id) == 0) {
            recv_cli_sock = client[i].cli_sock;
            break;
        }
    }

    if(i == CLIENT_SIZE) {     // client does not exist
        sprintf(msg, "User %s is not online", recv_id);
        my_write(fd, msg, strlen(msg));
    } else {
        sprintf(send_msg, "%s: %s", recv_id, msg);
        my_write(recv_cli_sock, send_msg, strlen(send_msg));
    }
}

void shout_cmd(int tid, char *cmd) {
    char buf[20];
    char send_msg[MSG_LENGTH], msg[MSG_LENGTH];

    // Format message
    sscanf(cmd, "%s %s", buf, msg);
    sprintf(send_msg, "!shout! *%s*: %s\n", client[tid].user_id, msg);

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

    sscanf(cmd, "%s %s %s", buf, recv_id, subject);

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

            fprintf(mail_file, "index %d\n", index);
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

void run_command(int tid, char* cmd) {
    char msg[MSG_LENGTH];
    int cli_sock = client[tid].cli_sock;

    //printf ("command:%s\n", cmd);
    if(starts_with(cmd, "register") == 0) {
        register_cmd(cli_sock, cmd);
    } else if(strcmp(cmd, "who") == 0) {
        who_cmd(cli_sock);
    } else if(strcmp(cmd, "help") == 0) {
        print_help(cli_sock);
    } else if (starts_with(cmd, "tell") == 0) {
        tell_cmd(cli_sock, cmd);
    } else if (starts_with(cmd, "shout") == 0) {
        shout_cmd(tid, cmd);
    } else if (strcmp(cmd, "quiet") == 0) {
        quiet_cmd(tid);
    } else if (strcmp(cmd, "nonquiet") == 0) {
        nonquiet_cmd(tid);
    } else if (starts_with(cmd, "block") == 0) {
        block_cmd(tid, cmd);
    } else if (starts_with(cmd, "unblock") == 0) {
        unblock_cmd(tid, cmd);
    } else if (starts_with(cmd, "mail") == 0) {
        mail_cmd(tid, cmd);
    } else {
        if(strcmp(cmd, "exit") != 0 && strcmp(cmd, "quit") != 0
                && cmd[0] != '\0') {
            strcpy(msg, "Command not supported.");
            my_write(cli_sock, msg, strlen(msg));
        }
    }
}

void close_client(int tid) {
    my_close(client[tid].cli_sock);
    client[tid].cli_sock = -1;
}

// TODO: if one thread closes, all threads and server closes
void *start_client(void *arg) {
    int command_counter = 0;
    char cmd[CMD_LENGTH], msg[MSG_LENGTH];
    char user_id[USERID_LENGTH], passwd[PASSWORD_LENGTH];

    int cli_sock;
    struct sockaddr_in cli_addr;
    socklen_t cli_len;

    int tid = *((int*)arg);
    client[tid].tid = tid;


    while(true) {
        // Idle till connection is made
        client[tid].cli_sock = cli_sock = 
            my_accept(listen_fd, (struct sockaddr *)(&cli_addr), &cli_len); 

        printf("Client connected to child %d <socket = %d machine = %s, port = %x, %x.>\n", 
            tid, cli_sock, inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port, ntohs(cli_addr.sin_port));

        // Login
        my_write(cli_sock, "\n\n\nusername(guest): ", 20);
        my_read(cli_sock, user_id, USERID_LENGTH);
        //If user didn't enter anything, we give them a guest user_id
        //printf("checking name\n");
        if(user_id[0] == '\0') {
            set_userid(tid, "guest");
        } else {
            my_write(cli_sock, "password: ", 11);
            my_read(cli_sock, passwd, PASSWORD_LENGTH);
            // The user is trying to login, authenticate who they are
            if(authenticate_user(user_id, passwd) == 0) {
                set_userid(tid, user_id);
            }
            else {
                strcpy(msg, "Login failed!! \nThank you for using Online Tic-tac-toe Server.\
                    \nSee you next time\n\n");
                my_write(cli_sock, msg, strlen(msg));
                close_client(tid);
                continue;
            }
        }

        print_help(cli_sock);
        if(strcmp(get_userid(tid), "guest") == 0) {
            strcpy(msg, "\nYou login as a guest.The only command that you can use is \
                \n'register <username> <password>'");
            my_write(cli_sock, msg, strlen(msg));
        }
        my_write(cli_sock, "\n\n", 2);

        do {  // Game loop
            sprintf(msg, "\n<%s: %d> ", get_userid(tid), command_counter);
            my_write(cli_sock, msg, strlen(msg));
            my_read(cli_sock, cmd, CMD_LENGTH);
            // user_id is set, based on their type allow them to run commands
            if(strcmp(get_userid(tid), "guest") == 0) {
                if(starts_with(cmd, "register") != 0 && (strcmp(cmd, "exit") != 0 
                                                  && strcmp(cmd, "quit") != 0)) {
                    strcpy(msg, "\nYou are not supposed to do this. \
                                    \nYou can only use 'register <username> <password>' as guest.\n");
                    my_write(cli_sock, msg, strlen(msg));
                } else {
                    if(starts_with(cmd, "register") == 0)
                        register_cmd(cli_sock, cmd);
                }
            } else {
                run_command(tid, cmd);
            }

            ++command_counter;
        } while(strcmp(cmd, "exit") != 0 && strcmp(cmd, "quit") != 0);

        close_client(tid);
    }
}