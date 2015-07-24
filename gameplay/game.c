#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "game.h"
#include "server.h"
#include "client.h"
#include "utility.h"

#define YES 1
#define NO 0

void print_game(int player1, int player2, int first_view, int result) {
    char game_state[500] = "";
    char *str = game_state;
    //Only first time view gets this
    if(first_view == YES) {
        if(client[player1].player_type == 'w')
            str += sprintf(str, "\n\nBlack:\t\t%s\t\tWhite:\t\t%s"
                    , client[player2].user_id, client[player1].user_id);
        else
            str += sprintf(str, "\nBlack:\t\t%s\t\tWhite:\t\t%s"
                    , client[player1].user_id, client[player2].user_id);
        str += sprintf(str, "\n Time:\t%d seconds\t\t Time:\t%d seconds\n\n"
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
        if(instance->winner_tid != -1)
            str += sprintf(str, "%s won the game",client[instance->winner_tid].user_id);
        else
            str += sprintf(str, "The game was a tie");
    }

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
        str += sprintf(str, "Game %d(%d): %s vs %s, %d moves"
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
                        client[i].game_time_limit = client[tid].game_time_limit = 300;
                        //Black gets to go first
                        if(client[tid].player_type == 'b')
                            client[tid].game_turn = true;
                        else
                            client[i].game_turn = true;
                        //Setting user ids to game instance
                        instances[game_count].player1_tid = tid;
                        instances[game_count].player2_tid = i;
                        //Print game board
                        print_game(i, tid, YES, NO); 
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

void print_stats(int tid) {

}

void update_and_reset(int tid, int won) {
    if(won == YES)
        client[tid].games_won++;
    client[tid].games_played++;
    client[tid].game_turn = false;
    client[tid].game_on = false;
    client[tid].game_id = -1;
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

    //So its a valid move, first upgrade the grid
    int game_id = client[tid].game_id;
    game* instance = &instances[game_id];
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
            update_and_reset(instance->player1_tid, YES);
            update_and_reset(instance->player2_tid, NO);
        } else {
            instance->winner_tid = instance->player2_tid;
            update_and_reset(instance->player2_tid, YES);
            update_and_reset(instance->player1_tid, NO);
        }
        print_game(instances[game_id].player1_tid, instances[game_id].player2_tid, NO, YES);
        for(int count = 0; count < instances[game_id].observer_count; count++) {
            print_game(instances[game_id].observers[count], -1, NO, YES);
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
            update_and_reset(instance->player1_tid, YES);
            update_and_reset(instance->player2_tid, NO);
        } else {
            instance->winner_tid = instance->player2_tid;
            update_and_reset(instance->player2_tid, YES);
            update_and_reset(instance->player1_tid, NO);
        }
        print_game(instances[game_id].player1_tid, instances[game_id].player2_tid, NO, YES);
        for(int count = 0; count < instances[game_id].observer_count; count++) {
            print_game(instances[game_id].observers[count], -1, NO, YES);
        }
        
        return 0;
    }

    if(winning_player == 'n') {
        //Grid is full, decrement game_count, declare stalemate & update stats
        game_count--;
        instance->winner_tid = -1; 
        update_and_reset(instance->player1_tid, NO);
        update_and_reset(instance->player2_tid, NO);
        print_game(instances[game_id].player1_tid, instances[game_id].player2_tid, NO, YES);
        for(int count = 0; count < instances[game_id].observer_count; count++) {
            print_game(instances[game_id].observers[count], -1, NO, YES);
        }
    } else {
        //The game is still on, So switch turns
        client[tid].game_turn = false;
        if(tid == instance->player1_tid)
            client[instance->player2_tid].game_turn = true;
        else
            client[instance->player1_tid].game_turn = true;
        //Print to players
        print_game(instances[game_id].player1_tid, instances[game_id].player2_tid, NO, NO);
        //And then print to observers
        for(int count = 0; count < instances[game_id].observer_count; count++) {
            print_game(instances[game_id].observers[count], -1, NO, NO);
        }
    }

    return 0;
}
