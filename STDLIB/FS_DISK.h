/**
 * STDLIB/FS_DISK.h - File-system shell for AstraC (hosted port)
 *
 * Wraps stdio.h / POSIX/Win32 file operations.
 * On your own OS replace with your VFS/disk driver calls.
 * Licensed under the MIT License.
 */
#ifndef STDLIB_FS_DISK_H
#define STDLIB_FS_DISK_H
#include "TYPEDEF.h"
#include "IO.h"   /* brings in FILE, MODE_*, FOPEN, FCLOSE, FWRITE, ... */

#ifdef _WIN32
#include <direct.h>  
#elif __linux__
#include <sys/stat.h>
#else
#endif

/* ── File existence / create / delete ──────────────────────────────────── */
/*
 * FILE_EXISTS(path)  — TRUE if the file can be opened for reading.
 * FILE_DELETE(path)  — remove the file; returns TRUE on success.
 * FILE_CREATE(path)  — create an empty file (truncates if exists).
 *                      Returns TRUE on success.
 */

static BOOL FILE_EXISTS_IMPL(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return FALSE;
    fclose(f);
    return TRUE;
}
#define AC_FILE_EXISTS(path)  FILE_EXISTS_IMPL(path)

#define AC_FILE_DELETE(path)  ((BOOL)(remove(path) == 0))

static BOOL FILE_CREATE_IMPL(const char* path) {
    FILE* f = fopen(path, "wb");
    if (!f) {
        // try to create the directory if it doesn't exist
        const char* last_slash = AC_STRRCHR(path, '/');
        if (!last_slash) last_slash = AC_STRRCHR(path, '\\');
        if (!last_slash) return FALSE;  // no directory to create
        char dir_path[256];
        AC_STRNCPY(dir_path, path, (U32)(last_slash - path));
        dir_path[last_slash - path] = '\0';
        #ifdef _WIN32
        _mkdir(dir_path);
        #elif __linux__
        mkdir(dir_path, 0755);
        #else
        #endif
        f = fopen(path, "wb");
        if (!f) return FALSE;
    }
    fclose(f);
    return TRUE;
}
#define AC_FILE_CREATE(path)  FILE_CREATE_IMPL(path)



/* ── File mode flags (bit-combined, e.g. MODE_R | MODE_FAT32) ─────────── */
#define MODE_R      0x01    /* read                                          */
#define MODE_W      0x02    /* write / create-truncate                       */
#define MODE_RA     0x04    /* random access  (read + write, file must exist) */
#define MODE_FA     0x08    /* full access    (read + write, file must exist) */
#define MODE_FAT32  0x00    /* filesystem type flag — ignored on host        */

/* ── File open / close ──────────────────────────────────────────────────── */
/*
 * FOPEN(path, mode_flags)
 *   mode_flags is an OR of MODE_* constants.
 *   Maps to the appropriate fopen() mode string.
 *   Declared here; defined in FS_DISK.c.
 */
FILE* AC_FOPEN(const char* path, int mode_flags);

#define AC_FCLOSE(f)    fclose(f)
#define AC_FFLUSH(f)    fflush(f)
#define AC_FEOF(f)      feof(f)

/* ── File read / write ──────────────────────────────────────────────────── */
/*
 * FWRITE(file, buf, len)  → writes `len` bytes from buf; returns TRUE on success.
 * FREAD(file, buf, len)   → reads  `len` bytes into buf; returns bytes actually read.
 * FILE_GET_LINE(f, buf, sz) → fgets wrapper; returns TRUE if a line was read.
 */
#define AC_FWRITE(file, buf, len)          ((I32)(fwrite((buf), 1, (len), (file)) == (size_t)(len)))
#define AC_FREAD(file, buf, len)           ((U32)fread((buf), 1, (len), (file)))
#define AC_FILE_GET_LINE(file, buf, sz)    (fgets((char*)(buf), (int)(sz), (file)) != NULL)

/* ── Standard streams ───────────────────────────────────────────────────── */
/* stdin / stdout / stderr provided by stdio.h */

#endif /* STDLIB_FS_DISK_H */
