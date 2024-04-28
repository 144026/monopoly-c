#pragma once
#include <stdio.h>
#include "map.h"

char *ui_read_line(FILE *where, char *buf, unsigned int size);

void ui_map_render(struct map *map);
int ui_game_prompt(struct game *game);
int ui_cmd_tokenize(char *cmd, const char *argv[], int n);

void dump_exit(struct game *game);
