#include "common.h"
#include "player.h"
#include "map.h"

const struct map_layout g_default_map_layout_v1 = {
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

    .items = {
        .info = {
            {.type = ITEM_BLOCK, .on_sell = 1, .price = 50},
            {.type = ITEM_BOMB,  .on_sell = 0, .price = 50},
            {.type = ITEM_ROBOT, .on_sell = 1, .price = 30},
        }
    },
    .gifts = {
        .n_gifts = GIFT_MAX,
        .gifts = {
            [GIFT_MONEY] = { .value = 2000, .name = "Bonus Cash",    .grant = player_grant_gift_money },
            [GIFT_POINT] = { .value = 200,  .name = "Point Card",    .grant = player_grant_gift_point },
            [GIFT_GOD]   = { .value = 5,    .name = "God of Wealth", .grant = player_grant_gift_god },
        }
    },

    .n_area = 3,
    .areas = {
        {START_POS, ITEM_HOUSE_POS, AREA_1_PRICE},
        {ITEM_HOUSE_POS, GIFT_HOUSE_POS, AREA_2_PRICE},
        {GIFT_HOUSE_POS, MAGIC_HOUSE_POS + 1, AREA_3_PRICE},
    },

    .n_mine = 6,
    .pos_mine = {64, 65, 66, 67, 68, 69},
    .points_mine = {60, 80, 40, 100, 80, 20},
};

const struct map_layout g_default_map_layout_v2 = {
    .map_size = 70,
    .map_width = 29,

    .n_start = 1,
    .n_hospital = 1,
    .n_item_house = 1,
    .n_gift_house = 1,
    .n_prison = 0,
    .n_park = 1,
    .n_magic_house = 1,

    .pos_start = {START_POS},
    .pos_hospital = {HOSPITAL_POS},
    .pos_item_house = {ITEM_HOUSE_POS},
    .pos_gift_house = {GIFT_HOUSE_POS},
    .pos_prison = {},
    .pos_park = {PRISON_POS},
    .pos_magic_house = {MAGIC_HOUSE_POS},

    .items = {
        .info = {
            {.type = ITEM_BLOCK, .on_sell = 1, .price = 50},
            {.type = ITEM_BOMB,  .on_sell = 0, .price = 50},
            {.type = ITEM_ROBOT, .on_sell = 1, .price = 30},
        }
    },
    .gifts = {
        .n_gifts = GIFT_MAX,
        .gifts = {
            [GIFT_MONEY] = { .value = 2000, .name = "Bonus Cash",    .grant = player_grant_gift_money },
            [GIFT_POINT] = { .value = 200,  .name = "Point Card",    .grant = player_grant_gift_point },
            [GIFT_GOD]   = { .value = 5,    .name = "God of Wealth", .grant = player_grant_gift_god },
        }
    },

    .n_area = 3,
    .areas = {
        {START_POS, ITEM_HOUSE_POS, AREA_1_PRICE},
        {ITEM_HOUSE_POS, GIFT_HOUSE_POS, AREA_2_PRICE},
        {GIFT_HOUSE_POS, MAGIC_HOUSE_POS + 1, AREA_3_PRICE},
    },

    .n_mine = 6,
    .pos_mine = {64, 65, 66, 67, 68, 69},
    .points_mine = {60, 80, 40, 100, 80, 20},
};

const struct map_layout *g_default_map_layout = &g_default_map_layout_v2;

void map_set_default_layout(enum map_layout_ver ver)
{
    switch (ver) {
    case MAP_LAYOUT_V1:
        g_default_map_layout = &g_default_map_layout_v1;
        break;
    case MAP_LAYOUT_V2:
        g_default_map_layout = &g_default_map_layout_v2;
        break;
    }
}

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

static void map_node_init(struct map_node *node, int idx, enum node_type type, const void *priv)
{
    int i;

    memset(node, 0, sizeof(*node));
    node->idx = idx;
    node->type = type;
    INIT_LIST_HEAD(&node->players);
    node->item = ITEM_INVALID;
    node->item_owner = NULL;

    if (type == MAP_NODE_INVALID) {
        return;
    } else if (type == MAP_NODE_VACANCY) {
        const int *price = priv;
        node->estate.price = *price;
        node->estate.level = ESTATE_WASTELAND;
        node->estate.owner = NULL;
        INIT_LIST_HEAD(&node->estate.estates_list);

    } else if (type == MAP_NODE_ITEM_HOUSE) {
        const struct items_list *items = priv;
        node->item_house.min_price = -1;
        for (i = 0; i < ITEM_MAX; i++) {
            if (!items->info[i].on_sell)
                continue;
            node->item_house.n_on_sell++;
            if (node->item_house.min_price < 0 || node->item_house.min_price > items->info[i].price)
                node->item_house.min_price = items->info[i].price;
        }
        memcpy(&node->item_house.items, items, sizeof(node->item_house.items));

    } else if (type == MAP_NODE_GIFT_HOUSE) {
        memcpy(&node->gift_house, priv, sizeof(node->gift_house));

    } else if (type == MAP_NODE_MAGIC_HOUSE) {
    } else if (type == MAP_NODE_HOSPITAL) {
    } else if (type == MAP_NODE_PRISON) {
    } else if (type == MAP_NODE_MINE) {
        node->mine_points = *(int *) priv;
    } else if (type == MAP_NODE_PARK) {
    }
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
        map_node_init(&map->nodes[i], i, MAP_NODE_INVALID, NULL);
    }

    /* init special */
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_start; i++) {
        pos = layout->pos_start[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_START, NULL);
    }
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_hospital; i++) {
        pos = layout->pos_hospital[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_HOSPITAL, NULL);
    }
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_item_house; i++) {
        pos = layout->pos_item_house[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_ITEM_HOUSE, &layout->items);
    }
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_gift_house; i++) {
        pos = layout->pos_gift_house[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_GIFT_HOUSE, &layout->gifts);
    }
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_prison; i++) {
        pos = layout->pos_prison[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_PRISON, NULL);
    }
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_park; i++) {
        pos = layout->pos_park[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_PARK, NULL);
    }
    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_magic_house; i++) {
        pos = layout->pos_magic_house[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_MAGIC_HOUSE, NULL);
    }

    for (i = 0; i < MAP_MAX_SPECIAL && i < layout->n_mine; i++) {
        pos = layout->pos_mine[i];
        map_node_init(&map->nodes[pos], pos, MAP_NODE_MINE, &layout->points_mine[i]);
    }

    /* init area */
    for (i = 0; i < MAP_MAX_AREA && i < layout->n_area; i++) {
        const struct map_area *area = &layout->areas[i];

        for (pos = area->pos_start; pos < area->pos_end; pos++) {
            if (map->nodes[pos].type != MAP_NODE_INVALID)
                continue;
            map_node_init(&map->nodes[pos], pos, MAP_NODE_VACANCY, &area->price);
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
    return map_init(map, g_default_map_layout);
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

int map_place_item(struct map *map, int pos, enum item_type item, struct player *owner)
{
    struct map_node *node;

    if (pos < 0 || pos >= map->n_used)
        return -1;

    if (item <= ITEM_INVALID || item >= ITEM_MAX)
        return -1;

    node = &map->nodes[pos];
    node->item = item;
    if (owner)
        node->item_owner = owner;

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

    if (node->estate.owner && node->estate.owner != owner) {
        game_err("map pos %d alreadly owned by other player %d\n", pos, node->estate.owner->idx);
        return -1;
    }

    if (owner != node->estate.owner) {
        node->estate.owner = owner;
        list_add_tail(&node->estate.estates_list, &owner->asset.estates);
    }
    return 0;
}

int map_nearest_node_from(const struct map *map, const struct map_layout *layout, int from, enum node_type type)
{
    int distance;
    int pos;
    struct map_node *node;

    if (from < 0 || from >= map->n_used)
        return -1;

    if (layout->map_size != map->n_used) {
        game_err("layout size %d and map size %d not match\n", layout->map_size, map->n_used);
        return -1;
    }

    for (distance = 1; 2 * distance <= map->n_used; distance++) {
        /* look forward */
        pos = (from + distance) % map->n_used;
        node = &map->nodes[pos];
        if (node->type == type)
            return pos;

        /* look backward */
        pos = (from - distance + map->n_used) % map->n_used;
        node = &map->nodes[pos];
        if (node->type == type)
            return pos;
    }

    return -1;
}

int map_node_price(struct map_node *node)
{
    if (node->type != MAP_NODE_VACANCY)
        return 0;

    return node->estate.price * (1 + node->estate.level);
}

