#ifndef __KIRC_ANSI_H
#define __KIRC_ANSI_H

#define CR  0x0D
#define ESC 0x1B

#define ANSI_RESET          "\x1b[0m"
#define ANSI_BOLD           "\x1b[1m"
#define ANSI_DIM            "\x1b[2m"
#define ANSI_REVERSE        "\x1b[7m"
#define ANSI_RED            "\x1b[31m"
#define ANSI_GREEN          "\x1b[32m"
#define ANSI_YELLOW         "\x1b[33m"
#define ANSI_BLUE           "\x1b[34m"
#define ANSI_MAGENTA        "\x1b[35m"
#define ANSI_CYAN           "\x1b[36m"
#define ANSI_BOLD_RED       "\x1b[1;31m"
#define ANSI_BOLD_GREEN     "\x1b[1;32m"
#define ANSI_BOLD_YELLOW    "\x1b[1;33m"
#define ANSI_BOLD_BLUE      "\x1b[1;34m"
#define ANSI_BOLD_MAGENTA   "\x1b[1;35m"
#define ANSI_BOLD_CYAN      "\x1b[1;36m"
#define ANSI_CLEAR_LINE     "\x1b[0K"
#define ANSI_CURSOR_HOME    "\x1b[H"

#endif  // __KIRC_ANSI_H
