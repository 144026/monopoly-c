#include "common.h"
#include "game.h"
#include "ui.h"
#include <time.h>

static struct game g_game;

int main(void)
{
    int i;

    srand(time(NULL));
    setlinebuf(stdin);

    if (game_init(&g_game)) {
        game_err("fail to init game\n");
        return -1;
    }

    game_event_loop(&g_game);

    game_exit(&g_game);
    return 0;
}
