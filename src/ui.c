#include "common.h"
#include "ui.h"

int prompt(void)
{
    fprintf(stdout, "input> ");
    return 0;
}

void discard_line(FILE *where)
{
    int c;

    while ((c = getc(where)) != EOF) {
        if (c == '\n')
            return;
    }
}

/* @return NULL if eof, empty string if nothing is read */
const char *grab_line(FILE *where, char *buf, unsigned int size)
{
    const char *ret;

    if (size < 2) {
        game_err("input buffer too small, can't read\n");
        return "";
    }
    /* guard */
    buf[size - 2] = 0;

    prompt();

    ret = fgets(buf, size, where);
    if (!ret) {
        if (feof(where))
            return NULL;
        game_err("fail to read input\n");
        return "";
    }

    /* check guard */
    if (buf[size - 2] && buf[size - 2] != '\n') {
        game_err("line length exceeds maximum %d characters\n", size - 2);
        buf[size - 2] = '\n';
        discard_line(where);
    }
    return ret;
}
