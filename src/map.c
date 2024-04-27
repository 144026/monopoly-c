#include "common.h"
#include "map.h"

static const struct map_layout g_default_map_layout = {
    .map_size = 70,
    .map_width = 29,

    .n_start = 1,
    .n_hospital = 1,
    .n_item_house = 1,
    .n_gift_house = 1,
    .n_prison = 1,
    .n_magic_house = 1,

    .pos_start = {START_POS},
    .pos_hospital = {HOSPITAL_POS},
    .pos_item_house = {ITEM_HOUSE_POS},
    .pos_gift_house = {GIFT_HOUSE_POS},
    .pos_prison = {PRISON_POS},
    .pos_magic_house = {MAGIC_HOUSE_POS},

    .n_area = 3,
    .areas = {
        {START_POS, ITEM_HOUSE_POS, AREA_1_PRICE},
        {ITEM_HOUSE_POS, GIFT_HOUSE_POS, AREA_2_PRICE},
        {GIFT_HOUSE_POS, MAGIC_HOUSE_POS + 1, AREA_3_PRICE},
    },

    .n_mine = 6,
    .pos_mine = {64, 65, 66, 67, 68, 69},
    .points_mine = {60, 80, 40, 100, 80, 30},
};

static const struct map_layout *g_cur_layout = &g_default_map_layout;

static int map_alloc(struct map *map, int n_node)
{
    if (n_node <= 0 || n_node > MAP_MAX_NODE)
        return -1;

    map->n_node = n_node;
    map->nodes = calloc(n_node, sizeof(struct map_node));

    if (!map->nodes) {
        return -2;
    }

    return 0;
}

static void map_free(struct map *map)
{
    map->n_node = 0;

    if (map->nodes) {
        free(map->nodes);
        map->nodes = NULL;
    }
}

static void map_node_init(struct map_node *node, int idx, enum node_type type, int v)
{
    memset(node, 0, sizeof(*node));

    node->idx = idx;
    node->type = type;

    node->level = ESTATE_INVALID;
    node->owner = NULL;
    node->item = ITEM_INVALID;

    if (type == MAP_NODE_VACANCY)
        node->price = v;
    else if (type == MAP_NODE_MINE)
        node->points = v;
}

static int map_fill_layout(struct map *map, const struct map_layout *layout)
{
    int i;
    int pos;
    struct map_node *node;

    if (layout->map_size > map->n_node) {
        game_err("map layout size %d > map capacity %d\n", layout->map_size, map->n_node);
        return -1;
    }
    if (layout->map_size & 1) {
        game_err("map layout size %d must be even number\n", layout->map_size);
        return -1;
    }
    map->n_used = layout->map_size;

    if (layout->map_width < MAP_MIN_WIDTH) {
        game_err("map width %d too small\n", layout->map_width);
        return -1;
    }
    if (layout->map_width * 2 > layout->map_size) {
        game_err("map width %d too big\n", layout->map_width);
        return -1;
    }
    map->width = layout->map_width;
    map->height = 2 + (layout->map_size - layout->map_width * 2) / 2;

    for (i = 0; i < map->n_node; i++) {
        map_node_init(&map->nodes[i], i, MAP_NODE_INVALID, 0);
    }

    /* init special */
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_start; i++) {
        pos = layout->pos_start[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_START, 0);
    }
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_hospital; i++) {
        pos = layout->pos_hospital[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_HOSPITAL, 0);
    }
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_item_house; i++) {
        pos = layout->pos_item_house[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_ITEM_HOUSE, 0);
    }
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_gift_house; i++) {
        pos = layout->pos_gift_house[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_GIFT_HOUSE, 0);
    }
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_prison; i++) {
        pos = layout->pos_prison[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_PRISON, 0);
    }
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_magic_house; i++) {
        pos = layout->pos_magic_house[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_MAGIC_HOUSE, 0);
    }

    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_mine; i++) {
        pos = layout->pos_mine[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_MINE, layout->points_mine[i]);
    }

    /* init area */
    for (i = 0; i < MAP_MAX_AREA && i < layout->n_area; i++) {
        const struct map_area *area = &layout->areas[i];

        for (pos = area->pos_start; pos < area->pos_end; pos++) {
            if (map->nodes[pos].type != MAP_NODE_INVALID)
                continue;
            map_node_init(&map->nodes[pos], pos, MAP_NODE_VACANCY, area->price);
        }
    }

    /* if any node is left invalid, reject this layout */
    for (i = 0; i < map->n_used; i++) {
        node = &map->nodes[i];
        if (node->idx != i) {
            game_err("layout invalid, node idx %d != node pos %d\n", node->idx, i);
            return -2;
        }
        if (node->type == MAP_NODE_INVALID) {
            game_err("layout invalid, node idx %d is not covered\n", i);
            return -3;
        }
    }

    return 0;
}

static int map_init(struct map *map, const struct map_layout *layout)
{
    int ret;

    ret = map_alloc(map, layout->map_size);
    if (ret) {
        game_err("fail to alloc %d map node(s)\n", layout->map_size);
        goto out;
    }

    ret = map_fill_layout(map, layout);
    if (ret) {
        game_err("fail to fill map layout");
        goto out_free;
    }

    return 0;

out_free:
    map_free(map);
out:
    return ret;
}

int init_map(struct map *map)
{
    return map_init(map, g_cur_layout);
}

void uninit_map(struct map *map)
{
    map->n_used = 0;
    map_free(map);
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
        game_err("line %u column %u calculated node idx %d > map->n_used %d", line, col, pos, map->n_used);
        return;
    }

    node = &map->nodes[pos];
    putchar(node_render_tab[node->type]);
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