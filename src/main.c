#include <signal.h>
#include "common.h"
#include "game.h"

#ifdef GAME_DEBUG
int g_game_dbg = 1;
#else
int g_game_dbg = 0;
#endif

static struct game g_game;
struct game_events g_game_events = {0};

void handle_winch(int sig)
{
    g_game_events.event_winch = 1;
}

void handle_term(int sig)
{
    g_game_events.event_term = sig;
}

int main(void)
{
    struct  sigaction winch_act = { .sa_handler = handle_winch };
    struct  sigaction term_act = { .sa_handler = handle_term };

    sigaction(SIGWINCH, &winch_act, NULL);
    sigaction(SIGINT, &term_act, NULL);
    sigaction(SIGTERM, &term_act, NULL);

    if (game_init(&g_game)) {
        game_err("fail to init game\n");
        return -1;
    }

    game_event_loop(&g_game);

    game_exit(&g_game);
    return 0;
}
