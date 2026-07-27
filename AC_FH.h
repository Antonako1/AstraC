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
#define AC_FILE_RESERVED_SIZE 64
#define AC_FILE_VERSION_MAJOR ((U16)1)
#define AC_FILE_VERSION_MINOR ((U16)0)
#define AC_FILE_VERSION       ((U32)AC_FILE_VERSION_MAJOR << 16 | AC_FILE_VERSION_MINOR)

enum {
    AC_FLAG_NONE       = 0,
    AC_FLAG_EXECUTABLE = 1 << 0,
    AC_FLAG_DYNAMIC    = 1 << 1,
};

typedef struct {
    U8  magic[AC_FILE_MAGIC_LEN];
    U32 version;
    U32 flags;
    U32 entry_point_offset;
    U32 code_offset;
    U32 data_offset;
    U32 rodata_offset;
    U32 bss_offset;
    U32 code_size;
    U32 data_size;
    U32 rodata_size;
    U32 bss_size;
    U8  reserved[AC_FILE_RESERVED_SIZE];
} ATTRIB_PACKED AC_FILE_HEADER;

#endif /* AC_FH_H */
