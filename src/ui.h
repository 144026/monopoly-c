#pragma once
#include <stdio.h>
#include "map.h"

void discard_line(FILE *where);
const char *grab_line(FILE *where, char *buf, unsigned int size);

void ui_map_render(struct map *map);
int ui_game_prompt(struct game *game);

void dump_exit(struct game *game);
