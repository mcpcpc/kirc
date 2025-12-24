/*
 * ansi.h
 * ANSI escape codes and constants
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_ANSI_H
#define __KIRC_ANSI_H

#define SOH          0x01
#define STX          0x02
#define ETX          0x03
#define EOT          0x04
#define ENQ          0x05
#define ACK          0x06
#define BEL          0x07
#define HT           0x09
#define LF           0x0A
#define CR           0x0D
#define NAK          0x15
#define ESC          0x1B
#define DEL          0x7F

#define RESET        "\x1b[0m"
#define BOLD         "\x1b[1m"
#define DIM          "\x1b[2m"
#define REVERSE      "\x1b[7m"
#define RED          "\x1b[31m"
#define GREEN        "\x1b[32m"
#define YELLOW       "\x1b[33m"
#define BLUE         "\x1b[34m"
#define MAGENTA      "\x1b[35m"
#define CYAN         "\x1b[36m"
#define BOLD_RED     "\x1b[1;31m"
#define BOLD_GREEN   "\x1b[1;32m"
#define BOLD_YELLOW  "\x1b[1;33m"
#define BOLD_BLUE    "\x1b[1;34m"
#define BOLD_MAGENTA "\x1b[1;35m"
#define BOLD_CYAN    "\x1b[1;36m"
#define CLEAR_LINE   "\x1b[0K"
#define CURSOR_HOME  "\x1b[H"

#endif  // __KIRC_ANSI_H
