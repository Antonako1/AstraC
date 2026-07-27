/**
 * STDLIB/STRING.h - String utilities shell for AstraC (hosted port)
 *
 * Wraps string.h. On your own OS, replace with your kernel string library.
 * Licensed under the MIT License.
 */
#ifndef STDLIB_STRING_H
#define STDLIB_STRING_H
#include "TYPEDEF.h"
#include <string.h>
#include <ctype.h>

/* ── Standard string operations ─────────────────────────────────────────── */
#define AC_STRLEN(s)                strlen((const char*)(s))
#define AC_STRCMP(a, b)             strcmp((const char*)(a), (const char*)(b))
#define AC_STRCAT(dst, src)         strcat((char*)(dst), (const char*)(src))
#define AC_STRNCAT(dst, src, n)     strncat((char*)(dst), (const char*)(src), (size_t)(n))
#define AC_STRCPY(dst, src)         strcpy((char*)(dst), (const char*)(src))
#define AC_STRNCPY(dst, src, n)     strncpy((char*)(dst), (const char*)(src), (size_t)(n))
#define AC_STRRCHR(s, c)            ((PU8)strrchr((const char*)(s), (c)))
#define AC_STRCHR(s, c)             ((PU8)strchr((const char*)(s), (c)))
#define AC_STRSTR(hay, needle)      ((PU8)strstr((const char*)(hay), (const char*)(needle)))
#define AC_STRRCHR(s, c)            ((PU8)strrchr((const char*)(s), (c)))
/* ── Case-insensitive comparisons ──────────────────────────────────────── */
#ifdef _MSC_VER
#  include <string.h>
#  define AC_STRICMP(a, b)         _stricmp((const char*)(a), (const char*)(b))
#  define AC_STRNICMP(a, b, n)     _strnicmp((const char*)(a), (const char*)(b), (size_t)(n))
#  define AC_STRDUP(s)             _strdup((const char*)(s))
#else
#  include <strings.h>
#  define AC_STRICMP(a, b)         strcasecmp((const char*)(a), (const char*)(b))
#  define AC_STRNICMP(a, b, n)     strncasecmp((const char*)(a), (const char*)(b), (size_t)(n))
#  define AC_STRDUP(s)             strdup((const char*)(s))
#endif

/* ── Case-insensitive substring search ─────────────────────────────────── */
static PU8 AC_STRISTR(const char* haystack, const char* needle) {
    if (!needle || !*needle) return (PU8)haystack;
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && (tolower((unsigned char)*h) == tolower((unsigned char)*n))) {
            h++; n++;
        }
        if (!*n) return (PU8)haystack;
    }
    return (PU8)NULL;
}

/* ── String-to-uppercase in place ──────────────────────────────────────── */
static PU8 AC_STRUPR(char* s) {
    char* p = s;
    while (*p) { *p = (char)toupper((unsigned char)*p); p++; }
    return (PU8)s;
}

/* ── In-place trim (removes leading and trailing whitespace) ───────────── */
static void AC_STR_TRIM(char* s) {
    if (!s) return;
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' ||
                        s[len-1] == '\n' || s[len-1] == '\r'))
        s[--len] = '\0';
    char* p = s;
    while (*p == ' ' || *p == '\t') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

#endif /* STDLIB_STRING_H */
