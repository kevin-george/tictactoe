#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "game.h"
#include "server.h"
#include "client.h"
#include "utility.h"

void print_game(int player1, int player2) {
    char game_state[500];
    char *str = game_state;
    if(client[player1].player_type == 'w')
        str += sprintf(str, "\n\nBlack:\t\t%s\t\tWhite:\t\t%s"
                        , client[player2].user_id, client[player1].user_id);
    else
        str += sprintf(str, "\nBlack:\t\t%s\t\tWhite:\t\t%s"
                        , client[player1].user_id, client[player2].user_id);
    
    str += sprintf(str, "\n Time:\t%d seconds\t\t Time:\t%d seconds\n\n"
            , client[player1].game_time_limit, client[player2].game_time_limit);
    str += sprintf(str, "    1  2  3");
    str += sprintf(str, "\nA   .  .  .");
    str += sprintf(str, "\nB   .  .  .");
    str += sprintf(str, "\nC   .  .  .");
    my_write(client[player1].cli_sock, game_state, strlen(game_state));
    my_write(client[player2].cli_sock, game_state, strlen(game_state));
}

void list_games(int tid) {
  char message[MSG_LENGTH];
  char *str = message;
  for(int count = 0; count <= game_count; count++) {
    str += sprintf(str, "Game %d(%d): %s vs %s, %d moves"
        , count, count, instances[count].player1_id, instances[count].player2_id
        , instances[count].no_of_moves);
  }
  my_write(client[tid].cli_sock, message, strlen(message));
}

void start_match(int tid, char* cmd) {
    char id[USERID_LENGTH];
    char type[2];
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
                        sprintf(message, "%s invites you for a game <match %s %c>."
                                            , client[tid].user_id, client[tid].user_id, p_type);
                        my_write(client[i].cli_sock, message, strlen(message));
                    } else {
                        //I'm the invitee, so game on!
                        game_count++;
                        client[i].game_on = true;
                        //Setting player type
                        client[tid].player_type = type[0];
                        client[i].player_type = (type[0] == 'b') ? 'w' : 'b';
                        //Setting the game instance
                        client[i].game_id = client[tid].game_id = game_count;
                        //Hardcoding time limit for now
                        client[i].game_time_limit = client[tid].game_time_limit = 300;
                        //Black gets to go first
                        if(client[tid].player_type == 'b')
                            client[tid].game_turn = true;
                        else
                            client[i].game_turn = true;
                        //Setting user ids to game instance
                        strcpy(instances[game_count].player1_id, client[tid].user_id);
                        strcpy(instances[game_count].player2_id, client[i].user_id);
                        //Print game board
                        print_game(i, tid); 
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

int make_a_move(int cli_sock, char* cmd) {
  return 0;
}
