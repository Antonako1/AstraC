/* STDLIB/FS_DISK.c — filesystem shell; helpers defined inline in FS_DISK.h */
#include "FS_DISK.h"

/*
 * FOPEN — convert MODE_* flag combination to an fopen() mode string.
 *
 *   MODE_W            → "w"    (write text, truncate)
 *   MODE_RA | MODE_FA → "r+b"  (random/full access, binary, file must exist)
 *   MODE_R (default)  → "rb"   (read binary)
 *
 * The MODE_FAT32 flag is 0 and has no effect on the selection.
 */
FILE* AC_FOPEN(const char* path, int mode_flags) {
    if (mode_flags & MODE_W)             return fopen(path, "w");
    if (mode_flags & (MODE_RA | MODE_FA)) return fopen(path, "r+b");
    return fopen(path, "rb");   /* MODE_R or any unknown combination */
}
