#pragma once
#include <stdio.h>
#include "map.h"

int prompt(void);
void discard_line(FILE *where);
const char *grab_line(FILE *where, char *buf, unsigned int size);

void map_render(struct map *map);

void dump_exit(void);
