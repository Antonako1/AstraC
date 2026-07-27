/**
 * STDLIB/DEBUG.h - Debug output shell for AstraC (hosted port)
 *
 * On your own OS replace with your kernel's debug print facility.
 * Licensed under the MIT License.
 */
#ifndef STDLIB_DEBUG_H
#define STDLIB_DEBUG_H
#include "TYPEDEF.h"
#include <stdio.h>

/* DEBUG_PRINTF — active only when DEBUG is defined */
#ifdef DEBUG
#  define AC_DEBUG_PRINTF(fmt, ...)  fprintf(stderr, "[DBG] " fmt, ##__VA_ARGS__)
#else
#  define AC_DEBUG_PRINTF(fmt, ...)  ((void)0)
#endif

#endif /* STDLIB_DEBUG_H */
