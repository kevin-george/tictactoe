#ifndef _H_GAME
#define _H_GAME

#include <login.h>

typedef struct game {
	char player1_id[USERID_LENGTH];
	char player2_id[USERID_LENGTH];
	int no_of_moves;
	char game_grid[3][3];
    int observers[20];
} game;

game instances[10];
int game_count;

void list_games(int cli_sock);
void start_match(int tid, char* cmd); 
int make_a_move(int cli_sock, char* cmd);

#endif

