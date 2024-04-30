#include "common.h"
#include "game.h"
#include "player.h"
#include "ui.h"

static int game_init_map(struct game *game)
{
    game->cur_layout = g_default_map_layout;
    return map_init(&game->map, g_default_map_layout);
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

    if (game->state == GAME_STATE_RUNNING || game->state == GAME_STATE_UNINIT) {
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

int game_add_players(struct game *game, int idxs[], int n_idx)
{
    int i, idx;
    int last_player_nr = game->cur_player_nr;

    if (game->state == GAME_STATE_RUNNING || game->state == GAME_STATE_UNINIT) {
        game_err("game state %d does not allow add player\n", game->state);
        return -1;
    }

    for (i = 0; i < n_idx; i++) {
        if (idxs[i] < 0 || idxs[i] >= PLAYER_MAX)
            return -2;

        if (game_add_player(game, idxs[i])) {
            game_err("add player idxs[%d] = %d fail\n", i, idxs[i]);
            return -3;
        }
        if (map_attach_player(&game->map, &game->players[idxs[i]])) {
            game_err("preset attach player idxs[%d] = %d to map fail\n", i, idxs[i]);
            return -4;
        }
    }

    if (last_player_nr == 0 && game->cur_player_nr > 0) {
        game->next_player_seq = 0;
        game->next_player = game->cur_players[0];
    }
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
        game->next_player_seq = 0;
        game->next_player = NULL;
        game_dbg("no player left on map\n");
        return 0;
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

    if (game->state == GAME_STATE_RUNNING || game->state == GAME_STATE_UNINIT) {
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

    if (game->state == GAME_STATE_RUNNING || game->state == GAME_STATE_UNINIT) {
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
    game->default_money = GAME_DEFAULT_MONEY;
    game->max_sell_per_turn = GAME_MAX_SELL_PER_TURN;

    if (ui_init(&game->ui))
        goto err;

    game->dice_facets = GAME_DEFAULT_DICE_SAHPE;
    if (game_init_map(game))
        goto err_ui;

    srand(time(NULL));
    game->state = GAME_STATE_INIT;
    return 0;

err_ui:
    ui_uninit(&game->ui);
err:
    game->state = GAME_STATE_UNINIT;
    return -1;
}

void game_uninit(struct game *game)
{
    game_del_all_players(game);
    game_uninit_map(game);
    ui_uninit(&game->ui);
    game->state = GAME_STATE_UNINIT;
}


static int game_prompt_action(struct game *game)
{
    struct ui *ui = &game->ui;

    if (game->state == GAME_STATE_RUNNING)
        ui_prompt_player_name(ui, game->next_player);
    else
        fprintf(ui->out, "enter 'start' to play");

    fprintf(ui->out, "> ");
    return 0;
}

int game_before_action(struct game *game)
{
    struct player *player = game->next_player;

    if (game->state != GAME_STATE_RUNNING)
        return 0;

    player_buff_apply(player);

    if (player->stat.empty)
        return 1;
    return 0;
}

static int game_prompt_buy(struct game *game, struct player *player, struct map_node *node)
{
    struct ui *ui = &game->ui;
    int ret, buy;
    const char *prompt;

    assert(node->type == MAP_NODE_VACANCY);

    if (player->asset.n_money < node->estate.price)
        return 0;

    prompt = ui_fmt(ui, "[BUY] Pay %d to buy this estate?", node->estate.price);
    while (1) {
        ret = ui_input_bool_prompt(ui, prompt, &buy);
        if (ret < 0)
            goto out_stop;
        if (ret > 0)
            break;
    }

    if (!buy)
        return 0;

    player->asset.n_money -= node->estate.price;
    node->estate.owner = player;
    list_add_tail(&node->estate.estates_list, &player->asset.estates);

    fprintf(ui->out, "[BUY] Bought estate at position %d.\n", player->pos);
    return 0;

out_stop:
    game_stop(game, GAME_STATE_STOPPED);
    return -1;
}

static int game_prompt_upgrade(struct game *game, struct player *player, struct map_node *node)
{
    struct ui *ui = &game->ui;
    int ret, up;
    const char *prompt;

    assert(node->type == MAP_NODE_VACANCY);

    if (node->estate.level >= ESTATE_SKYSCRAPER)
        return 0;
    if (player->asset.n_money < node->estate.price)
        return 0;

    prompt = ui_fmt(ui, "[UPGRADE] Pay %d to upgrade this estate?", node->estate.price);
    while (1) {
        ret = ui_input_bool_prompt(ui, prompt, &up);
        if (ret < 0)
            goto out_stop;
        if (ret > 0)
            break;
    }

    if (!up)
        return 0;

    player->asset.n_money -= node->estate.price;
    node->estate.level += 1;

    fprintf(ui->out, "[UPGRADE] Upgraded estate at position %d to level %d.\n", player->pos, node->estate.level);
    return 0;

out_stop:
    game_stop(game, GAME_STATE_STOPPED);
    return -1;
}

static int game_player_pay_toll(struct game *game, struct player *player, struct map_node *node)
{
    struct ui *ui = &game->ui;
    int price = map_node_price(node);

    assert(node->type == MAP_NODE_VACANCY);

    fprintf(ui->out, "[TOLL] Need to pay %d.\n", price);
    if (player->stat.god) {
        fprintf(ui->out, "[GOD] God of wealth helped you out.\n");
        return 0;
    }

    player->asset.n_money -= price;
    if (player->asset.n_money < 0) {
        fprintf(ui->out, "[BANKRUPT] Went backrupt, debt %d.\n", player->asset.n_money);
        return 0;
    }

    node->estate.owner->asset.n_money += price;
    return 0;
}

static int game_prompt_item_house(struct game *game, struct player *player, struct map_node *node)
{
    struct ui *ui = &game->ui;
    struct asset *asset = &player->asset;
    struct item_house *house = &node->item_house;
    int i, j, ret;
    struct choice choices[ITEM_MAX + 1] = {};
    struct select sel;
    const char *prompt;
    struct item_info *chosen;

    assert(node->type == MAP_NODE_ITEM_HOUSE);

    if (asset->n_bomb + asset->n_robot + asset->n_block >= PLAYER_MAX_ITEM) {
        fprintf(ui->out, "[ITEM HOUSE] Inventory full, can't buy new item.\n");
        return 0;
    }
    if (asset->n_points < node->item_house.min_price) {
        fprintf(ui->out, "[ITEM HOUSE] Player points %d not enough, exit from item house.\n", asset->n_points);
        return 0;
    }

    /* build choices */
    for (i = 0, j = 0; i < ITEM_MAX; i++) {
        if (!house->items.info[i].on_sell)
            continue;
        choices[i].name = ui_item_name(i);
        choices[i].id = '1' + j;
        j++;
    }

    choices[ITEM_MAX].name = "Exit from item house";
    choices[ITEM_MAX].id = 'f';

    sel.n_selected = 0;
    sel.n_choice = ARRAY_SIZE(choices);
    sel.choices = choices;
    prompt = ui_fmt(ui, "[ITEM HOUSE] Welcome, what item do you what?\n");
    while (1) {
        ret = ui_selection_menu_prompt(ui, prompt, &sel);
        if (ret < 0)
            goto out_stop;
        if (ret == 0)
            continue;

        if (sel.cur_choice == ITEM_MAX) {
            fprintf(ui->out, "[ITEM HOUSE] Exit from item house.\n");
            return 0;
        }

        /* infinite goods supply */
        chosen = &house->items.info[sel.cur_choice];
        choices[sel.cur_choice].chosen = 0;

        if (asset->n_points < chosen->price) {
            fprintf(ui->out, "[ITEM HOUSE] Player points %d not enough, need %d to by '%s'.\n",
                    asset->n_points, chosen->price, choices[sel.cur_choice].name);
            continue;
        }

        asset->n_points -= chosen->price;
        fprintf(ui->out, "[ITEM HOUSE] Bought '%s', payed %d points.\n", choices[sel.cur_choice].name, chosen->price);
        /* FIXME: change struct asset to be be fully generic */
        if (sel.cur_choice == ITEM_BLOCK)
            asset->n_block++;
        else if (sel.cur_choice == ITEM_ROBOT)
            asset->n_robot++;

        /* check again */
        if (asset->n_bomb + asset->n_robot + asset->n_block >= PLAYER_MAX_ITEM) {
            fprintf(ui->out, "[ITEM HOUSE] Inventory full, can't buy new item.\n");
            return 0;
        }
        if (asset->n_points < node->item_house.min_price) {
            fprintf(ui->out, "[ITEM HOUSE] Player points %d not enough, exit from item house.\n", asset->n_points);
            return 0;
        }
    }

    return 0;

out_stop:
    game_stop(game, GAME_STATE_STOPPED);
    return -1;
}

static int game_prompt_gift_house(struct game *game, struct player *player, struct map_node *node)
{
    struct ui *ui = &game->ui;
    struct gift_house *house = &node->gift_house;
    int i, j, ret;
    struct choice choices[GIFT_MAX] = {};
    struct select sel;
    const char *prompt;
    struct gift_info *chosen;

    assert(node->type == MAP_NODE_GIFT_HOUSE);

    /* build choices */
    for (i = 0, j = 0; i < ITEM_MAX && i < house->n_gifts; i++) {
        choices[i].name = house->gifts[i].name;
        choices[i].id = '1' + j;
        j++;
    }

    sel.n_selected = 0;
    sel.n_choice = ARRAY_SIZE(choices);
    sel.choices = choices;
    prompt = ui_fmt(ui, "[GIFT HOUSE] Welcome, what gift do you what?\n");

    /* player only has one chance to choose gift */
    ret = ui_selection_menu_prompt(ui, prompt, &sel);
    if (ret < 0)
        goto out_stop;

    if (ret == 0) {
        fprintf(ui->out, "[GIFT HOUSE] Choice is invalid, exit from gift house.\n");
    } else {
        chosen = &house->gifts[sel.cur_choice];
        chosen->grant(chosen, game, player);
    }

    return 0;

out_stop:
    game_stop(game, GAME_STATE_STOPPED);
    return -1;
}

static int game_map_after_action(struct game *game)
{
    struct map *map = &game->map;
    struct player *player = game->next_player;
    struct map_node *node = &map->nodes[player->pos];

    switch (node->type) {
    case MAP_NODE_VACANCY:
        if (!node->estate.owner)
            return game_prompt_buy(game, player, node);

        if (node->estate.owner == player)
            return game_prompt_upgrade(game, player, node);
        return game_player_pay_toll(game, player, node);

    case MAP_NODE_ITEM_HOUSE:
        return game_prompt_item_house(game, player, node);
    case MAP_NODE_GIFT_HOUSE:
        return game_prompt_gift_house(game, player, node);
    case MAP_NODE_MAGIC_HOUSE:
        break;

    case MAP_NODE_PRISON:
        player->buff.n_empty_rounds = 2;
        fprintf(game->ui.out, "[PRISON] Caught and handcuffed.\n");
        break;

    case MAP_NODE_MINE:
        player->asset.n_points += node->mine_points;
        fprintf(game->ui.out, "[MINE] Got %d points.\n", node->mine_points);
        break;

    case MAP_NODE_PARK:
        fprintf(game->ui.out, "[PARK] Enjoy yourself.\n");
        break;
    default:
        break;
    }

    return 0;
}

static int game_player_after_action(struct game *game)
{
    struct map *map = &game->map;
    struct player *player = game->next_player;

    player->stat.n_sell_done = 0;
    player_buff_wearoff(game->next_player);
    return 0;
}

int game_after_action(struct game *game)
{
    if (game->state != GAME_STATE_RUNNING)
        return 0;

    game_map_after_action(game);
    game_player_after_action(game);

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

    if (game_add_players(game, idxs, n_id)) {
        game_err("preset add players fail\n");
        game_del_all_players(game);
        return -3;
    }

    game->state = GAME_STATE_START;
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

    assert(game->map.nodes[pos].type == MAP_NODE_VACANCY);
    game->map.nodes[pos].estate.level = lv;
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
        player->buff.n_god_rounds = num;
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

    player->buff.n_empty_rounds = empty_round;
    return map_move_player(&game->map, player, pos);
}

static int game_cmd_preset_nextuser(struct game *game, int argc, const char *argv[])
{
    int idx;
    int char_id;
    struct player *player;

    if (argc != 3)
        return -1;

    char_id = argv[2][0];
    if (!isprint(char_id))
        return -1;

    player = game_get_player_by_id(game, argv[2]);
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

    return map_place_item(&game->map, pos, ITEM_BLOCK, NULL);
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

static int game_cmd_start(struct game *game)
{
    struct ui *ui = &game->ui;
    const char *prompt;
    struct range range;
    int ret, i, seq;
    int  n_players;
    int idxs[GAME_PLAYER_MAX];
    struct choice choices[GAME_PLAYER_MAX] = {};
    struct select sel;

    range.begin = GAME_DEFAULT_MONEY_MIN;
    range.end = GAME_DEFAULT_MONEY_MAX;
    prompt = ui_fmt(ui, "select initial money (%ld-%ld): ", range.begin, range.end);
    while (1) {
        ret = ui_input_int_prompt(ui, prompt, &range, &game->default_money);
        if (ret < 0)
            goto out_stop;
        if (ret > 0)
            break;
    }

    range.begin = GAME_PLAYER_MIN;
    range.end = GAME_PLAYER_MAX;
    prompt = ui_fmt(ui, "select number of players (%ld-%ld): ", range.begin, range.end);
    while (1) {
        ret = ui_input_int_prompt(ui, prompt, &range, &n_players);
        if (ret < 0)
            goto out_stop;
        if (ret > 0)
            break;
    }

    /* build choices */
    for (i = 0; i < GAME_PLAYER_MAX; i++) {
        choices[i].name = player_idx_to_name(i);
        choices[i].id = player_idx_to_char(i);
        if (i < 9)
            choices[i].alt_id = '1' + i;
    }

    sel.n_selected = 0;
    sel.n_choice = ARRAY_SIZE(choices);
    sel.choices = choices;
    while (1) {
        prompt = ui_fmt(ui, "select your player (%d/%d):\n", sel.n_selected + 1, n_players);
        ret = ui_selection_menu_prompt(ui, prompt, &sel);
        if (ret < 0)
            goto out_stop;
        if (ret == 0)
            continue;

        if (sel.n_selected >= n_players)
            break;
    }

    for (i = 0, seq = 0; i < GAME_PLAYER_MAX && seq < n_players; i++) {
        if (!choices[i].chosen)
            continue;
        idxs[seq++] = i;
    }

    if (game_add_players(game, idxs, n_players)) {
        game_err("fail to add players\n");
        goto out_stop;
    }

    game->state = GAME_STATE_START;
    return 0;

out_stop:
    game_stop(game, GAME_STATE_STOPPED);
    return -1;
}

static int game_player_step(struct game *game, struct player *player, int step)
{
    struct ui *ui = &game->ui;
    struct map *map = &game->map;
    struct map_node *node;
    int is_backward = 0;
    int n, pos;

    if (!player->valid || !player->attached) {
        game_err("player invalid\n");
        return -1;
    }

    if (step == 0) {
        game_dbg("step 0 hack\n");
        pos = player->pos;
        goto step_into;
    }

    if (step < 0) {
        step = -step;
        is_backward = 1;
    }

    /* walk */
    for (n = 1; n <= step; n++) {
        if (is_backward)
            pos = (player->pos - n + map->n_used) % map->n_used;
        else
            pos = (player->pos + n) % map->n_used;

step_into:
        node = &map->nodes[pos];
        if (node->item == ITEM_BLOCK) {
            node->item = ITEM_INVALID;
            fprintf(ui->out, "[STEP] Walked %d step(s) forward.\n", n);
            fprintf(ui->out, "[BLOCK] Oh! Stop here.\n");
            if (map_move_player(map, player, pos))
                return -1;
            return 1;
        }
        if (node->item == ITEM_BOMB) {
            int hospital_pos;

            node->item = ITEM_INVALID;
            fprintf(ui->out, "[STEP] Walked %d step(s) forward.\n", n);
            fprintf(ui->out, "[BOMB] Explosion! Transferred to nearest hospital, rest 3 rounds\n");

            hospital_pos = map_nearest_node_from(map, game->cur_layout, pos, MAP_NODE_HOSPITAL);
            if (map_move_player(map, player, pos))
                return -1;
            player->buff.n_empty_rounds = 3;
            return 1;
        }
    }

    if (map_move_player(map, player, pos))
        return -1;
    fprintf(ui->out, "[STEP] Walked %d step(s) forward.\n", step);
    return 1;
}

static int game_cmd_roll(struct game *game)
{
    return game_player_step(game, game->next_player, 1 + rand() % game->dice_facets);
}

static int game_cmd_sell(struct game *game, int argc, const char *argv[])
{
    struct ui *ui = &game->ui;
    struct map *map = &game->map;
    struct player *player = game->next_player;
    struct map_node *node;
    int idx, sold;
    char *endptr;

    if (argc != 2 || !argv[1]) {
        fprintf(ui->out, "step command syntax error\n");
        return -1;
    }

    endptr = NULL;
    idx = strtol(argv[1], &endptr, 10);
    if (*endptr) {
        fprintf(ui->out, "not a valid number: %s\n", argv[1]);
        return -1;
    }

    if (idx < 0 || idx >= map->n_used) {
        fprintf(ui->out, "sell %d out of map idx range [%d, %d)\n", idx, 0, map->n_used);
        return -1;
    }

    if (player->stat.n_sell_done >= game->max_sell_per_turn) {
        fprintf(ui->out, "[SELL] Already sold %d times, wait next turn.\n", player->stat.n_sell_done);
        return -1;
    }

    node = &map->nodes[idx];
    if (node->type != MAP_NODE_VACANCY) {
        fprintf(ui->out, "[SELL] Cannot sell, map idx %d not allowed to buy or sell.\n", idx);
        return -1;
    }
    if (node->estate.owner != player) {
        fprintf(ui->out, "[SELL] Cannot sell, map idx %d not owned by current player.\n", idx);
        return -1;
    }

    sold = 2 * map_node_price(node);
    player->asset.n_money += sold;

    node->estate.owner = NULL;
    node->estate.level = ESTATE_WASTELAND;
    list_del_init(&node->estate.estates_list);

    fprintf(ui->out, "[SELL] Sold map %d estate at price %d.\n", idx, sold);
    return 0;
}

static int game_cmd_step(struct game *game, int argc, const char *argv[])
{
    struct ui *ui = &game->ui;
    int step;
    char *endptr;

    if (argc != 2 || !argv[1]) {
        fprintf(ui->out, "step command syntax error\n");
        return -1;
    }

    endptr = NULL;
    step = strtol(argv[1], &endptr, 10);
    if (*endptr) {
        fprintf(ui->out, "not a valid number: %s\n", argv[1]);
        return -1;
    }

    return game_player_step(game, game->next_player, step);
}

static void game_cmd_help(struct game *game)
{
    struct ui *ui = &game->ui;

    fprintf(ui->out, "Available commands:\n");
    fprintf(ui->out, "  start       begin game\n");
    fprintf(ui->out, "  roll        roll dice and walk\n");
    fprintf(ui->out, "  sell N      sell estate on N-th map node\n");
    fprintf(ui->out, "  block N     use barrier item, N is distance from current player\n");
    fprintf(ui->out, "  bomb N      use bomb item, N is distance from current player\n");
    fprintf(ui->out, "  robot       use robot item\n");
    fprintf(ui->out, "  query       show current player stats\n");
    fprintf(ui->out, "  quit        stop game and exit\n");
    fprintf(ui->out, "  help        show this help\n");
    fprintf(ui->out, "\n");
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

    cmd = argv[0];
    game_debug_show_cmd(argc, argv);

    if (!strcmp(cmd, "quit")) {
        game_stop(game, GAME_STOP_NODUMP);
        return 0;
    } else if (!strcmp(cmd, "dump")) {
        game_stop(game, GAME_STOP_DUMP);
        return 0;
    } else if (!strcmp(cmd, "preset")) {
        return game_cmd_preset(game, argc, argv);
    } else if (!strcmp(cmd, "help")) {
        game_cmd_help(game);
        return 0;
    }

    if (game->state != GAME_STATE_RUNNING) {
        if (!strcmp(cmd, "start"))
            return game_cmd_start(game);
        return -1;
    }

    /* real game commands */
    if (!strcmp(cmd, "roll")) {
        return game_cmd_roll(game);

    } else if (!strcmp(cmd, "sell")) {
        return game_cmd_sell(game, argc, argv);

    } else if (!strcmp(cmd, "query")) {
    } else if (!strcmp(cmd, "block")) {
    } else if (!strcmp(cmd, "robot")) {
    } else if (!strcmp(cmd, "step")) {
        return game_cmd_step(game, argc, argv);
    }

    game_err("cmd '%s' unknown\n", cmd);
    return -1;
}

static char *game_read_line(struct game *game)
{
    char *line = NULL;

    line = ui_read_line(&game->ui);

    if (!line) {
        game_stop(game, GAME_STOP_NODUMP);
    }
    return line;
}

int game_event_loop(struct game *game)
{
    int stop_reason = 0;
    int cmd_ret;
    char *line = NULL;

    /* TODO: make sure on first enter, next_player is valid */
    while (game->state != GAME_STATE_STOPPED) {

        assert(game->state != GAME_STATE_UNINIT);
        ui_map_render(&game->ui, &game->map);

        /* TODO: check winning here */

        if (game_before_action(game)) {
            goto skip_action;
        }

        game_prompt_action(game);
        line = game_read_line(game);
        if (!line) {
            stop_reason = 1;
            break;
        }

        cmd_ret = game_handle_command(game, line);

        if (game->state == GAME_STATE_START)
            game->state = GAME_STATE_RUNNING;

        if (cmd_ret <= 0)
            continue;

skip_action:
        game_after_action(game);

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
        game_dump(game);

    game_uninit(game);
}


static void game_dump_player_asset(struct ui *ui, int id_char, struct asset *asset)
{
    struct map_node *node;

    list_for_each_entry(node, &asset->estates, estate.estates_list) {
        fprintf(ui->err, "map %d %c %d\n", node->idx, id_char, node->estate.level);
    }
    fprintf(ui->err, "fund %c %d\n", id_char, asset->n_money);
    fprintf(ui->err, "credit %c %d\n", id_char, asset->n_points);
}

static void game_dump_player_item(struct ui *ui, int id_char, struct asset *asset)
{
    if (asset->n_bomb)
        fprintf(ui->err, "gift %c bomb %d\n", id_char, asset->n_bomb);

    if (asset->n_block)
        fprintf(ui->err, "gift %c barrier %d\n", id_char, asset->n_block);

    if (asset->n_robot)
        fprintf(ui->err, "gift %c robot %d\n", id_char, asset->n_robot);
}

static void game_dump_player(struct ui *ui, struct player *player)
{
    struct map_node *node;
    int id_char = player_id_to_char(player);

    game_dump_player_asset(ui, id_char, &player->asset);
    fprintf(ui->err, "userloc %c %d %d\n", id_char, player->pos, player->buff.n_empty_rounds);

    game_dump_player_item(ui, id_char, &player->asset);
    if (player->buff.n_god_rounds)
        fprintf(ui->err, "gift %c god %d\n", id_char, player->buff.n_god_rounds);
}

void game_dump(struct game *game)
{
    int i;
    struct map_node *node;
    struct player *player;
    struct ui *ui = &game->ui;

    if (game->cur_player_nr)
        fprintf(ui->err, "user ");

    for_each_player_begin(game, player) {
        fprintf(ui->err, "%c", player_id_to_char(player));
    } for_each_player_end()

    if (game->cur_player_nr)
        fprintf(ui->err, "\n");

    for_each_player_begin(game, player) {
        game_dump_player(ui, player);
    } for_each_player_end()

    for (i = 0; i < game->map.n_used; i++) {
        node = &game->map.nodes[i];
        if (node->item == ITEM_BLOCK)
            fprintf(ui->err, "barrier %d\n", node->idx);
    }

    if (game->cur_player_nr)
        fprintf(ui->err, "nextuser %c\n", player_id_to_char(game->next_player));
}

