#pragma once
#include "list.h"

enum player_color {
    PLAYER_COLOR_INVALID = 0,
    PLAYER_COLOR_RED,
    PLAYER_COLOR_GREEN,
    PLAYER_COLOR_BLUE,
    PLAYER_COLOR_YELLOW,
    PLAYER_COLOR_WHITE,
    PLAYER_COLOR_MAX,
};

#define PLAYER_MAX_ITEM 10

struct asset {
    int n_money;
    int n_points;
    int n_estate;
    struct list_head estates;

    int n_block;
    int n_bomb;
    int n_robot;
};

struct buff {
    int n_empty_rounds;
    int n_god_buff;
};

struct stat {
    int n_sell_done;
};

#define PLAYER_NAME_SZ 32

struct player {
    int valid;

    int idx;
    int seq;
    const char *id;
    const char *name;
    enum player_color color;

    int pos;
    int attached;
    struct list_head pos_list;

    struct asset asset;
    struct buff buff;
    struct stat stat;
};

#define PLAYER_MAX 16

int player_init(struct player *player, int idx, int seq);
int player_uninit(struct player *player);
int player_id_to_char(struct player *player);
char player_idx_to_char(int idx);
int player_char_to_idx(int c);

const char *player_idx_to_name(int idx);

#define for_each_arr_entry(ent, arr, size) \
    for ((ent) = (arr); (ent) - (arr) < size; (ent)++)

#define for_each_player(game, pplayer) for_each_arr_entry(pplayer, (game)->cur_players, (game)->cur_player_nr)

#define for_each_player_begin(game, player) { \
    struct player *const *__pplayer; \
    for_each_player(game, __pplayer) { \
        player = *__pplayer; \

#define for_each_player_end() }}

int player_buff_countdown(struct player *player);

