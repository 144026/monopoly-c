#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef GAME_DEBUG
#define game_log(fmt, args...) fprintf(stderr, "[LOG] " fmt, ## args)
#define game_err(fmt, args...) fprintf(stderr, "[ERR] " fmt, ## args)
#define game_dbg(fmt, args...) fprintf(stderr, "[DBG] " fmt, ## args)
#else
#define game_log(fmt, args...)
#define game_err(fmt, args...)
#define game_dbg(fmt, args...)
#endif

