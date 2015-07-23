#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>

#include "server.h"
#include "client.h"
#include "utility.h"
#include "login.h"
#include "message.h"
#include "game.h"

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

void passwd_cmd(int tid, char *cmd) {
    FILE *infile, *copy_infile;
    char new_passwd[PASSWORD_LENGTH], buf[30];
    char file_path[] = "./login/login_details.dat";
    char copy_path[] = "./login/copy.dat";
    sscanf(cmd, "%s %s", buf, new_passwd);

    if ( (copy_infile = fopen(copy_path, "a+")) == NULL) {
        my_error("Unable to open copy passwd file");
    }

    if ( (infile = fopen(file_path, "a+")) == NULL) {
        my_error("Unable to open passwd file");
    } else {
        char id[USERID_LENGTH], pass[PASSWORD_LENGTH];
        while (fscanf(infile, "%s %s\n", id, pass) != EOF) {
            if (strcmp(id, client[tid].user_id) == 0)
                strcpy(pass, new_passwd);
            fprintf(copy_infile, "%s %s\n", id, pass);
        }

        my_write(client[tid].cli_sock, "Password changed.", 17);

        fclose(copy_infile);
        fclose(infile);
        remove(file_path);
        rename(copy_path, file_path);
    }

}

void run_command(int tid, char* cmd) {
    char msg[MSG_LENGTH];
    int cli_sock = client[tid].cli_sock;

    //printf ("command:%s\n", cmd);
    if(starts_with(cmd, "register") == 0) {
        if (check_args(cmd, 3) == true)
            register_cmd(cli_sock, cmd);
        else
            my_write(client[tid].cli_sock, "Invalid format", 14);
    } else if(strcmp(cmd, "who") == 0) {
        who_cmd(cli_sock);
    } else if(strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        print_help(cli_sock);
    } else if(starts_with(cmd, "tell") == 0) {
        tell_cmd(cli_sock, cmd);
    } else if(starts_with(cmd, "shout") == 0) {
        shout_cmd(tid, cmd);
    } else if(strcmp(cmd, "quiet") == 0) {
        quiet_cmd(tid);
    } else if(strcmp(cmd, "nonquiet") == 0) {
        nonquiet_cmd(tid);
    } else if(starts_with(cmd, "block") == 0) {
        block_cmd(tid, cmd);
    } else if(starts_with(cmd, "unblock") == 0) {
        unblock_cmd(tid, cmd);
    } else if(starts_with(cmd, "mail") == 0) {
        mail_cmd(tid, cmd);
    } else if(strcmp(cmd, "listmail") == 0) {
        listmail_cmd(tid);
    } else if(starts_with(cmd, "readmail") == 0) {
        // check for valid arguements
        readmail_cmd(tid, cmd);
    } else if(starts_with(cmd, "deletemail") == 0) {
        // Check for valid arguments 
        deletemail_cmd(tid, cmd);
    } else if(starts_with(cmd, "game") == 0) {
        list_games(tid);
    } else if(starts_with(cmd, "match") == 0) {
        if(check_args(cmd, 3) == true) {
            start_match(tid, cmd);
        }
    } else if (starts_with(cmd, "readmail") == 0) {
        if (check_args(cmd, 2) == true)
            readmail_cmd(tid, cmd);
        else
            my_write(client[tid].cli_sock, "Invalid format", 14);
    } else if (starts_with(cmd, "deletemail") == 0) {
        if (check_args(cmd, 2) == true)
            deletemail_cmd(tid, cmd);
        else
            my_write(client[tid].cli_sock, "Invalid format", 14);
    } else if (starts_with(cmd, "passwd") == 0) {
        if (check_args(cmd, 2) == true)
            passwd_cmd(tid, cmd);
        else
            my_write(client[tid].cli_sock, "Password must be one word", 25);
    } else {
        //This can be a game move
        //First check if a game is in progress
        if(client[tid].game_id >= 0) {
            //If yes, perform move
            if(make_a_move(tid, cmd) == -1) {
                strcpy(msg, "Invalid move.");
                my_write(cli_sock, msg, strlen(msg));
            }
        }
        //If no, its an unsupported command
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
