/**
 * STDLIB/BINARY.h - Number-parsing utilities for AstraC (hosted port)
 *
 * Provides ATOI_E, ATOI_HEX_E, ATOI_BIN_E with error returns.
 * On your own OS these may be kernel intrinsics; here they are portable C.
 * Licensed under the MIT License.
 */
#ifndef STDLIB_BINARY_H
#define STDLIB_BINARY_H
#include "TYPEDEF.h"

/*
 * ATOI_E(str, out)
 *   Parse a decimal integer string into *out.
 *   Returns TRUE on success, FALSE if the string is not a valid decimal.
 */
static BOOL AC_ATOI_E(const char* str, U32* out) {
    if (!str || !*str) return FALSE;
    const char* p = str;
    if (*p == '+') p++;
    if (!(*p >= '0' && *p <= '9')) return FALSE;
    U32 v = 0;
    while (*p >= '0' && *p <= '9') { v = v * 10 + (U32)(*p - '0'); p++; }
    if (*p != '\0') return FALSE;
    *out = v;
    return TRUE;
}

/*
 * ATOI_HEX_E(str, out)
 *   Parse a hexadecimal string (without 0x prefix) into *out.
 *   Returns TRUE on success.
 */
static BOOL AC_ATOI_HEX_E(const char* str, U32* out) {
    if (!str || !*str) return FALSE;
    const char* p = str;
    U32 v = 0;
    while (*p) {
        U8 c = (U8)*p;
        U32 d;
        if (c >= '0' && c <= '9')      d = (U32)(c - '0');
        else if (c >= 'a' && c <= 'f') d = (U32)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = (U32)(c - 'A' + 10);
        else return FALSE;
        v = (v << 4) | d;
        p++;
    }
    *out = v;
    return TRUE;
}

/*
 * ATOI_BIN_E(str, out)
 *   Parse a binary string (without 0b prefix) into *out.
 *   Returns TRUE on success.
 */
static BOOL AC_ATOI_BIN_E(const char* str, U32* out) {
    if (!str || !*str) return FALSE;
    const char* p = str;
    U32 v = 0;
    while (*p) {
        if (*p != '0' && *p != '1') return FALSE;
        v = (v << 1) | (U32)(*p - '0');
        p++;
    }
    *out = v;
    return TRUE;
}

#endif /* STDLIB_BINARY_H */
