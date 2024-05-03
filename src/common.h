#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>

extern int g_game_dbg;

#define game_log(fmt, args...) do { if (g_game_dbg) fprintf(stdout, "[LOG][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)
#define game_err(fmt, args...) do { if (g_game_dbg) fprintf(stdout, "[ERR][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)
#define game_dbg(fmt, args...) do { if (g_game_dbg) fprintf(stdout, "[DBG][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)

struct range {
    long begin;
    long end;
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof (arr) / sizeof (arr)[0])
#endif

/* forward declaration */
struct game_events;
struct game;
struct ui;
struct map;
struct player;

struct item_info;
struct gift_info;

extern struct game_events g_game_events;
