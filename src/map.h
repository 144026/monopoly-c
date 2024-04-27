

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

struct map_node {
    int idx;
    enum node_type type;
    enum estate_level level;
    void *owner;
    enum item_type item;
    union {
        int price;
        int points;
    };
};

#define MAP_MAX_NODE    512

#define MAP_MIN_WIDTH   2
#define MAP_MIN_HEIGHT  2

struct map {
    int n_node;
    struct map_node *nodes;

    int n_used;
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

    int n_gift_house;
    int pos_gift_house[MAP_MAX_SPECIAL];

    int n_prison;
    int pos_prison[MAP_MAX_SPECIAL];

    int n_magic_house;
    int pos_magic_house[MAP_MAX_SPECIAL];

    int n_mine;
    int pos_mine[MAP_MAX_SPECIAL];
    int points_mine[MAP_MAX_SPECIAL];

    int n_area;
    struct map_area areas[MAP_MAX_AREA];
};

int init_map(struct map *map);
void uninit_map(struct map *map);

void map_render(struct map *map);