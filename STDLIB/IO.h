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
#define AC_FPRINTF(file, fmt, ...)  fprintf(file, fmt, ##__VA_ARGS__)
#define AC_PRINTF(fmt, ...)         printf(fmt, ##__VA_ARGS__)


#endif /* STDLIB_IO_H */
