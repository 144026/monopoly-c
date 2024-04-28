#include "common.h"
#include "player.h"
#include "map.h"

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


int player_id_to_char(struct player *player)
{
    const char *id;

    if (!player || !player->valid)
        return '?';

    if (player->idx < 0 || player->idx > PLAYER_MAX)
        return '?';

    if (!player->id || !player->id[0])
        return '?';

    if (!g_player_ids[player->idx])
        game_err("player idx %d too big, short name id not implemented\n", player->idx);

    return player->id[0];
}

int player_char_to_idx(int c) {
    int i;
    const char *id;

    if (c <= 0 || !isprint(c))
        return -1;

    for (i = 0; i < PLAYER_MAX; i++) {
        id = g_player_ids[i];

        if (id && c == id[0])
            return i;
    }

    return -1;
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
    INIT_LIST_HEAD(&asset->estates);
    return 0;
}

static int player_buff_init(struct buff *buff)
{
    memset(buff, 0, sizeof(*buff));
    return 0;
}

int player_init(struct player *player, int idx, int seq)
{
    player->idx = idx;
    player->seq = seq;
    player->id = g_player_ids[idx];
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

int player_uninit (struct player *player)
{
    player->valid = 0;
    player->id = NULL;
    return player_del_name(player);
}


int player_buff_countdown(struct player *player)
{
    struct buff *buff = &player->buff;

    if (buff->n_god_buff > 0)
        buff->n_god_buff--;

    if (buff->n_empty_rounds > 0)
        buff->n_empty_rounds--;
    return 0;
}
