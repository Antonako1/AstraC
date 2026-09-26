/**
 * AC_FH.h — AstraC binary file header format definition.
 *
 * Swap-in on your own OS by replacing this file.
 */
#ifndef AC_FH_H
#define AC_FH_H
#include "STDLIB/TYPEDEF.h"

#define OFFSET_NON_EXISTENT ((U32)(-1))

#define AC_FILE_MAGIC         "ACFH"
#define AC_FILE_MAGIC_LEN     4
#define AC_FILE_RESERVED_SIZE 48
#define AC_FILE_VERSION_MAJOR ((U16)1)
#define AC_FILE_VERSION_MINOR ((U16)0)
#define AC_FILE_VERSION       ((U32)AC_FILE_VERSION_MAJOR << 16 | AC_FILE_VERSION_MINOR)

enum {
    AC_FLAG_NONE       = 0,
    AC_FLAG_EXECUTABLE = 1 << 0,
    AC_FLAG_DYNAMIC    = 1 << 1,
    AC_FLAG_HAS_RELOCS = 1 << 2,
    AC_FLAG_HAS_FUNCS  = 1 << 3,   /* Binary includes a function export table */
};

enum {
    AC_FUNC_FLAG_CDECL    = 0 << 0,  /* Default cdecl ABI (caller cleans stack) */
    AC_FUNC_FLAG_STDCALL  = 1 << 0,  /* stdcall ABI (callee cleans stack) */
    AC_FUNC_FLAG_VARIADIC = 1 << 1,  /* Variadic function (...) */
};

typedef struct {
    U32 entry_count;             /* Number of exported function entries */
    U32 string_table_size;       /* Size of the string table pool in bytes */
} ATTRIB_PACKED AC_FUNC_TABLE_HDR;

typedef struct {
    U32 address;                 /* Function code-relative offset */
    U32 name_offset;             /* Byte offset into the function string table pool */
    U16 param_size;              /* Total parameter size in bytes (e.g. 12 for 3 dwords) */
    U16 flags;                   /* Calling convention & function attributes */
} ATTRIB_PACKED AC_FUNC_ENTRY;

typedef struct {
    U8  magic[AC_FILE_MAGIC_LEN];
    U32 version;
    U32 flags;
    U32 entry_point_offset;
    U32 code_offset;
    U32 data_offset;
    U32 rodata_offset;
    U32 bss_offset;
    U32 reloc_offset;
    U32 code_size;
    U32 data_size;
    U32 rodata_size;
    U32 bss_size;
    U32 reloc_size;
    U32 func_table_offset;       /* File offset to function export table header */
    U32 func_table_size;         /* Total size of function export table section in bytes */
    U8  reserved[AC_FILE_RESERVED_SIZE];
} ATTRIB_PACKED AC_FILE_HEADER;

#endif /* AC_FH_H */
