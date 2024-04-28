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

    player_init(player, idx, game->cur_player_nr);
    player->asset.n_money = game->default_money;

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

struct player *game_get_player_by_id(struct game *game, const char *id)
{
    struct player *player;

    for_each_player_begin(game, player) {
        if (0 == strcmp(player->id, id))
            return player;
    } for_each_player_end()
    return NULL;
}

int game_rotate_player(struct game *game)
{
    int next, dead;
    struct player *player;

    if (game->state != GAME_STATE_RUNNING)
        return 0;

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

int game_set_next_player(struct game *game, struct player *player)
{
    if (!player->valid || !player->attached)
        return -1;

    if (player->seq < 0 || player->seq >= game->cur_player_nr)
        return -2;

    game->next_player = player;
    game->next_player_seq = player->seq;
    return 0;
}

int game_del_player(struct game *game, int idx)
{
    struct player *player;

    if (idx < 0 || idx >= PLAYER_MAX) {
        game_err("player idx %d out of range\n", idx);
        return -1;
    }

    if (game->state == GAME_STATE_RUNNING) {
        game_err("game state %d does not allow del player\n", game->state);
        return -1;
    }

    player = &game->players[idx];

    if (!player->valid) {
        game_err("player %d already valid\n", idx);
        return -2;
    }

    if (player->attached) {
        game_err("player %d already attached on map\n", idx);
        return -3;
    }

    player_uninit(player);

    game->cur_player_nr--;
    game->cur_players[game->cur_player_nr] = NULL;
    return 0;
}

int game_del_all_players(struct game *game)
{
    int n = game->cur_player_nr;
    int ret;

    if (game->state == GAME_STATE_RUNNING) {
        game_err("game state %d does not allow del player\n", game->state);
        return -1;
    }

    while (n-- > 0) {
        struct player *player = game->cur_players[n];

        ret = map_detach_player(&game->map, player);
        if (ret)
            game_err("fail to detach player %d, ret %d\n", n, ret);

        ret = game_del_player(game, player->idx);
        if (ret)
            game_err("fail to free player %d, ret %d\n", n, ret);
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

void game_uninit(struct game *game)
{
    game_del_all_players(game);
    game_uninit_map(game);
}


int game_before_action(struct game *game)
{
    if (game->state != GAME_STATE_RUNNING)
        return 0;

    player_buff_countdown(game->next_player);
    return 0;
}

int game_after_action(struct game *game)
{
    if (game->state != GAME_STATE_RUNNING)
        return 0;

    return 0;
}

#ifdef GAME_DEBUG
static inline void game_debug_show_cmd(int argc, const char *argv[])
{
    int i;

    for (i = 0; i < argc; i++)
        game_dbg("argv[%d]: %s\n", i, argv[i]);
}
#else
static inline void game_debug_show_cmd(int argc, const char *argv[])
{
}
#endif

static int game_cmd_preset_user(struct game *game, int argc, const char *argv[])
{
    int i;
    int n_id;
    int idxs[PLAYER_MAX];

    if (argc != 3)
        return -1;

    n_id = strlen(argv[2]);

    if (n_id < GAME_PLAYER_MIN || n_id > GAME_PLAYER_MAX) {
        game_err("preset user number %d out of range\n", n_id);
        return -1;
    }

    for (i = 0; i < n_id; i++) {
        char char_id = argv[2][i];
        int idx = player_char_to_idx(char_id);

        if (idx < 0 || idx >= PLAYER_MAX) {
            game_err("preset user id %c(%#x) unkown\n", char_id, char_id);
            return -1;
        }
        idxs[i] = idx;
    }

    game_stop(game, GAME_STOP_NODUMP);
    game_uninit(game);

    if (game_init(game)) {
        game_err("preset game init fail\n");
        return -2;
    }

    for (i = 0; i < n_id; i++) {
        if (game_add_player(game, idxs[i])) {
            game_err("preset add player idxs[%d] = %d fail\n", i, idxs[i]);
            return -3;
        }
        if (map_attach_player(&game->map, game->cur_players[i])) {
            game_err("preset attach player idxs[%d] = %d to map fail\n", i, idxs[i]);
            return -4;
        }
    }

    game->next_player_seq = 0;
    game->next_player = game->cur_players[0];
    game->state = GAME_STATE_RUNNING;

    return 0;
}

static int game_cmd_preset_map(struct game *game, int argc, const char *argv[])
{
    struct player *player;
    int pos, lv;
    char *endptr;

    if (argc != 5 || !argv[2] || !argv[3] || !argv[4])
        return -1;

    endptr = NULL;
    pos = strtol(argv[2], &endptr, 10);
    if (*endptr) {
        game_err("not a valid number: %s\n", argv[2]);
        return -1;
    }

    player = game_get_player_by_id(game, argv[3]);
    if (!player) {
        game_err("fail to find user with id: %s\n", argv[3]);
        return -1;
    }

    endptr = NULL;
    lv = strtol(argv[4], &endptr, 10);
    if (*endptr) {
        game_err("not a valid number: %s\n", argv[4]);
        return -1;
    }

    if (lv < ESTATE_WASTELAND || lv >= ESTATE_MAX)
        return -1;

    if (map_set_owner(&game->map, pos, player))
        return -1;

    game->map.nodes[pos].level = lv;
    return 0;
}

static int game_cmd_preset_asset(struct game *game, int argc, const char *argv[])
{
    struct player *player;
    int num;
    char *endptr;

    if (argc != 4 || !argv[2] || !argv[3])
        return -1;

    player = game_get_player_by_id(game, argv[2]);
    if (!player) {
        game_err("fail to find user with id: %s\n", argv[2]);
        return -1;
    }

    endptr = NULL;
    num = strtol(argv[3], &endptr, 10);
    if (*endptr) {
        game_err("not a valid number: %s\n", argv[3]);
        return -1;
    }

    if (num < 0)
        return -1;

    if (!strcmp(argv[1], "fund"))
        player->asset.n_money = num;
    else if (!strcmp(argv[1], "credit"))
        player->asset.n_points = num;
    else
        return -1;

    return 0;
}

static int game_cmd_preset_gift(struct game *game, int argc, const char *argv[])
{
    struct player *player;
    int num;
    char *endptr;

    if (argc != 5 || !argv[2] || !argv[3] || !argv[4])
        return -1;

    player = game_get_player_by_id(game, argv[2]);
    if (!player) {
        game_err("fail to find user with id: %s\n", argv[2]);
        return -1;
    }

    endptr = NULL;
    num = strtol(argv[4], &endptr, 10);
    if (*endptr) {
        game_err("not a valid number: %s\n", argv[4]);
        return -1;
    }

    if (num < 0)
        return -1;

    if (!strcmp(argv[3], "barrier")) {
        if (player->asset.n_robot + num > PLAYER_MAX_ITEM)
            return -1;
        player->asset.n_block = num;

    } else if (!strcmp(argv[3], "robot")) {
        if (player->asset.n_block + num > PLAYER_MAX_ITEM)
            return -1;
        player->asset.n_robot = num;

    } else if (!strcmp(argv[3], "god"))  {
        player->buff.n_god_buff = num;
    } else {
        return -1;
    }

    return 0;
}

static int game_cmd_preset_userloc(struct game *game, int argc, const char *argv[])
{
    struct player *player;
    int pos, empty_round;
    char *endptr;

    if (argc != 5 || !argv[2] || !argv[3] || !argv[4])
        return -1;

    player = game_get_player_by_id(game, argv[2]);
    if (!player) {
        game_err("fail to find user with id: %s\n", argv[2]);
        return -1;
    }

    endptr = NULL;
    pos = strtol(argv[3], &endptr, 10);
    if (*endptr) {
        game_err("not a valid number: %s\n", argv[3]);
        return -1;
    }

    endptr = NULL;
    empty_round = strtol(argv[4], &endptr, 10);
    if (*endptr) {
        game_err("not a valid number: %s\n", argv[4]);
        return -1;
    }

    if (pos < 0 || pos >= game->map.n_used || empty_round < 0)
        return -1;

    map_move_player(&game->map, player, pos);
    player->buff.n_empty_rounds = empty_round;
    return 0;
}

static int game_cmd_preset_nextuser(struct game *game, int argc, const char *argv[])
{
    int idx;
    int char_id;
    struct player *player;

    if (argc != 2)
        return -1;

    char_id = argv[1][0];
    if (!isprint(char_id))
        return -1;

    player = game_get_player_by_id(game, argv[1]);
    if (!player) {
        game_err("fail to find user with id: %s\n", argv[1]);
        return -1;
    }

    return game_set_next_player(game, player);
}

static int game_cmd_preset_barrier(struct game *game, int argc, const char *argv[])
{
    struct map *map = &game->map;
    int pos;
    char *endptr;

    if (argc != 3 || !argv[2])
        return -1;

    endptr = NULL;
    pos = strtol(argv[2], &endptr, 10);
    if (*endptr) {
        game_err("not a valid number: %s\n", argv[2]);
        return -1;
    }

    return map_place_item(&game->map, pos, ITEM_BLOCK);
}


static int game_cmd_preset(struct game *game, int argc, const char *argv[])
{
    int i;
    const char *subcmd;

    if (argc < 2)
        return -1;

    subcmd = argv[1];

    if (!strcmp(subcmd, "user")) {
        return game_cmd_preset_user(game, argc, argv);

    } else if (!strcmp(subcmd, "map")) {
        return game_cmd_preset_map(game, argc, argv);

    } else if (!strcmp(subcmd, "fund") || !strcmp(subcmd, "credit")) {
        return game_cmd_preset_asset(game, argc, argv);

    } else if (!strcmp(subcmd, "gift")) {
        return game_cmd_preset_gift(game, argc, argv);

    } else if (!strcmp(subcmd, "userloc")) {
        return game_cmd_preset_userloc(game, argc, argv);

    } else if (!strcmp(subcmd, "nextuser")) {
        return game_cmd_preset_nextuser(game, argc, argv);

    } else if (!strcmp(subcmd, "barrier")) {
        return game_cmd_preset_barrier(game, argc, argv);
    }
    return -1;
}

#define GAME_CMD_MAX_ARGC 16

/* @return: < 0 err, == 0 good, > 0 action performed */
static int game_handle_command(struct game *game, char *line)
{
    int argc;
    const char *cmd;
    const char *argv[GAME_CMD_MAX_ARGC];

    argc = ui_cmd_tokenize(line, argv, GAME_CMD_MAX_ARGC);
    if (argc <= 0)
        return -1;

    game_debug_show_cmd(argc, argv);

    /* dispatch cmd */
    cmd = argv[0];
    if (!strcmp(cmd, "quit")) {
        game_stop(game, GAME_STOP_NODUMP);
        return 0;
    } else if (!strcmp(cmd, "dump")) {
        game_stop(game, GAME_STOP_DUMP);
        return 0;
    } else if (!strcmp(cmd, "preset")) {
        return game_cmd_preset(game, argc, argv);
    }

    game_err("cmd '%s' unknown\n", cmd);
    return -1;
}

int game_event_loop(struct game *game)
{
    int stop_reason = 0;
    int cmd_ret;
    char *line = NULL;

    /* TODO: make sure on first enter, next_player is valid */
    while (game->state != GAME_STATE_STOPPED) {
        ui_map_render(&game->map);

        /* TODO: check winning here */

        if (game_before_action(game)) {
            goto skip_action;
        }

        ui_game_prompt(game);

        /* wait user input event */
        line = ui_read_line(stdin, game->input_buf, INPUT_BUF_SIZE);
        if (line)
            game_dbg("read line: %s", line);

        if (!line) {
            game_dbg("end of file, exit\n");
            game_stop(game, GAME_STOP_NODUMP);
            stop_reason = 1;
            break;
        }

        cmd_ret = game_handle_command(game, line);

skip_action:
        game_after_action(game);

        if (cmd_ret <= 0)
            continue;

        if (game_rotate_player(game)) {
            game_dbg("rotate player fail\n");
            game_stop(game, GAME_STOP_NODUMP);
            stop_reason = 2;
            break;
        }
    }

    return stop_reason;
}

void game_stop(struct game *game, int need_dump)
{
    game->state = GAME_STATE_STOPPED;
#ifdef GAME_DEBUG
    game->need_dump = 1;
#else
    game->need_dump = need_dump;
#endif
}

void game_exit(struct game *game)
{
    if (game->need_dump)
        dump_exit(game);

    game_uninit(game);
}

