#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "common.h"
#include "player.h"
#include "ui.h"
#include "term.h"


static const char item_ui_char[] = {
    [ITEM_BLOCK] = '#',
    [ITEM_BOMB] = '@',
    [ITEM_ROBOT] = 'R',
};

static const char *item_ui_name[] = {
    [ITEM_BLOCK] = "Block",
    [ITEM_BOMB] = "Bomb",
    [ITEM_ROBOT] = "Robot",
};

static const char *player_ui_color[] = {
    [PLAYER_COLOR_NONE] = "",
    [PLAYER_COLOR_RED] = VT100_COLOR_RED,
    [PLAYER_COLOR_GREEN] = VT100_COLOR_GREEN,
    [PLAYER_COLOR_BLUE] = VT100_COLOR_BLUE,
    [PLAYER_COLOR_YELLOW] = VT100_COLOR_YELLOW,
    [PLAYER_COLOR_WHITE] = VT100_COLOR_WHITE,
};

int ui_init(struct ui *ui)
{
    int i;

    memset(ui, 0, sizeof(*ui));
    ui->in = stdin;
    ui->out = stdout;
    ui->err = stderr;

    ui->in_isatty = isatty(fileno(ui->in));
    ui->out_isatty = isatty(fileno(ui->out));
    if (ui->out_isatty) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        ui->lines = w.ws_row;
        ui->cols = w.ws_col;
    }

    ui->in_buf_size = INPUT_BUF_SIZE;
    ui->in_buf = calloc(1, INPUT_BUF_SIZE);
    if (!ui->in_buf)
        return -1;

    ui->fmt_idx = 0;
    ui->fmt_buf_size = FORMAT_BUF_SIZE;
    ui->fmt_buf[0] = calloc(N_FORMAT_BUF, FORMAT_BUF_SIZE);
    if (!ui->fmt_buf[0]) {
        free(ui->in_buf);
        return -2;
    }
    for (i = 1; i < N_FORMAT_BUF; i++) {
        ui->fmt_buf[i] = ui->fmt_buf[0] + i * FORMAT_BUF_SIZE;
    }

    setvbuf(ui->in, NULL, _IONBF, 0);
    return 0;
}

int ui_uninit(struct ui *ui)
{
    int i;

    if (ui->in_buf) {
        free(ui->in_buf);
        ui->in_buf = NULL;
        ui->in_buf_size = 0;
    }

    if (ui->fmt_buf[0]) {
        free(ui->fmt_buf[0]);
        for (i = 0; i < N_FORMAT_BUF; i++)
            ui->fmt_buf[i] = NULL;

        ui->fmt_buf_size = 0;
        ui->fmt_idx = 0;
    }
    return 0;
}

int ui_is_interactive(struct ui *ui)
{
    return ui && ui->in_isatty && ui->out_isatty;
}

static int ui_vsnprintf(char *buf, size_t size, const char *format, va_list ap)
{
    int n;

    n = vsnprintf(buf, size, format, ap);

    if (n < 0)
        return 0;
    if (n < size)
        return n;
    /* n >= size, no more space */
    return size;
}

const char *ui_fmt(struct ui *ui, const char *fmt, ...)
{
    char *buf = ui->fmt_buf[ui->fmt_idx];
    va_list ap;

    va_start(ap, fmt);
    ui_vsnprintf(buf, ui->fmt_buf_size, fmt, ap);
    va_end(ap);

    ui->fmt_idx += 1;
    ui->fmt_idx %= N_FORMAT_BUF;
    return buf;
}

const char *ui_player_name(struct ui *ui, struct player *player)
{
    const char *color;
    const char *modesoff;

    if (!player || !player->valid || !player->attached)
        return "NULL";

    if (ui->out_isatty) {
        color = player_ui_color[player->color];
        modesoff = VT100_MODES_OFF;
    } else {
        color = modesoff = "";
    }

    if (!player->name)
        return ui_fmt(ui, "%s%c%s", color, player_id_to_char(player), modesoff);

    return ui_fmt(ui, "%s%s%s", color, player->name, modesoff);
}

void ui_prompt_player_name(struct ui *ui, struct player *player)
{
    fprintf(ui->out, "%s", ui_player_name(ui, player));
}


static void discard_line(FILE *where)
{
    int c;

    while ((c = getc(where)) != EOF) {
        if (c == '\n')
            return;
    }
}

/* @return NULL if eof, empty string if nothing is read */
char *ui_read_line(struct ui *ui)
{
    char *ret;
    char *buf = ui->in_buf;
    int size = ui->in_buf_size;

    if (!buf || size < 2) {
        game_err("input buffer error, can't read\n");
        return NULL;
    }

    /* guard */
    buf[size - 2] = buf[size - 1] = 0;

    ret = fgets(buf, size, ui->in);
    if (!ret) {
        if (feof(ui->in)) {
            game_dbg("end of file\n");
            return NULL;
        }
        game_err("fail to read input\n");
        return "";
    }

    /* check guard */
    if (buf[size - 2] && buf[size - 2] != '\n') {
        game_err("line length exceeds maximum %d characters\n", size - 2);
        buf[size - 2] = '\n';
        discard_line(ui->in);
    }

    if (!ui->in_isatty) {
        /* echo back user input */
        fprintf(ui->out, "%s", buf);
    }

    game_dbg("read line: %s", ret);
    return ret;
}

static inline int ui_cmd_eol(int c)
{
    return !c || c == '\n' || c == '#';
}

int ui_cmd_tokenize(char *cmd, const char *argv[], int n)
{
    int argc = 0;

    while (argc < n) {
        /* strip front */
        while (isspace(*cmd))
            cmd++;
        /* end of line */
        if (ui_cmd_eol(*cmd))
            break;

        /* eat through */
        argv[argc++] = cmd;
        while (*cmd && !isspace(*cmd) && *cmd != '#')
            cmd++;
        /* '\0', space, or '#' */
        if (isspace(*cmd)) {
            *cmd++ = '\0';
        } else if (*cmd == '#') {
            *cmd++ = '\0';
            break;
        }
    }

    return argc;
}

/* @return: < 0 fatal err, == 0 try again, > 0 done */
int ui_input_bool_prompt(struct ui *ui, const char *prompt, int *res)
{
    char *line = NULL;
    const char *toks[4], *ans;
    int val;
    int n_tok;

    if (prompt)
        fprintf(ui->out, "%s (y/n) ", prompt);

    line = ui_read_line(ui);
    if (!line)
        return -1;

    n_tok = ui_cmd_tokenize(line, toks, ARRAY_SIZE(toks));
    if (n_tok == 0) {
        return 0;
    } else if (n_tok != 1) {
        fprintf(ui->out, "input error, only y or n allowed, got %d tokens\n", n_tok);
        return 0;
    }

    ans = toks[0];
    while (isspace(*ans))
        ans++;

    val = *ans;
    if (isalpha(val))
        val = tolower(val);

    if (val == 'y' || val == 'Y') {
        *res = 1;
    } else if (val == 'n' || val == 'N') {
        *res = 0;
    } else {
        fprintf(ui->out, "input error, only y or n allowed, got %s\n", toks[0]);
        return 0;
    }

    return 1;
}

/* @return: < 0 fatal err, == 0 try again, > 0 done */
int ui_input_int_prompt(struct ui *ui, const char *prompt, const struct range *range, int *res)
{
    char *endptr, *line = NULL;
    const char *toks[4];
    int n_tok, num;

    if (prompt)
        fprintf(ui->out, "%s", prompt);

    line = ui_read_line(ui);
    if (!line)
        return -1;

    n_tok = ui_cmd_tokenize(line, toks, ARRAY_SIZE(toks));
    if (n_tok == 0) {
        return 0;
    } else if (n_tok != 1) {
        fprintf(ui->out, "input error, only one number is allowed, got %d tokens\n", n_tok);
        return 0;
    }

    endptr = NULL;
    num = strtol(toks[0], &endptr, 10);
    if (*endptr) {
        fprintf(ui->out, "input not a valid number: %s\n", toks[0]);
        return 0;
    }

    if (num < range->begin || num > range->end) {
        fprintf(ui->out, "input number %d out of range [%ld, %ld]\n", num, range->begin, range->end);
        return 0;
    }

    *res = num;
    return 1;
}

/* @return: < 0 fatal err, == 0 try again, > 0 chosen */
int ui_selection_menu_prompt(struct ui *ui, const char *prompt, struct select *sel)
{
    int i;
    struct choice *choice;
    char *line = NULL;
    const char *toks[4];
    int n_tok;

    if (sel->n_choice <= 0)
        return -1;

    if (prompt)
        fprintf(ui->out, "%s", prompt);

    for (i = 0; i < sel->n_choice; i++) {
        choice = &sel->choices[i];
        if (choice->chosen)
            continue;
        if (!choice->id && !choice->alt_id)
            continue;

        fprintf(ui->out, "%c", choice->id);
        if (choice->alt_id)
            fprintf(ui->out, "|%c", choice->alt_id);

        if (choice->name)
            fprintf(ui->out, ") %s\n", choice->name);
        else
            fprintf(ui->out, ") %s\n", "NULL");
    }
    fprintf(ui->out, "input your choice? ");

    line = ui_read_line(ui);
    if (!line)
        return -1;

    n_tok = ui_cmd_tokenize(line, toks, ARRAY_SIZE(toks));
    if (n_tok == 0) {
        return 0;
    } else if (n_tok != 1) {
        fprintf(ui->out, "input error, only one choice is allowed, got %d tokens\n", n_tok);
        return 0;
    } else if (strlen(toks[0]) != 1) {
        fprintf(ui->out, "input error, only one choice is allowed, got %s\n", toks[0]);
        return 0;
    }

    for (i = 0; i < sel->n_choice; i++) {
        choice = &sel->choices[i];
        if (choice->chosen)
            continue;
        if (!choice->id && !choice->alt_id)
            continue;

        if (choice->id == toks[0][0])
            break;
        if (choice->alt_id && choice->alt_id == toks[0][0])
            break;
    }

    if (i < sel->n_choice) {
        choice->chosen = 1;
        sel->cur_choice = i;
        sel->n_selected++;
        return 1;
    }

    fprintf(ui->out, "input error, %s is not in one of the choices\n", toks[0]);
    return 0;
}


static const char node_render_tab[MAP_NODE_MAX] = {
    [MAP_NODE_START] = 'S',
    [MAP_NODE_VACANCY] = '0',
    [MAP_NODE_ITEM_HOUSE] = 'T',
    [MAP_NODE_GIFT_HOUSE] = 'G',
    [MAP_NODE_MAGIC_HOUSE] = 'M',
    [MAP_NODE_HOSPITAL] = 'H',
    [MAP_NODE_PRISON] = 'P',
    [MAP_NODE_MINE] = '$',
    [MAP_NODE_PARK] = 'P',
};

static void map_node_render(struct ui *ui, struct map *map, unsigned line, unsigned col)
{
    int pos = -1;
    struct map_node *node;
    struct player *player, *owner;

    if (line == 0) {
        pos = col;
    } else if (line == map->height - 1) {
        pos = map->n_used - (map->height - 1) - col;
    } else if (col == 0) {
        pos = map->n_used - line;
    } else if (col == map->width - 1) {
        pos = map->width + (line - 1);
    }

    if (pos < 0) {
        fputc(' ', ui->out);
        return;
    }

    if (pos >= map->n_used) {
        game_err("line %u column %u calculated node idx %d > map->n_used %d\n", line, col, pos, map->n_used);
        return;
    }

    /* player char always on top */
    node = &map->nodes[pos];
    if (!list_empty(&node->players)) {
        player = list_first_entry(&node->players, struct player, pos_list);
        if (ui->out_isatty) {
            fprintf(ui->out, "%s", VT100_MODE_BOLD);
            fprintf(ui->out, "%s", player_ui_color[player->color]);
        }

        fputc(player_id_to_char(player), ui->out);
        if (ui->out_isatty)
            fprintf(ui->out, "%s", VT100_MODES_OFF);
        return;
    }

    if (node->item > ITEM_INVALID && node->item < ITEM_MAX) {
        owner = node->item_owner;
        if (ui->out_isatty && owner)
            fprintf(ui->out, "%s", player_ui_color[owner->color]);

        fputc(item_ui_char[node->item], ui->out);
        if (ui->out_isatty && owner)
            fprintf(ui->out, "%s", VT100_MODES_OFF);
        return;
    }

    if (node->type != MAP_NODE_VACANCY || !node->estate.owner) {
        fputc(node_render_tab[node->type], ui->out);
        return;
    }
    owner = node->estate.owner;

    if (ui->out_isatty)
        fprintf(ui->out, "%s", player_ui_color[owner->color]);

    fputc(node_render_tab[node->type] + node->estate.level, ui->out);
    if (ui->out_isatty)
        fprintf(ui->out, "%s", VT100_MODES_OFF);
    return;
}

void ui_map_render(struct ui *ui, struct map *map)
{
    int line, col;

    if (!map->dirty)
        return;

    map->dirty = 0;

    if (ui_is_interactive(ui)) {
        if (ui->use_setwin) {
            fprintf(ui->out, VT100_SAVE_CURSOR);
            /*
             * clearbos also clears current character under cursor, to avoid clearing first character
             * of our prompt (starts at map->height + 2), seek to the line above, clear that line,
             * then do clearbos.
             */
            fprintf(ui->out, VT100_CURSOR_POS, map->height + 1, 0);
            fprintf(ui->out, VT100_CLEAR_EOL);
            fprintf(ui->out, VT100_CLEAR_BOS);
        } else {
            fprintf(ui->out, VT100_CLEAR_SCREEN);
        }
        fprintf(ui->out, VT100_CURSOR_HOME);
    }

    for (line = 0; line < map->height; line++) {
        for (col = 0; col < map->width; col++)
            map_node_render(ui, map, line, col);
        fputc('\n', ui->out);
    }

    if (ui_is_interactive(ui)) {
        fprintf(ui->out, "\n");
        if (ui->use_setwin)
            fprintf(ui->out, VT100_RESTORE_CURSOR);
    }
}

const char *ui_item_name(enum item_type type)
{
    if (type <= ITEM_INVALID || type >= ITEM_MAX)
        return "??";

    return item_ui_name[type];
}

int ui_dump_player_stats(struct ui *ui, const char *prompt, struct player *player)
{
    struct map_node *node;

    if (!player)
        return 0;

    fprintf(ui->out, "[%s] money: %d\n", prompt, player->asset.n_money);
    fprintf(ui->out, "[%s] points: %d\n", prompt, player->asset.n_points);
    fprintf(ui->out, "[%s] items:\n", prompt);
    fprintf(ui->out, "[%s]   barrier: %d\n", prompt, player->asset.n_block);
    fprintf(ui->out, "[%s]   bomb: %d\n", prompt, player->asset.n_bomb);
    fprintf(ui->out, "[%s]   robot: %d\n", prompt, player->asset.n_robot);
    fprintf(ui->out, "[%s] buff:\n", prompt);
    fprintf(ui->out, "[%s]   god of wealth: %d (%d rounds left)\n", prompt, player->stat.god, player->buff.n_god_rounds);
    fprintf(ui->out, "[%s]   empty: %d (%d rounds left)\n", prompt, player->stat.empty, player->buff.n_empty_rounds);

    if (list_empty(&player->asset.estates))
        return 0;

    fprintf(ui->out, "[%s] estates:\n", prompt);
    list_for_each_entry(node, &player->asset.estates, estate.estates_list) {
        fprintf(ui->out, "[%s]   house #%d, level %d, value %d\n", prompt, node->idx, node->estate.level, node->estate.price);
    }
    return 0;
}

void ui_on_game_start(struct ui *ui, struct map *map)
{
    if (!ui_is_interactive(ui))
        return;

    game_dbg("ui is interactive, ui %d lines, map %u lines\n", ui->lines, map->height);
    fprintf(ui->out, VT100_CLEAR_SCREEN);
    fprintf(ui->out, VT100_CURSOR_HOME);

    if (ui->lines >= 12 + map->height) {
        ui->use_setwin = 1;
        fprintf(ui->out, VT100_SETWIN, map->height + 2, 0);
        fprintf(ui->out, VT100_CURSOR_POS, map->height + 2, 0);
        game_dbg("using setwin\n");
    }
}

void ui_on_game_stop(struct ui *ui)
{
    if (!ui_is_interactive(ui))
        return;

    if (ui->use_setwin) {
        fprintf(ui->out, "%s%s%s", VT100_SAVE_CURSOR, VT100_RESETWIN, VT100_RESTORE_CURSOR);
    }
}
