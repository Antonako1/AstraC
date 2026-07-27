/**
 * STDLIB/MEM.h - Memory management shell for AstraC (hosted port)
 *
 * Wraps stdlib.h malloc/free/realloc + string.h memset/memcpy.
 * On your own OS, replace with your kernel memory allocator.
 * Licensed under the MIT License.
 */
#ifndef STDLIB_MEM_H
#define STDLIB_MEM_H
#include "TYPEDEF.h"
#include <stdlib.h>
#include <string.h>

/* ── Allocation ─────────────────────────────────────────────────────────── */
#define AC_MAlloc(sz)           malloc((size_t)(sz))
#define AC_MFree(ptr)           free(ptr)
#define AC_ReAlloc(ptr, sz)     realloc((ptr), (size_t)(sz))

/* ── Memory operations ──────────────────────────────────────────────────── */
#define AC_MEMSET(dst, val, sz)         memset((dst), (val), (size_t)(sz))
#define AC_MEMCPY(dst, src, sz)         memcpy((dst), (src), (size_t)(sz))
#define AC_MEMCPY_OPT(dst, src, sz)     memcpy((dst), (src), (size_t)(sz))
#define AC_MEMZERO(dst, sz)             memset((dst), 0, (size_t)(sz))
#define AC_MEMMOVE(dst, src, sz)        memmove((dst), (src), (size_t)(sz))

#endif /* STDLIB_MEM_H */
