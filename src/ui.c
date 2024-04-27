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
    if (list_empty(&node->player_list)) {
        putchar(node_render_tab[node->type]);
        return;
    }

    player = list_first_entry(&node->player_list, struct player, pos_list);
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
