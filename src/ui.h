#pragma once
#include <stdio.h>
#include "map.h"

#define MAX_INPUT_LEN 512
#define INPUT_BUF_SIZE (MAX_INPUT_LEN + 2)

#define FORMAT_BUF_SIZE 512
#define N_FORMAT_BUF   4

struct ui {
    FILE *in;
    FILE *out;
    FILE *err;

    int in_isatty;
    int out_isatty;
    int lines;
    int cols;
    int use_clear;
    int use_setwin;

    int in_buf_size;
    char *in_buf;

    int fmt_buf_size;
    char *fmt_buf[N_FORMAT_BUF];
    int fmt_idx;
};

int ui_init(struct ui *ui);
int ui_uninit(struct ui *ui);
int ui_is_interactive(struct ui *ui);

const char *ui_player_name(struct ui *ui, struct player *player);
const char *ui_item_name(enum item_type type);
void ui_map_render(struct ui *ui, struct map *map);

void ui_on_game_start(struct ui *ui, struct map *map);
void ui_on_game_stop(struct ui *ui);
void ui_handle_winch(struct ui *ui, struct map *map);

void ui_prompt_player_name(struct ui *ui, struct player *player);
int ui_dump_player_stats(struct ui *ui, const char *prompt, struct player *player);

char *ui_read_line(struct ui *ui);
int ui_cmd_tokenize(char *cmd, const char *argv[], int n);


#ifndef __printf
#define __printf(x, y) __attribute__(( format(printf, x, y) ))
#endif

const char *ui_fmt(struct ui *ui, const char *fmt, ...) __printf(2, 3) ;

int ui_input_bool_prompt(struct ui *ui, const char *prompt, int *res);
int ui_input_int_prompt(struct ui *ui, const char *prompt, const struct range *range, int *res);

struct choice {
    const char *name;
    char id;
    char alt_id;
    char chosen;
};

struct select {
    int n_choice;
    struct choice *choices;
    /* return from callee */
    int cur_choice;
    int n_selected;
};

int ui_selection_menu_prompt(struct ui *ui, const char *prompt, struct select *sel);
