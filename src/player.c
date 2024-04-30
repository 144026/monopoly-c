#include "common.h"
#include "player.h"
#include "map.h"
#include "game.h"

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
    "Qianfuren",
    "Atubo",
    "Sunxiaomei",
    "Jinbeibei",
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

char player_idx_to_char(int idx)
{
    if (idx < 0 || idx >= PLAYER_MAX)
        return 0;

    if (g_player_ids[idx])
        return g_player_ids[idx][0];

    return 0;
}

const char *player_idx_to_name(int idx)
{
    if (idx < 0 || idx >= PLAYER_MAX)
        return NULL;

    if (g_player_names[idx])
        return g_player_names[idx];

    return NULL;
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

static int player_stat_init(struct stat *stat)
{
    memset(stat, 0, sizeof(*stat));
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
    player_stat_init(&player->stat);

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

int player_grant_gift_money(struct gift_info *gift, struct game *game, struct player *player)
{
    struct ui *ui = &game->ui;

    player->asset.n_money += gift->value;
    fprintf(ui->out, "[GIFT] Acquired bonus cash %d.\n", gift->value);
    return 0;
}

int player_grant_gift_point(struct gift_info *gift, struct game *game, struct player *player)
{
    struct ui *ui = &game->ui;

    player->asset.n_points += gift->value;
    fprintf(ui->out, "[GIFT] Acquired bonus points %d.\n", gift->value);
    return 0;
}

int player_grant_gift_god(struct gift_info *gift, struct game *game, struct player *player)
{
    struct ui *ui = &game->ui;

    player->buff.n_god_buff += gift->value;
    fprintf(ui->out, "[GIFT] God of Wealth be with you in %d rounds.\n", gift->value);
    return 0;
}
