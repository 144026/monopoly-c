#include "common.h"
#include "ui.h"
#include <time.h>

int g_game_stop;

#define MAX_INPUT_LEN 512
#define INPUT_BUF_SIZE (MAX_INPUT_LEN + 2)
static char g_input_buf[INPUT_BUF_SIZE];

int main(void)
{
    const char *line = NULL;

    srand(time(NULL));
    setlinebuf(stdin);

    while (!g_game_stop) {
        line = grab_line(stdin, g_input_buf, INPUT_BUF_SIZE);
        if (line)
            game_dbg("read line: %s", line);

        if (!line) {
            game_dbg("end of file, exit\n");
            g_game_stop = 1;
            break;
        }
    }

    return 0;
}
