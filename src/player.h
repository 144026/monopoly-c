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


/* raw player creation/deletion, unattached */
int add_player(int idx);
int del_player(int idx);

/* delete every player, player should be attached on map */
int del_all_players(struct map *map);

struct player *get_player(int idx);

extern int g_player_nr;
extern struct player *g_cur_players[PLAYER_MAX];
extern struct player *g_next_player;

#define for_each_arr_entry(ent, arr, size) \
    for ((ent) = (arr); (ent) - (arr) < size; (ent)++)

#define for_each_player(pplayer) for_each_arr_entry(pplayer, g_cur_players, g_player_nr)

#define for_each_player_begin(player) { \
    struct player *const *_pplayer; \
    for_each_player(_pplayer) { \
        player = *_pplayer; \

#define for_each_player_end() }}


int player_render_id(struct player *player);
