#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>

#ifdef GAME_DEBUG
#define game_log(fmt, args...) fprintf(stdout, "[LOG][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args)
#define game_err(fmt, args...) fprintf(stdout, "[ERR][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args)
#define game_dbg(fmt, args...) fprintf(stdout, "[DBG][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args)
#else
#define game_log(fmt, args...)
#define game_err(fmt, args...)
#define game_dbg(fmt, args...)
#endif

struct range {
    long begin;
    long end;
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof (arr) / sizeof (arr)[0])
#endif

/* forward declaration */
struct game;
struct ui;
struct map;
struct player;

struct item_info;
struct gift_info;
