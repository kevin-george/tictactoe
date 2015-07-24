#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "game.h"
#include "server.h"
#include "client.h"
#include "utility.h"
#include "microtime.h"


#define YES 1
#define NO 0

void print_game(int player1, int player2, int first_view, int result, int time_out) {
    char game_state[1500] = "";
    char *str = game_state;
    //Only first time view gets this
    if(first_view == YES) {
        if(client[player1].player_type == 'w')
            str += sprintf(str, "\n\nBlack:\t\t%s\t\tWhite:\t\t%s"
                    , client[player2].user_id, client[player1].user_id);
        else
            str += sprintf(str, "\nBlack:\t\t%s\t\tWhite:\t\t%s"
                    , client[player1].user_id, client[player2].user_id);
        str += sprintf(str, "\n  Time:\t%.0f seconds\t\t  Time:\t%.0f seconds\n\n"
                , client[player1].game_time_limit, client[player2].game_time_limit);
    }

    //To be replaced by actual game grid
    int game_id = client[player1].game_id;
    game *instance = &instances[game_id];
    str += sprintf(str, "\n    1  2  3");
    str += sprintf(str, "\nA   %c  %c  %c"
    , instance->game_grid[0][0], instance->game_grid[0][1], instance->game_grid[0][2]);
    str += sprintf(str, "\nB   %c  %c  %c"
    , instance->game_grid[1][0], instance->game_grid[1][1], instance->game_grid[1][2]);
    str += sprintf(str, "\nC   %c  %c  %c"
    , instance->game_grid[2][0], instance->game_grid[2][1], instance->game_grid[2][2]);
    
    if(result == YES) {
        if(instance->winner_tid != -1) {
            str += sprintf(str, "\n%s won the game",client[instance->winner_tid].user_id);
            if(time_out == YES)
                str+= sprintf(str, " by timeout");
        } else
            str += sprintf(str, "\nThe game was a tie");
    }
    fflush(stdout);

    if(player2 != -1) {
        //This is for player1 & player2
        my_write(client[player1].cli_sock, game_state, strlen(game_state)); 
        my_write(client[player2].cli_sock, game_state, strlen(game_state));
    } else {
        //This is for observers
        my_write(client[player1].cli_sock, game_state, strlen(game_state));
    }
}

void list_games(int tid) {
    char message[MSG_LENGTH] = "";
    char *str = message;
    for(int count = 0; count <= game_count; count++) {
        str += sprintf(str, "\nGame %d(%d): %s vs %s, %d moves"
                , count, count, client[instances[count].player1_tid].user_id
                , client[instances[count].player2_tid].user_id
                , instances[count].no_of_moves);
    }
    my_write(client[tid].cli_sock, message, strlen(message));
}

void start_match(int tid, char* cmd) {
    char id[USERID_LENGTH] = "";
    char type[2] = "";
    //Break the command down
    sscanf(cmd, "%*s %s %s", id, type);
    //Check if the user is trying to start a match with himself
    if(strcmp(id, client[tid].user_id) == 0) {
        char message[MSG_LENGTH];
        strcpy(message, "You cannot have a match with yourself.");
        my_write(client[tid].cli_sock, message, strlen(message));
        return;
    } else {
        //If not, check if the user is online
        bool user_is_online = false;
        for(int i = 0; i < CLIENT_SIZE; ++i) {      
            if(client[i].cli_sock != -1) {
                if(strcmp(id, client[i].user_id) == 0) {
                    user_is_online = true;
                    //If online, first check if i'm the inviter/invitee
                    if(!client[tid].game_on) {
                        //I'm the inviter, waiting..
                        client[i].game_on = true;
                        char p_type = (type[0] == 'w') ? 'b' : 'w';
                        char message[MSG_LENGTH];
                        sprintf(message, "%s invites you for a game <Please type -> match %s %c>."
                                , client[tid].user_id, client[tid].user_id, p_type);
                        my_write(client[i].cli_sock, message, strlen(message));
                    } else {
                        //I'm the invitee, so game on!
                        game_count++;
                        for(int i = 0;i < 3; i++)
                            for(int j = 0;j < 3;j++)
                                instances[game_count].game_grid[i][j] = '.';
                        instances[game_count].no_of_moves = 0;
                        instances[game_count].winner_tid = -1;
                        for(int i = 0; i < 20; i++)
                            instances[game_count].observers[i] = -1;
                        client[i].game_on = true;
                        //Setting player type
                        client[tid].player_type = type[0];
                        client[i].player_type = (type[0] == 'b') ? 'w' : 'b';
                        //Setting the game instance
                        client[i].game_id = client[tid].game_id = game_count;
                        //Hardcoding time limit for now
                        client[i].game_time_limit = client[tid].game_time_limit = 5.0;
                        //Black gets to go first
                        if(client[tid].player_type == 'b')
                            client[tid].game_turn = true;
                        else
                            client[i].game_turn = true;
                        //Setting user ids to game instance
                        instances[game_count].player1_tid = tid;
                        instances[game_count].player2_tid = i;
                        //Print game board
                        print_game(i, tid, YES, NO, NO);
                        //Timer starts for Black
                        instances[game_count].game_start = microtime();
                    }
                    break; 
                }
            }
        }
        if(!user_is_online) {
            char message[MSG_LENGTH];
            strcpy(message, "User is not online.");
            my_write(client[tid].cli_sock, message, strlen(message));
        }
    }
}

void create_stats(char *user_id) {
    FILE *file;
    char path[50];

    sprintf(path, "./gameplay/%s_stat.dat", user_id);
    if ( (file = fopen(path, "a+")) == NULL) {
        my_error("Cannont open user stat file");
    } else {
        fprintf(file, "User: %s\n", user_id);
        fprintf(file, "Info: none\n");
        fprintf(file, "Wins: 0\nLoses: 0\n");
        fprintf(file, "Quiet: no\n");
        fprintf(file, "Blocked: none\n");

        fclose(file);
    }
}

void print_stats(int tid, char *cmd) {
    FILE *file, *login_file;
    char path[50], line[50], user_id[USERID_LENGTH], buf[10];
    char msg[MSG_LENGTH];
    char *str = msg;

    sscanf(cmd, "%s %s", buf, user_id);

    if ( (login_file = fopen("./login/login_details.dat", "r")) == NULL) {
        my_error("Unable to open login file");
    } else {
        char id[USERID_LENGTH]; char passwd[PASSWORD_LENGTH];
        bool is_registered = false;
        while (fscanf(login_file, "%s %s[^\n]", id, passwd) != EOF) {
            if (strcmp(id, user_id) == 0) {
                is_registered = true;
                break;
            }
        }

        fclose(login_file);

        if (is_registered == false) {
            my_write(client[tid].cli_sock, "User does not exist.", 20);
            return;
        }

    }

    sprintf(path, "./gameplay/%s_stat.dat", user_id);
    if ( (file = fopen(path, "a+")) == NULL) {
        my_error("Cannont open user stat file");
    } else {
        while (fgets(line, 50, file) != NULL) {
            str += sprintf(str, "%s", line);
        }

        bool is_online = false;
        for (int i = 0; i < CLIENT_SIZE; ++i) {     // Check if user is online
            if (client[i].cli_sock != -1) {
                if (strcmp(user_id, client[i].user_id) == 0) {
                    is_online = true;
                    break;
                }
            }
        }

        if (is_online == true)
            str += sprintf(str, "\n%s is currently online", user_id);
        else
            str += sprintf(str, "\n%s is currently offline", user_id);

        my_write(client[tid].cli_sock, msg, strlen(msg));
        fclose(file);
    }
}

void update_stats(char *user_id, char *category, char *value) {
    // testing comment
    FILE *old, *new;
    char old_path[50], new_path[50];
    char line[100];
    char *str;
    sprintf(old_path, "./gameplay/%s_stat.dat", user_id);
    sprintf(new_path, "./gameplay/%s_new.dat", user_id);

    if ( (new = fopen(new_path, "a+")) == NULL) {
        my_error("Unable to open new stats file");
    }

    if ( (old = fopen(old_path, "r")) == NULL) {
        fclose(new);
        my_error("Unable to open new stats file");
    } else {
        char buf[30], buf2[30];
        while (fgets(line, 100, old) != NULL) {
            str = line;
            sscanf(line, "%s %s", buf, buf2);
            if (strcmp(buf, "Blocked:") == 0 && strcmp(category, "Blocked:") == 0) {
                bool block = true;
                char *pch;
                char *blocked_users = (char*)malloc(sizeof(char)*strlen(line)+1);
                strcpy(blocked_users, line);
                printf("blocked_users:%s\n", blocked_users);
                pch = strtok(blocked_users, " ");
                str += sprintf(str, "%s ", pch);
                while ((pch = strtok(NULL, " \n"))) {  // Check if user is to be unblocked
                    if (strcmp(pch, "none") == 0) {
                        continue;
                    }
                    if (strcmp(pch, value) != 0) {  // If on blocked list, skip user
                        str += sprintf(str, "%s ", pch);
                    } else {
                        block = false;
                    }
                }

                if (block == true)
                    str += sprintf(str, "%s\n", value);

                free(blocked_users);
            } else if (strcmp(buf, "Info:") == 0 && strcmp(category, "Info:") == 0) {
                if (strcmp(value, "") == 0)
                    str += sprintf(str, "Info: none\n");
                else
                    str += sprintf(str, "Info: %s\n", value);
            }else if (strcmp(buf, category) == 0) {
                sprintf(line, "%s %s\n", category, value);
            }
            fprintf(new, "%s", line);
        }
    }
}

void update_and_reset(int tid, int won) {
    if(won == YES)
        client[tid].games_won++;
    client[tid].games_played++;
    client[tid].game_turn = false;
    client[tid].game_on = false;
    client[tid].game_id = -1;

    char num_won[3], num_loss[3];
    sprintf(num_won, "%d", client[tid].games_won);
    sprintf(num_loss, "%d", (client[tid].games_played - client[tid].games_won));
    update_stats(client[tid].user_id, "Wins:", num_won);
    update_stats(client[tid].user_id, "Loses:", num_loss);
}

int make_a_move(int tid, char* cmd) {
    //Is it even your turn?
    if(!client[tid].game_turn) {
        char message[MSG_LENGTH];
        strcpy(message, "It's not your turn.");
        my_write(client[tid].cli_sock, message, strlen(message));
        return 0;
    }

    //Check if its a valid move
    if(cmd[0] != 'A' && cmd[0] != 'B' && cmd[0] != 'C')
        return -1;
    if(cmd[1] != '1' && cmd[1] != '2' && cmd[1] != '3')
        return -1;

    //So its a valid move
    //Was he too late?
    int game_id = client[tid].game_id;
    game* instance = &instances[game_id];
    int other_user_tid;
    if(tid == instance->player1_tid)
        other_user_tid = instance->player2_tid;
    else
        other_user_tid = instance->player1_tid;
    double seconds = (microtime() - instance->game_start) / 1000000;
    if((seconds) > client[tid].game_time_limit) {
        game_count--;
        instance->winner_tid = other_user_tid;
        print_game(instances[game_id].player1_tid, instances[game_id].player2_tid, NO, YES, YES);
        for(int count = 0; count < instances[game_id].observer_count; count++) {
            print_game(instances[game_id].observers[count], -1, NO, YES, YES);
        }
        update_and_reset(other_user_tid, YES);
        update_and_reset(tid, NO);
        return 0;
    } else {
        instance->game_start = microtime();
    }
        
    //He wasn't, so upgrade the grid
    int row = (cmd[0] == 'A') ? 0 : (cmd[0] == 'B' ? 1 : 2);
    int column = (cmd[1] == '1') ? 0 : (cmd[1] == '2' ? 1 : 2);
    instance->game_grid[row][column] = client[tid].player_type;
    instance->no_of_moves++;

    char winning_player = 'n';
    //First we check for all black win scenarios
    if(instance->game_grid[0][0] == 'b') {
        //Case 1,2,3
        if(instance->game_grid[0][1] == 'b' && instance->game_grid[0][2] == 'b')
            winning_player = 'b';
        if(instance->game_grid[1][0] == 'b' && instance->game_grid[2][0] == 'b')
            winning_player = 'b';
        if(instance->game_grid[1][1] == 'b' && instance->game_grid[2][2] == 'b')
            winning_player = 'b';
    } 
    if(instance->game_grid[1][2] == 'b') {
        //Case 4
        if(instance->game_grid[0][2] == 'b' && instance->game_grid[2][2] == 'b')
            winning_player = 'b';
        //Case 6 
        if(instance->game_grid[1][0] == 'b' && instance->game_grid[1][1] == 'b')
            winning_player = 'b';
    }
    if(instance->game_grid[2][1] == 'b') {
        //Case 5 
        if(instance->game_grid[2][0] == 'b' && instance->game_grid[2][2] == 'b')
            winning_player = 'b';
        //Case 7
        if(instance->game_grid[0][1] == 'b' && instance->game_grid[1][2] == 'b')
            winning_player = 'b';
    } 
    if(instance->game_grid[0][2] == 'b') {
        //Case 8 
        if(instance->game_grid[1][1] == 'b' && instance->game_grid[2][0] == 'b')
            winning_player = 'b';
    } 
    if(winning_player == 'n') {
        //Black didn't win, so we check for white win scenarios
        if(instance->game_grid[0][0] == 'w') {
            //Case 1,2,3
            if(instance->game_grid[0][1] == 'w' && instance->game_grid[0][2] == 'w')
                winning_player = 'w';
            if(instance->game_grid[1][0] == 'w' && instance->game_grid[2][0] == 'w')
                winning_player = 'w';
            if(instance->game_grid[1][1] == 'w' && instance->game_grid[2][2] == 'w')
                winning_player = 'w';
        }
        if(instance->game_grid[1][2] == 'w') {
            //Case 4
            if(instance->game_grid[0][2] == 'w' && instance->game_grid[2][2] == 'w')
                winning_player = 'w';
            //Case 6 
            if(instance->game_grid[1][0] == 'w' && instance->game_grid[1][1] == 'w')
                winning_player = 'w';
        }
        if(instance->game_grid[2][1] == 'w') {
            //Case 5 
            if(instance->game_grid[2][0] == 'w' && instance->game_grid[2][2] == 'w')
                winning_player = 'w';
            //Case 7
            if(instance->game_grid[0][1] == 'w' && instance->game_grid[1][2] == 'w')
                winning_player = 'w';
        } 
        if(instance->game_grid[0][2] == 'w') {
            //Case 8 
            if(instance->game_grid[1][1] == 'w' && instance->game_grid[2][0] == 'w')
                winning_player = 'w';
        } 
    } else {
        //Black won, decrement game_count & update stats
        game_count--;
        if(client[instance->player1_tid].player_type == 'b') {
            instance->winner_tid = instance->player1_tid;
            
            print_game(instances[game_id].player1_tid, instances[game_id].player2_tid, NO, YES, NO);
            for(int count = 0; count < instances[game_id].observer_count; count++) {
                print_game(instances[game_id].observers[count], -1, NO, YES, NO);
            }

            update_and_reset(instance->player1_tid, YES);
            update_and_reset(instance->player2_tid, NO);
        } else {
            instance->winner_tid = instance->player2_tid;
            
            print_game(instances[game_id].player1_tid, instances[game_id].player2_tid, NO, YES, NO);
            for(int count = 0; count < instances[game_id].observer_count; count++) {
                print_game(instances[game_id].observers[count], -1, NO, YES, NO);
            }

            update_and_reset(instance->player2_tid, YES);
            update_and_reset(instance->player1_tid, NO);
        }
        return 0;
    }
    if(winning_player == 'n') {
        //So no one won, Check if the grid is full
        for(int i = 0;i < 3;i++) {
            for(int j = 0; j< 3;j++) {
                if(instance->game_grid[i][j] == '.') {
                    winning_player = '.';
                    break;
                }
            }
        }
    } else {
        //White won, decrement game_count & update stats
        game_count--;
        if(client[instance->player1_tid].player_type == 'w') {
            instance->winner_tid = instance->player1_tid;

            print_game(instances[game_id].player1_tid, instances[game_id].player2_tid, NO, YES, NO);
            for(int count = 0; count < instances[game_id].observer_count; count++) {
                print_game(instances[game_id].observers[count], -1, NO, YES, NO);
            }

            update_and_reset(instance->player1_tid, YES);
            update_and_reset(instance->player2_tid, NO);
        } else {
            instance->winner_tid = instance->player2_tid;
            
            print_game(instances[game_id].player1_tid, instances[game_id].player2_tid, NO, YES, NO);
            for(int count = 0; count < instances[game_id].observer_count; count++) {
                print_game(instances[game_id].observers[count], -1, NO, YES, NO);
            }
            
            update_and_reset(instance->player2_tid, YES);
            update_and_reset(instance->player1_tid, NO);
        }
                
        return 0;
    }

    if(winning_player == 'n') {
        //Grid is full, decrement game_count, declare stalemate & update stats
        game_count--;
        instance->winner_tid = -1; 
        update_and_reset(instance->player1_tid, NO);
        update_and_reset(instance->player2_tid, NO);
        print_game(instances[game_id].player1_tid, instances[game_id].player2_tid, NO, YES, NO);
        for(int count = 0; count < instances[game_id].observer_count; count++) {
            print_game(instances[game_id].observers[count], -1, NO, YES, NO);
        }
    } else {
        //The game is still on, So switch turns
        client[tid].game_turn = false;
        if(tid == instance->player1_tid)
            client[instance->player2_tid].game_turn = true;
        else
            client[instance->player1_tid].game_turn = true;
        //Print to players
        print_game(instances[game_id].player1_tid, instances[game_id].player2_tid, NO, NO, NO);
        //And then print to observers
        for(int count = 0; count < instances[game_id].observer_count; count++) {
            print_game(instances[game_id].observers[count], -1, NO, NO, NO);
        }
    }

    return 0;
}
