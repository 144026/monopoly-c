#pragma once
#include "player.h"
#include "map.h"

#define MAX_INPUT_LEN 512
#define INPUT_BUF_SIZE (MAX_INPUT_LEN + 2)

enum game_state {
    GAME_STATE_INIT = 0,
    GAME_STATE_RUNNING,
    GAME_STATE_STOPPED,
    GAME_STAT_MAX,
};

struct game {
    enum game_state state;
    int need_dump;
    char input_buf[INPUT_BUF_SIZE];

    struct map map;
    const struct map_layout *cur_layout;

    struct player players[PLAYER_MAX];
    int cur_player_nr;
    struct player *cur_players[PLAYER_MAX];
    int next_player_seq;
    struct player *next_player;
    int default_money;
};

#define GAME_PLAYER_MIN 2
#define GAME_PLAYER_MAX 4

/* raw player creation/deletion, unattached */
int game_add_player(struct game *game, int idx);
int game_del_player(struct game *game, int idx);
/* delete every player, player should be attached on map */
int game_del_all_players(struct game *game);

struct player *game_get_player(struct game *game, int idx);


int game_init(struct game *game);
int game_event_loop(struct game *game);

enum {
    GAME_STOP_NODUMP = 0,
    GAME_STOP_DUMP,
};

void game_stop(struct game *game, int need_dump);
void game_exit(struct game *game);

