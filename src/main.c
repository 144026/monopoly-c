#include "common.h"
#include "ui.h"
#include "player.h"
#include "map.h"
#include <time.h>

int g_game_stop;

#define MAX_INPUT_LEN 512
#define INPUT_BUF_SIZE (MAX_INPUT_LEN + 2)
static char g_input_buf[INPUT_BUF_SIZE];

int main(void)
{
    int i;
    const char *line = NULL;

    srand(time(NULL));
    setlinebuf(stdin);

    if (init_map(&g_map)) {
        game_err("fail to init map\n");
        return -1;
    }

    /* TODO: delete test code */
    g_map.nodes[2].item = ITEM_BLOCK;
    for (i = 0; i < 4; i++) {
        if (add_player(i)) {
            game_err("fail to add player %d\n", i);
            return -2;
        }
        map_attach_player(&g_map, get_player(i));
    }
    g_next_player = get_player(0);

    while (!g_game_stop) {
        map_render(&g_map);

        line = grab_line(stdin, g_input_buf, INPUT_BUF_SIZE);
        if (line)
            game_dbg("read line: %s", line);

        if (!line) {
            game_dbg("end of file, exit\n");
            g_game_stop = 1;
            break;
        }
    }

    /* TODO: delete test code */
    dump_exit();

    del_all_players(&g_map);
    uninit_map(&g_map);
    return 0;
}
