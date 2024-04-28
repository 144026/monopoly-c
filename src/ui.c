#include "common.h"
#include "game.h"
#include "player.h"
#include "ui.h"


static const char *player_ui_color[] = {
    [PLAYER_COLOR_INVALID] = "\e[0m",
    [PLAYER_COLOR_RED] = "\e[31m",
    [PLAYER_COLOR_GREEN] = "\e[32m",
    [PLAYER_COLOR_BLUE] = "\e[34m",
    [PLAYER_COLOR_YELLOW] = "\e[33m",
    [PLAYER_COLOR_WHITE] = "\e[37m",
};


static void prompt_player_name(struct player *player)
{
    if (!player || !player->valid || !player->attached) {
        fprintf(stdout, "NULL");
        return;
    }

    fprintf(stdout, "%s", player_ui_color[player->color]);

    if (!player->name)
        fprintf(stdout, "%c", player_id_to_char(player));
    else
        fprintf(stdout, "%s", player->name);

    fprintf(stdout, "%s", player_ui_color[PLAYER_COLOR_INVALID]);
}

int ui_game_prompt(struct game *game)
{
    if (game->state == GAME_STATE_RUNNING)
        prompt_player_name(game->next_player);
    else
        fprintf(stdout, "enter 'start' to play");

    fprintf(stdout, "> ");
    return 0;
}

static void discard_line(FILE *where)
{
    int c;

    while ((c = getc(where)) != EOF) {
        if (c == '\n')
            return;
    }
}

/* @return NULL if eof, empty string if nothing is read */
char *ui_read_line(FILE *where, char *buf, unsigned int size)
{
    char *ret;

    if (size < 2) {
        game_err("input buffer too small, can't read\n");
        return "";
    }
    /* guard */
    buf[size - 2] = 0;

    ret = fgets(buf, size, where);
    if (!ret) {
        if (feof(where))
            return NULL;
        game_err("fail to read input\n");
        return "";
    }

    /* check guard */
    if (buf[size - 2] && buf[size - 2] != '\n') {
        game_err("line length exceeds maximum %d characters\n", size - 2);
        buf[size - 2] = '\n';
        discard_line(where);
    }
    return ret;
}

static inline int ui_cmd_eol(int c)
{
    return !c || c == '\n' || c == '#';
}

int ui_cmd_tokenize(char *cmd, const char *argv[], int n)
{
    int argc = 0;

    while (argc < n) {
        /* strip front */
        while (isspace(*cmd))
            cmd++;
        /* end of line */
        if (ui_cmd_eol(*cmd))
            break;

        /* eat through */
        argv[argc++] = cmd;
        while (*cmd && !isspace(*cmd) && *cmd != '#')
            cmd++;
        /* '\0', space, or '#' */
        if (isspace(*cmd)) {
            *cmd++ = '\0';
        } else if (*cmd == '#') {
            *cmd++ = '\0';
            break;
        }
    }

    return argc;
}


static const char node_render_tab[MAP_NODE_MAX] = {
    [MAP_NODE_START] = 'S',
    [MAP_NODE_VACANCY] = '0',
    [MAP_NODE_ITEM_HOUSE] = 'T',
    [MAP_NODE_GIFT_HOUSE] = 'G',
    [MAP_NODE_MAGIC_HOUSE] = 'M',
    [MAP_NODE_HOSPITAL] = 'H',
    [MAP_NODE_PRISON] = 'P',
    [MAP_NODE_MINE] = '$',
    [MAP_NODE_PARK] = 'P',
};

static void map_node_render(struct map *map, unsigned line, unsigned col)
{
    int pos = -1;
    struct map_node *node;
    struct player *player;

    if (line == 0) {
        pos = col;
    } else if (line == map->height - 1) {
        pos = map->n_used - (map->height - 1) - col;
    } else if (col == 0) {
        pos = map->n_used - line;
    } else if (col == map->width - 1) {
        pos = map->width + (line - 1);
    }

    if (pos < 0) {
        putchar(' ');
        return;
    }

    if (pos >= map->n_used) {
        game_err("line %u column %u calculated node idx %d > map->n_used %d\n", line, col, pos, map->n_used);
        return;
    }

    node = &map->nodes[pos];
    if (list_empty(&node->players)) {
        putchar(node_render_tab[node->type]);
        return;
    }

    player = list_first_entry(&node->players, struct player, pos_list);
    putchar(player_id_to_char(player));
}

void ui_map_render(struct map *map)
{
    int line, col;

    for (line = 0; line < map->height; line++) {
        for (col = 0; col < map->width; col++)
            map_node_render(map, line, col);
        putchar('\n');
    }
}

void dump_exit_player_asset(int id_char, struct asset *asset)
{
    struct map_node *estate;

    list_for_each_entry(estate, &asset->estates, estates_list) {
        fprintf(stderr, "map %d %c %d\n", estate->idx, id_char, estate->level);
    }
    fprintf(stderr, "fund %c %d\n", id_char, asset->n_money);
    fprintf(stderr, "credit %c %d\n", id_char, asset->n_points);
}

void dump_exit_player_item(int id_char, struct asset *asset)
{
    if (asset->n_bomb)
        fprintf(stderr, "gift %c bomb %d\n", id_char, asset->n_bomb);

    if (asset->n_block)
        fprintf(stderr, "gift %c block %d\n", id_char, asset->n_block);

    if (asset->n_robot)
        fprintf(stderr, "gift %c robot %d\n", id_char, asset->n_robot);
}

void dump_exit_player(struct player *player)
{
    struct map_node *node;
    int id_char = player_id_to_char(player);

    dump_exit_player_asset(id_char, &player->asset);
    fprintf(stderr, "userloc %c %d %d\n", id_char, player->pos, player->buff.n_empty_rounds);

    dump_exit_player_item(id_char, &player->asset);
    if (player->buff.n_god_buff + player->buff.b_god_buff)
        fprintf(stderr, "gift %c god %d\n", id_char, player->buff.n_god_buff + player->buff.b_god_buff);
}

void dump_exit(struct game *game)
{
    int i;
    struct map_node *node;
    struct player *player;

    if (game->cur_player_nr)
        fprintf(stderr, "user ");

    for_each_player_begin(game, player) {
        fprintf(stderr, "%c", player_id_to_char(player));
    } for_each_player_end()

    if (game->cur_player_nr)
        fprintf(stderr, "\n");

    for_each_player_begin(game, player) {
        dump_exit_player(player);
    } for_each_player_end()

    for (i = 0; i < game->map.n_used; i++) {
        node = &game->map.nodes[i];
        if (node->item == ITEM_BLOCK)
            fprintf(stderr, "barrier %d\n", node->idx);
    }

    if (game->cur_player_nr)
        fprintf(stderr, "nextuser %c\n", player_id_to_char(game->next_player));
    exit(EXIT_SUCCESS);
}
