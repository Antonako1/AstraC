/**
 * STDLIB/TYPEDEF.h - Type definitions for AstraC (hosted port)
 *
 * This is the STDLIB shell layer. On your own OS, replace with your own TYPEDEF.h.
 * Licensed under the MIT License. See LICENSE file in the project root for full license information.
 */
#ifndef TYPEDEF_H
#define TYPEDEF_H

/* ── Integer types ─────────────────────────────────────────────────────── */
typedef unsigned char       U8;
typedef unsigned short      U16;
typedef unsigned int        U32;

typedef signed char         I8;
typedef signed short        I16;
typedef signed int          I32;
typedef signed int          S32;  /* signed 32-bit alias */

typedef float               F32;

/* ── Pointer types ─────────────────────────────────────────────────────── */
typedef unsigned char*      PU8;
typedef unsigned short*     PU16;
typedef unsigned int*       PU32;
typedef signed char*        PI8;
typedef signed short*       PI16;
typedef signed int*         PI32;

typedef float*              PF32;

typedef PU8*                PPU8;
typedef PU16*               PPU16;
typedef PU32*               PPU32;
typedef PI8*                PPI8;
typedef PI16*               PPI16;
typedef PI32*               PPI32;

/* ── Boolean ───────────────────────────────────────────────────────────── */
#define BOOL       I32
#define BOOLEAN    I32
#define BOOL8      I8

#define TRUE  1
#define FALSE 0

/* ── Null ──────────────────────────────────────────────────────────────── */
#ifndef NULL
#define NULL    0
#endif
#define NULLPTR 0

/* ── Void aliases ──────────────────────────────────────────────────────── */
#define VOID    void
#define VOIDPTR void*
#define U0      void       /* AstraC language void type */

/* ── Storage / linkage qualifiers ─────────────────────────────────────── */
#define STATIC   static
#define CONST    const
#define RETURN   return    /* OS may do cleanup here; on host just return */

/* ── Section placement attributes ─────────────────────────────────────── */
/* On the target OS these place symbols in specific linker sections.        */
/* On the host they are no-ops.                                             */
#define ATTRIB_DATA
#define ATTRIB_RODATA

/* Packed struct: GCC/Clang attribute; MSVC uses #pragma pack elsewhere.   */
#ifdef _MSC_VER
#  define ATTRIB_PACKED
#else
#  define ATTRIB_PACKED __attribute__((packed))
#endif

/* ── Numeric limits ────────────────────────────────────────────────────── */
#define MAX_U8   0xFF
#define MAX_U16  0xFFFF
#define MAX_U32  0xFFFFFFFFU
#define MAX_I8   0x7F
#define MAX_I16  0x7FFF
#define MAX_I32  0x7FFFFFFF
#define MIN_I8   ((I8)0x80)
#define MIN_I16  ((I16)0x8000)
#define MIN_I32  ((I32)0x80000000)
#define MAX_F32  3.402823466e+38F
#define MIN_F32  1.175494351e-38F

#define U32_MAX  MAX_U32

/* ── Character classification ──────────────────────────────────────────── */
#define AC_IS_DIGIT(c)   ((U8)(c) >= '0' && (U8)(c) <= '9')
#define AC_IS_UPPER(c)   ((U8)(c) >= 'A' && (U8)(c) <= 'Z')
#define AC_IS_LOWER(c)   ((U8)(c) >= 'a' && (U8)(c) <= 'z')
#define AC_IS_ALPHA(c)   (AC_IS_UPPER(c) || AC_IS_LOWER(c) || (U8)(c) == '_')
#define AC_IS_ASCII(c)   ((U8)(c) >= 0x20 && (U8)(c) <= 0x7E)
#define AC_IS_ALNUM(c)   (AC_IS_ALPHA(c) || AC_IS_DIGIT(c))
#define AC_IS_HEXDIG(c)  (AC_IS_DIGIT(c) || ((U8)(c)>='A'&&(U8)(c)<='F') || ((U8)(c)>='a'&&(U8)(c)<='f'))
#define AC_IS_SPACE(c)   ((U8)(c) == ' ' || (U8)(c) == '\t' || (U8)(c) == '\n' || (U8)(c) == '\r')

#endif /* TYPEDEF_H */