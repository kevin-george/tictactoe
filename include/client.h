#ifndef _H_CLIENT
#define _H_CLIENT

#include <stdbool.h>

#include "login.h"

#define CMD_LENGTH 100
#define MSG_LENGTH 500

typedef struct client_t{
	int cli_sock;
	int tid;
	bool is_quiet;
	char user_id[USERID_LENGTH];
    //Game related vars
    bool game_on;
    char player_type;
    int game_id;
    double game_time_limit;
    bool game_turn;
    int games_played;
    int games_won;
    bool is_observing;
    int observe_match_num;
} client_t;

// pThread

void close_client(int tid);
void *start_client(void *arg);
void reset_client(int tid);
bool check_online_status(int tid, char *user_id);
bool is_blocked(int tid, char *user_id);

#endif
