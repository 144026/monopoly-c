#pragma once
#include "common.h"
#include "list.h"

#define AREA_1_PRICE        200
#define AREA_2_PRICE        500
#define AREA_3_PRICE        300

#define START_POS           0
#define HOSPITAL_POS        14
#define ITEM_HOUSE_POS      28
#define GIFT_HOUSE_POS      35
#define PRISON_POS          49
#define MAGIC_HOUSE_POS     63
#define MAP_SIZE            70


enum node_type {
    MAP_NODE_START,
    MAP_NODE_VACANCY,
    MAP_NODE_ITEM_HOUSE,
    MAP_NODE_GIFT_HOUSE,
    MAP_NODE_MAGIC_HOUSE,
    MAP_NODE_HOSPITAL,
    MAP_NODE_PRISON,
    MAP_NODE_MINE,
    MAP_NODE_PARK,
    MAP_NODE_MAX,
    MAP_NODE_INVALID = -1,
};

enum estate_level {
    ESTATE_WASTELAND,
    ESTATE_HUT,
    ESTATE_HOUSE,
    ESTATE_SKYSCRAPER,
    ESTATE_MAX,
    ESTATE_INVALID = -1,
};

enum item_type {
    ITEM_BLOCK,
    ITEM_BOMB,
    ITEM_ROBOT,
    ITEM_MAX,
    ITEM_INVALID = -1,
};

struct estate {
    int price;
    enum estate_level level;
    struct player *owner;
    struct list_head estates_list;
};

struct item_info {
    enum item_type type;
    int on_sell;
    int price;
};

struct items_list {
    struct item_info info[ITEM_MAX];
};

struct item_house {
    int n_on_sell;
    int min_price;
    struct items_list items;
};

enum gift_type {
    GIFT_MONEY,
    GIFT_POINT,
    GIFT_GOD,
    GIFT_MAX,
    GIFT_INVALID = -1,
};

struct gift_info {
    int value;
    const char *name;
    int (*grant)(struct gift_info *gift, struct game *game, struct player *player);
};

struct gift_house {
    int n_gifts;
    struct gift_info gifts[GIFT_MAX];
};

struct map_node {
    int idx;
    enum node_type type;
    struct list_head players;
    enum item_type item;
    struct player *item_owner;

    union {
        struct estate estate;
        struct item_house item_house;
        struct gift_house gift_house;
        int mine_points;
    };
};

#define MAP_MAX_NODE    512

#define MAP_MIN_WIDTH   2
#define MAP_MIN_HEIGHT  2

struct map {
    int n_node;
    int n_used;
    struct map_node *nodes;

    /* need re-draw */
    int dirty;

    /* corner is counted in both w/h */
    unsigned int width;
    unsigned int height;
};

struct map_area {
    int pos_start;
    int pos_end;
    int price;
};

#define MAP_MAX_SPECIAL 8
#define MAP_MAX_AREA    8

struct map_layout {
    int map_size;
    int map_width;

    int n_start;
    int pos_start[MAP_MAX_SPECIAL];

    int n_hospital;
    int pos_hospital[MAP_MAX_SPECIAL];

    int n_item_house;
    int pos_item_house[MAP_MAX_SPECIAL];
    struct items_list items;

    int n_gift_house;
    int pos_gift_house[MAP_MAX_SPECIAL];
    struct gift_house gifts;

    int n_prison;
    int pos_prison[MAP_MAX_SPECIAL];

    int n_park;
    int pos_park[MAP_MAX_SPECIAL];

    int n_magic_house;
    int pos_magic_house[MAP_MAX_SPECIAL];

    int n_mine;
    int pos_mine[MAP_MAX_SPECIAL];
    int points_mine[MAP_MAX_SPECIAL];

    int n_area;
    struct map_area areas[MAP_MAX_AREA];
};

extern const struct map_layout *g_default_map_layout;

enum map_layout_ver {
    MAP_LAYOUT_V1,
    MAP_LAYOUT_V2,
};

void map_set_default_layout(enum map_layout_ver ver);

int map_init(struct map *map, const struct map_layout *layout);
void map_free(struct map *map);

int map_attach_player(struct map *map, struct player *player);
int map_detach_player(struct map *map, struct player *player);
int map_move_player(struct map *map, struct player *player, int pos);

int map_place_item(struct map *map, int pos, enum item_type item, struct player *owner);

int map_set_owner(struct map *map, int pos, struct player *owner);

int map_nearest_node_from(const struct map *map, const struct map_layout *layout, int pos, enum node_type type);

int map_node_price(struct map_node *node);
