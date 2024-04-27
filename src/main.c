#include "common.h"
#include "ui.h"
#include "map.h"
#include <time.h>

int g_game_stop;

#define MAX_INPUT_LEN 512
#define INPUT_BUF_SIZE (MAX_INPUT_LEN + 2)
static char g_input_buf[INPUT_BUF_SIZE];

static struct map g_map;

int main(void)
{
    const char *line = NULL;

    srand(time(NULL));
    setlinebuf(stdin);

    if (init_map(&g_map)) {
        game_err("fail to init map\n");
        return -1;
    }

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

    uninit_map(&g_map);
    return 0;
}
