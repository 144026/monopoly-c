#include "common.h"
#include "game.h"
#include "player.h"
#include "ui.h"

static int game_init_map(struct game *game)
{
    game->cur_layout = &g_default_map_layout;
    return map_init(&game->map, &g_default_map_layout);
}

static void game_uninit_map(struct game *game)
{
    game->cur_layout = NULL;
    map_free(&game->map);
}


int game_add_player(struct game *game, int idx)
{
    struct player *player;

    if (idx < 0 || idx >= PLAYER_MAX)
        return -1;

    if (game->state == GAME_STATE_RUNNING) {
        game_err("game state %d does not allow add player\n", game->state);
        return -1;
    }

    player = &game->players[idx];

    if (player->valid)
        return -2;

    player_init(player, idx, game->default_money);

    game->cur_players[game->cur_player_nr] = player;
    game->cur_player_nr++;
    return 0;
}

struct player *game_get_player(struct game *game, int idx)
{
    struct player *player;

    if (idx < 0 || idx >= PLAYER_MAX)
        return NULL;

    player = &game->players[idx];
    if (!player->valid)
        return NULL;

    return player;
}

int game_rotate_player(struct game *game)
{
    int next, dead;
    struct player *player;

    if (game->state != GAME_STATE_RUNNING) {
        game_err("game state %d does not allow rotate player\n", game->state);
        return -1;
    }
    next = game->next_player_seq;
    dead = 0;

again:
    if (dead >= game->cur_player_nr) {
        game_err("no player found on map\n");
        return -2;
    }

    next += 1;
    next %= game->cur_player_nr;
    player = game->cur_players[next];

    if (!player || !player->valid) {
        game_err("current player list corrupted\n");
        return -3;
    }
    if (!player->attached) {
        dead += 1;
        goto again;
    }

    game->next_player_seq = next;
    game->next_player = player;
    return 0;
}

int game_del_player(struct game *game, int idx)
{
    struct player *player;

    if (idx < 0 || idx >= PLAYER_MAX)
        return -1;

    if (game->state == GAME_STATE_RUNNING) {
        game_err("game state %d does not allow del player\n", game->state);
        return -1;
    }

    player = &game->players[idx];

    if (!player->valid)
        return -2;
    if (player->attached)
        return -3;

    player_uninit(player);

    game->cur_player_nr--;
    game->cur_players[game->cur_player_nr] = NULL;
    return 0;
}

int game_del_all_players(struct game *game)
{
    int n = game->cur_player_nr;

    if (game->state == GAME_STATE_RUNNING) {
        game_err("game state %d does not allow del player\n", game->state);
        return -1;
    }

    while (n-- > 0) {
        map_detach_player(&game->map, game->cur_players[n]);
        if (game_del_player(game, n))
            game_err("fail to free player %d\n", n);
    }

    return 0;
}

struct player *game_find_player_by_id(struct game *game, const char *id)
{
    int i;
    struct player *player;

    if (!id)
        return NULL;

    for (i = 0; i <= game->cur_player_nr; i++) {
        player = game->cur_players[i];
        if (!player || !player->valid) {
            game_err("current player %d invalid\n", i);
            continue;
        }
        if (0 == strncmp(player->name, id, PLAYER_NAME_SZ))
            return player;
    }

    return NULL;
}


int game_init(struct game *game)
{
    memset(game, 0, sizeof(*game));
    game->state = GAME_STATE_INIT;
    game->default_money = DEFAULT_MONEY;
    return game_init_map(game);
}

int game_event_loop(struct game *game)
{
    int stop_reason = 0;
    const char *line = NULL;

    while (game->state != GAME_STATE_STOPPED) {
        map_render(&game->map);

        line = grab_line(stdin, game->input_buf, INPUT_BUF_SIZE);
        if (line)
            game_dbg("read line: %s", line);

        if (!line) {
            game_dbg("end of file, exit\n");
            game->state = GAME_STATE_STOPPED;
            stop_reason = 1;
            break;
        }

        if (game_rotate_player(game)) {
            game_dbg("rotate player fail\n");
            game->state = GAME_STATE_STOPPED;
            stop_reason = 2;
            break;
        }
    }

    return stop_reason;
}

void game_stop(struct game *game, int need_dump)
{
    game->state = GAME_STATE_STOPPED;
    game->need_dump = need_dump;
}

void game_exit(struct game *game)
{
    if (game->need_dump)
        dump_exit(game);

    game_del_all_players(game);
    game_uninit_map(game);
}

