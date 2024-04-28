#include "common.h"
#include "player.h"
#include "map.h"

const struct map_layout g_default_map_layout = {
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

void map_free(struct map *map)
{
    map->n_used = 0;
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
    INIT_LIST_HEAD(&node->estates_list);

    INIT_LIST_HEAD(&node->players);
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

int map_init(struct map *map, const struct map_layout *layout)
{
    int ret;

    ret = map_alloc(map, layout->map_size);
    if (ret) {
        game_err("fail to alloc %d map node(s)\n", layout->map_size);
        goto out;
    }

    ret = map_fill_layout(map, layout);
    if (ret) {
        game_err("fail to fill map layout\n");
        goto out_free;
    }

    return 0;

out_free:
    map_free(map);
out:
    return ret;
}

int map_init_default(struct map *map)
{
    return map_init(map, &g_default_map_layout);
}

int map_attach_player(struct map *map, struct player *player)
{
    struct map_node *node;

    if (player->attached)
        return -1;
    if (player->pos < 0 || player->pos >= map->n_used)
        return -1;

    node = &map->nodes[player->pos];
    list_add(&player->pos_list, &node->players);
    player->attached = 1;

    return 0;
}

int map_detach_player(struct map *map, struct player *player)
{
    struct map_node *node;

    if (!player->attached)
        return -1;
    if (player->pos < 0 || player->pos >= map->n_used)
        return -1;

    node = &map->nodes[player->pos];
    if (list_empty(&node->players))
        return -1;

    list_del_init(&player->pos_list);
    player->attached = 0;

    return 0;
}

int map_move_player(struct map *map, struct player *player, int pos)
{
    int ret;

    if (pos < 0 || pos >= map->n_used)
        return -1;

    ret = map_detach_player(map, player);
    if (ret)
        return ret;

    player->pos = pos;
    return map_attach_player(map, player);
}

int map_place_item(struct map *map, int pos, enum item_type item)
{
    struct map_node *node;

    if (pos < 0 || pos >= map->n_used)
        return -1;

    if (item <= ITEM_INVALID || item >= ITEM_MAX)
        return -1;

    node = &map->nodes[pos];
    if (node->type != MAP_NODE_VACANCY) {
        game_err("map pos %d type %d, item %d not allowed\n", pos, node->type, item);
        return -1;
    }
    if (!list_empty(&node->players)) {
        game_err("map pos %d has players, item %d not allowed\n", pos, item);
        return -1;
    }
    if (node->item != ITEM_INVALID) {
        game_err("map pos %d already has item %d, item %d not allowed\n", pos, node->item, item);
        return -1;
    }

    node->item = item;
    return 0;
}

int map_set_owner(struct map *map, int pos, struct player *owner)
{
    struct map_node *node;

    if (pos < 0 || pos >= map->n_used)
        return -1;

    if (!owner->valid) {
        game_err("player %d not valid\n", owner->idx);
        return -1;
    }
    if (!owner->attached) {
        game_err("player %d not attached to map\n", owner->idx);
        return -1;
    }

    node = &map->nodes[pos];
    if (node->type != MAP_NODE_VACANCY) {
        game_err("map pos %d type %d, owner not allowed\n", pos, node->type);
        return -1;
    }

    if (node->owner && node->owner != owner) {
        game_err("map pos %d alreadly owned by other player %d\n", pos, node->owner->idx);
        return -1;
    }

    if (owner != node->owner) {
        node->owner = owner;
        list_add_tail(&node->estates_list, &owner->asset.estates);
    }
    return 0;
}

