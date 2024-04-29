#include "common.h"
#include "game.h"
#include "ui.h"

static struct game g_game;

int main(void)
{
    int i;

    if (game_init(&g_game)) {
        game_err("fail to init game\n");
        return -1;
    }

    game_event_loop(&g_game);

    game_exit(&g_game);
    return 0;
}
