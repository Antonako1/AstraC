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



/* ── File mode flags (bit-combined, e.g. MODE_R | MODE_FAT32) ─────────── 
    These are copied from atOS/STD/FS_DISK.h.
    Do not modify them here, modify them inside wrapper functions if needed for your OS.
*/
typedef enum {
    MODE_R        = 0x0001,  // Read
    MODE_W        = 0x0002,  // Write
    MODE_RW       = MODE_R | MODE_W,   // Read & Write
    MODE_A        = 0x0008,  // Append
    MODE_RA       = MODE_R | MODE_A,
    MODE_FAT32    = 0x0100,  // FAT32 backend
    MODE_ISO9660  = 0x0200,  // ISO9660 backend
    MODE_FR       = MODE_R | MODE_FAT32, // FAT32 Read
    MODE_FW       = MODE_W | MODE_FAT32, // FAT32 Write
    MODE_FRW      = MODE_RW| MODE_FAT32, // FAT32 Read & Write
    MODE_FA       = MODE_A | MODE_FAT32, // FAT32 Append
    MODE_FRA      = MODE_RA| MODE_FAT32, // FAT32 Read & Append
} FILEMODES;

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

/** ── File size / existence ─────────────────────────────────────────────────────
 *
 * AC_FSIZE(file)       — returns the current file position (size in bytes).
 * AC_FILE_EXISTS(path) — TRUE if the file can be opened for reading.
 * AC_FILE_CREATE(path) — create an empty file (truncates if exists)
 *  Returns TRUE on success. 
 * AC_FILE_DELETE(path) — remove the file; returns TRUE on success.
 */
#define AC_FSIZE(file)                      ((U32)ftell(file))
#define AC_FILE_EXISTS(path)                FILE_EXISTS_IMPL(path)
#define AC_FILE_CREATE(path)                FILE_CREATE_IMPL(path)
#define AC_FILE_DELETE(path)                ((BOOL)(remove(path) == 0))


/* ── Standard streams ───────────────────────────────────────────────────── */
/* stdin / stdout / stderr provided by stdio.h */

#endif /* STDLIB_FS_DISK_H */
