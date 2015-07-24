#ifndef _H_GAME
#define _H_GAME

typedef struct game {
	//These store the user id of players
	int player1_tid;
	int player2_tid;
	int no_of_moves;
	//This stores the moves b/w
	char game_grid[3][3];
    //This stores the TID of observers
	int observers[20];
	int observer_count;
    int winner_tid;
    double game_start;
} game;

struct game instances[10];
//This is initalized to -1
int game_count;

void update_and_reset(int tid, int won);
void print_game(int tid1, int tid2, int first, int result, int time_out);
void list_games(int cli_sock);
void start_match(int tid, char* cmd, int argc); 
int make_a_move(int cli_sock, char* cmd);
void create_stats(char *user_id);
void print_stats(int tid, char *cmd);
void update_stats(char *user_id, char *category, char *value);
#endif

