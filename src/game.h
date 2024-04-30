#pragma once
#include "player.h"
#include "map.h"
#include "ui.h"

enum game_state {
    /* resource freed */
    GAME_STATE_UNINIT = 0,
    /* resource allocated, minial prompt */
    GAME_STATE_INIT,
    /* intermidate state, invisible to player/effects */
    GAME_STATE_START,
    GAME_STATE_RUNNING,
    /* freeze game, keep all states */
    GAME_STATE_STOPPED,
    GAME_STAT_MAX,
};

enum game_option {
    GAME_OPT_MANUAL_SKIP,
    GAME_OPT_MAX,
};

struct game_opt {
    /* user must type skip command manually when stat->empty == 1, added for bomb testing */
    const char *name;
    int on;
};

struct game_options {
    struct game_opt opts[GAME_OPT_MAX];
};

struct game {
    enum game_state state;
    int need_dump;
    struct game_options option;

    struct ui ui;

    int dice_facets;
    struct map map;
    const struct map_layout *cur_layout;

    int default_money;
    struct player players[PLAYER_MAX];

    int cur_player_nr;
    struct player *cur_players[PLAYER_MAX];

    int next_player_seq;
    struct player *next_player;
    int max_sell_per_turn;
};

#define GAME_DEFAULT_DICE_SAHPE 6

#define GAME_DEFAULT_MONEY      10000
#define GAME_DEFAULT_MONEY_MIN  1000
#define GAME_DEFAULT_MONEY_MAX  50000

#define GAME_PLAYER_MIN 2
#define GAME_PLAYER_MAX 4

#define GAME_MAX_SELL_PER_TURN 1

#define GAME_ITEM_BLOCK_RANGE 10
#define GAME_ITEM_BOMB_RANGE  10
#define GAME_ITEM_ROBOT_RANGE 10

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
void game_dump(struct game *game);
