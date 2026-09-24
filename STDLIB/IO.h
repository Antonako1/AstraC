/**
 * STDLIB/IO.h - I/O shell for AstraC (hosted port)
 *
 * Wraps stdio.h. On your own OS, replace with your kernel's I/O calls.
 * Licensed under the MIT License.
 */
#ifndef STDLIB_IO_H
#define STDLIB_IO_H
#include "TYPEDEF.h"
#include <stdio.h>
#include <stdarg.h>


/* ── FILE type (re-export stdio's FILE) ────────────────────────────────── */
/* FILE is already defined by stdio.h */

/* ── Text formatting ────────────────────────────────────────────────────── */
#define AC_SPRINTF(buf, fmt, ...)   sprintf((char*)(buf), (fmt), ##__VA_ARGS__)
#define AC_PRINTF(fmt, ...)         printf(fmt, ##__VA_ARGS__)

/* ── Terminal colours (ANSI SGR) ────────────────────────────────────────── */
#define AC_CLR_RED      "\033[31m"
#define AC_CLR_YELLOW   "\033[33m"
#define AC_CLR_RESET    "\033[0m"

/* Coloured diagnostic helpers: errors in red, warnings in yellow. */
#define AC_PRINTF_ERR(fmt, ...)    AC_PRINTF(AC_CLR_RED fmt AC_CLR_RESET, ##__VA_ARGS__)
#define AC_PRINTF_WARN(fmt, ...)   AC_PRINTF(AC_CLR_YELLOW fmt AC_CLR_RESET, ##__VA_ARGS__)


#endif /* STDLIB_IO_H */
