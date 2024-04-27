#include "common.h"
#include "player.h"
#include "map.h"

int g_player_nr;
struct player *g_cur_players[PLAYER_MAX];
struct player *g_next_player;

static struct player g_players[PLAYER_MAX];

static const char *const g_player_ids[PLAYER_MAX] = {
    "Q",
    "A",
    "S",
    "J",
};

static const enum player_color g_player_colors[PLAYER_MAX] = {
    PLAYER_COLOR_RED,
    PLAYER_COLOR_GREEN,
    PLAYER_COLOR_BLUE,
    PLAYER_COLOR_YELLOW,
};

static const char *const g_player_names[PLAYER_MAX] = {
    "钱夫人",
    "阿土伯",
    "孙小美",
    "金贝贝",
};

int g_default_money = DEFAULT_MONEY;


int player_render_id(struct player *player)
{
    const char *id;

    if (!player || player->idx < 0 || player->idx > PLAYER_MAX)
        return '?';

    id = g_player_ids[player->idx];
    if (id)
        return id[0];

    game_err("player id %d too big, render not implemented\n", player->idx);
    return player->name[0];
}

static int player_add_name(struct player *player)
{
    int idx = player->idx;
    char *buf;

    if (g_player_names[idx]) {
        player->name = g_player_names[idx];
        return 0;
    }

    buf = calloc(1, PLAYER_NAME_SZ);
    if (!buf) {
        game_err("fail to alloc player %d name\n", idx);
        return -1;
    }

    snprintf(buf, PLAYER_NAME_SZ, "player%d", idx);
    buf[PLAYER_NAME_SZ - 1] = '\0';
    player->name = buf;

    return 0;
}

int player_del_name(struct player *player)
{
    int idx = player->idx;

    if (g_player_names[idx]) {
        player->name = NULL;
        return 0;
    }

    if (player->name) {
        free((void *) player->name);
        player->name = NULL;
    }
    return 0;
}

static int player_asset_init(struct asset *asset)
{
    memset(asset, 0, sizeof(*asset));
    asset->n_money = g_default_money;
    INIT_LIST_HEAD(&asset->estates);
    return 0;
}

static int player_buff_init(struct buff *buff)
{
    memset(buff, 0, sizeof(*buff));
    return 0;
}

static int player_init (struct player *player, int idx)
{
    player->idx = idx;
    player_add_name(player);
    player->color = g_player_colors[idx];

    player->pos = 0;
    player->attached = 0;
    INIT_LIST_HEAD(&player->pos_list);

    player_asset_init(&player->asset);
    player_buff_init(&player->buff);

    player->valid = 1;
    return 0;
}


int add_player(int idx)
{
    struct player *player;

    if (idx < 0 || idx >= PLAYER_MAX)
        return -1;

    player = &g_players[idx];

    if (player->valid)
        return -2;

    player_init(player, idx);

    g_cur_players[g_player_nr++] = player;
    return 0;
}

struct player *get_player(int idx)
{
    struct player *player;

    if (idx < 0 || idx >= PLAYER_MAX)
        return NULL;

    player = &g_players[idx];
    if (!player->valid)
        return NULL;

    return player;
}

int del_player(int idx)
{
    struct player *player;

    if (idx < 0 || idx >= PLAYER_MAX)
        return -1;

    player = &g_players[idx];

    if (!player->valid)
        return -2;
    if (player->attached)
        return -3;

    player_del_name(player);
    player->valid = 0;

    g_player_nr--;
    g_cur_players[g_player_nr] = NULL;
    return 0;
}

int del_all_players(struct map *map)
{
    int n = g_player_nr;

    while (n-- > 0) {
        map_detach_player(map, g_cur_players[n]);
        if (del_player(n))
            game_err("fail to free player %d\n", n);
    }

    return 0;
}

struct player *find_player_by_id(const char *id)
{
    int i;
    struct player *player;

    if (!id)
        return NULL;

    for (i = 0; i <= g_player_nr; i++) {
        player = g_cur_players[i];
        if (!player || !player->valid) {
            game_err("current player %d invalid\n", i);
            continue;
        }
        if (0 == strncmp(player->name, id, PLAYER_NAME_SZ))
            return player;
    }

    return NULL;
}

