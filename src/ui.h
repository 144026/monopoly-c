#pragma once
#include <stdio.h>

int prompt(void);
void discard_line(FILE *where);
const char *grab_line(FILE *where, char *buf, unsigned int size);
