#pragma once

/* Clear screen from cursor down */
#define VT100_CLEAR_EOS "\e[0J"
/* Clear screen from cursor up */
#define VT100_CLEAR_BOS "\e[1J"
/* Clear entire screen */
#define VT100_CLEAR_SCREEN "\e[2J"

#define VT100_CLEAR_EOL "\e[K"

#define VT100_SETWIN_FMT "\e[%u;%ur"
#define VT100_RESETWIN "\e[;r"

#define VT100_CURSOR_POS "\e[%u;%uH"
#define VT100_CURSOR_HOME "\e[H"
#define VT100_SAVE_CURSOR "\e7"
#define VT100_RESTORE_CURSOR "\e8"

#define VT100_MODES_OFF "\e[0m"
#define VT100_MODE_BOLD "\e[1m"

#define VT100_COLOR_RED "\e[31m"
#define VT100_COLOR_GREEN "\e[32m"
#define VT100_COLOR_BLUE "\e[34m"
#define VT100_COLOR_YELLOW "\e[33m"
#define VT100_COLOR_WHITE "\e[37m"
