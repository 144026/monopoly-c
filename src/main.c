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

    /* TODO: delete test code */
    g_game.map.nodes[2].item = ITEM_BLOCK;
    for (i = 0; i < 4; i++) {
        if (game_add_player(&g_game, i)) {
            game_err("fail to add player %d\n", i);
            return -2;
        }
        map_attach_player(&g_game.map, game_get_player(&g_game, i));
    }
    g_game.next_player_seq = 0;
    g_game.next_player = game_get_player(&g_game, 0);
    g_game.state = GAME_STATE_RUNNING;

    game_event_loop(&g_game);

    game_exit(&g_game);
    return 0;
}
