#pragma once
#include "list.h"
#include "map.h"

enum player_color {
    PLAYER_COLOR_INVALID = 0,
    PLAYER_COLOR_RED,
    PLAYER_COLOR_GREEN,
    PLAYER_COLOR_BLUE,
    PLAYER_COLOR_YELLOW,
    PLAYER_COLOR_WHITE,
    PLAYER_COLOR_MAX,
};

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
    int b_sell_estate;
    int b_god_buff;
};

#define PLAYER_NAME_SZ 32

struct player {
    int valid;

    int idx;
    const char *name;
    enum player_color color;

    int pos;
    int attached;
    struct list_head pos_list;

    struct asset asset;
    struct buff buff;
};

#define PLAYER_MAX 16
#define DEFAULT_MONEY 10000

int player_init(struct player *player, int idx, int n_money);
int player_uninit(struct player *player);
int player_id_to_char(struct player *player);

#define for_each_arr_entry(ent, arr, size) \
    for ((ent) = (arr); (ent) - (arr) < size; (ent)++)

#define for_each_player(game, pplayer) for_each_arr_entry(pplayer, (game)->cur_players, (game)->cur_player_nr)

#define for_each_player_begin(game, player) { \
    struct player *const *__pplayer; \
    for_each_player(game, __pplayer) { \
        player = *__pplayer; \

#define for_each_player_end() }}

int player_buff_countdown(struct player *player);

