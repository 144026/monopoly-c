#include "common.h"
#include "player.h"
#include "ui.h"

int prompt(void)
{
    fprintf(stdout, "input> ");
    return 0;
}

void discard_line(FILE *where)
{
    int c;

    while ((c = getc(where)) != EOF) {
        if (c == '\n')
            return;
    }
}

/* @return NULL if eof, empty string if nothing is read */
const char *grab_line(FILE *where, char *buf, unsigned int size)
{
    const char *ret;

    if (size < 2) {
        game_err("input buffer too small, can't read\n");
        return "";
    }
    /* guard */
    buf[size - 2] = 0;

    prompt();

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
    putchar(player_render_id(player));
}

void map_render(struct map *map)
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
    int id_char = player_render_id(player);

    dump_exit_player_asset(id_char, &player->asset);
    fprintf(stderr, "userloc %c %d %d\n", id_char, player->pos, player->buff.n_empty_rounds);

    dump_exit_player_item(id_char, &player->asset);
    if (player->buff.n_god_buff + player->buff.b_god_buff)
        fprintf(stderr, "gift %c god %d\n", id_char, player->buff.n_god_buff + player->buff.b_god_buff);
}

void dump_exit(void)
{
    int i;
    struct map_node *node;
    struct player *player;

    fprintf(stderr, "user ");
    for_each_player_begin(player) {
        fprintf(stderr, "%c", player_render_id(player));
    } for_each_player_end()
    fprintf(stderr, "\n");

    for_each_player_begin(player) {
        dump_exit_player(player);
    } for_each_player_end()

    for (i = 0; i < g_map.n_used; i++) {
        node = &g_map.nodes[i];
        if (node->item == ITEM_BLOCK)
            fprintf(stderr, "barrier %d\n", node->idx);
    }

    fprintf(stderr, "nextuser %c", player_render_id(g_next_player));
    exit(EXIT_SUCCESS);
}
