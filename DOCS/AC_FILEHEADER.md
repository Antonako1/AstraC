# ACFH Binary Specification & OS Loader Architecture

The **ACFH** (AstraC File Header) format is the native executable and dynamic library binary container emitted by AstraC when `exe` or `lib` mode is enabled. It is designed to be lightweight, deterministic, and easy to parse in kernel-space or user-space OS loaders without third-party dependencies.

---

## 1. File Layout Overview

An ACFH binary file consists of a fixed 112-byte (0x70) header followed by up to six contiguous on-disk sections, plus a memory-only `.bss` region immediately following the file in RAM:

```text
+-------------------------------------------------------------+
| AC_FILE_HEADER (112 bytes / 0x70)                           |
+-------------------------------------------------------------+
| Code Section (.code)                                        |
+-------------------------------------------------------------+
| Data Section (.data)                                        |
+-------------------------------------------------------------+
| Read-Only Data Section (.rodata)                            |
+-------------------------------------------------------------+
| Relocation Table Section (reloc_table / ot)                 |
+-------------------------------------------------------------+
| Function Export Table Section (function_table / ft)         |
+-------------------------------------------------------------+
| Import Table Section (import_table / it)                    |
+=============================================================+ <-- End of file on disk (bss_offset)
| Uninitialized Data Section (.bss) [0 bytes on disk, in RAM] |
+-------------------------------------------------------------+
```

Because flat in-place OS loaders (such as atOS) read the entire ACFH binary file directly into memory at `load_base`, `.bss` virtual addresses are placed **after** all trailing ACFH tables (`bss_offset` equals the total file size on disk) so zeroing `[load_base + bss_offset .. load_base + bss_offset + bss_size)` in RAM never overwrites the relocation, function export, or import tables.

---

## 2. Master Header Structure

Defined in `AC_FH.h`:

```c
typedef struct {
    U8  magic[4];                /* "ACFH" magic signature */
    U32 version;                 /* Format version (1.0 = 0x00010000) */
    U32 flags;                   /* Binary capability flags bitmask */
    U32 entry_point_offset;      /* Binary-relative offset to entry point (_start / _main) */
    U32 code_offset;             /* File offset to .code section */
    U32 data_offset;             /* File offset to .data section */
    U32 rodata_offset;           /* File offset to .rodata section */
    U32 bss_offset;              /* Memory offset to .bss section (end of file on disk) */
    U32 reloc_offset;            /* File offset to Relocation Table */
    U32 func_table_offset;       /* File offset to Function Export Table */
    U32 import_table_offset;     /* File offset to Import Table */
    U32 code_size;               /* Size of .code section in bytes */
    U32 data_size;               /* Size of .data section in bytes */
    U32 rodata_size;             /* Size of .rodata section in bytes */
    U32 bss_size;                /* Uninitialized .bss size in bytes (0 disk bytes, memory-only) */
    U32 reloc_size;              /* Size of Relocation Table in bytes */
    U32 func_table_size;         /* Size of Function Export Table section in bytes */
    U32 import_table_size;       /* Size of Import Table section in bytes */
    U8  reserved[40];            /* Reserved bytes (padded to 112 bytes total / 0x70) */
} ATTRIB_PACKED AC_FILE_HEADER;
```

### Binary Flags (`flags` field)
| Flag Bit | Constant | Value | Description |
|---|---|---|---|
| Bit 0 | `AC_FLAG_EXECUTABLE` | `1 << 0` (0x01) | Executable binary program |
| Bit 1 | `AC_FLAG_DYNAMIC` | `1 << 1` (0x02) | Dynamic shared library (`.lib`) |
| Bit 2 | `AC_FLAG_HAS_RELOCS` | `1 << 2` (0x04) | Relocation offset table present |
| Bit 3 | `AC_FLAG_HAS_FUNCS` | `1 << 3` (0x08) | Function export table present |
| Bit 4 | `AC_FLAG_HAS_IMPORTS` | `1 << 4` (0x10) | Dynamic import table present |

---

## 3. Relocation Table (`reloc_table` / `ot`)

### Purpose
The **Relocation Table** stores 32-bit section-relative byte offsets for every **internal absolute address reference** in `.code`, `.data`, and `.rodata`. When the OS loader loads an ACFH executable or library at a dynamic base address (e.g., `0x1000000` instead of `0x0`), it iterates through the Relocation Table and adds the load base address to each specified location.

### Triggers (What Requires Relocation)
- **Data Section Pointers**: Absolute address initializers stored in `.data` or `.rodata`, e.g.:
  ```c
  U32 global_var = 42;
  PU32 pvar = &global_var;     // .data DWORD initialized to absolute address of global_var
  PU8 str = "Hello World";     // .data DWORD initialized to absolute address of string
  ```
- **Instruction Absolute Addresses**: Absolute address operands in machine code instructions, e.g.:
  ```assembly
  MOV EAX, [_my_global]        ; ModR/M displacement field = absolute address of _my_global
  LEA EAX, [_my_global]        ; ModR/M displacement field = absolute address of _my_global
  MOV EAX, _my_global          ; Immediate field = absolute address of _my_global
  PUSH _str1                   ; Immediate field = absolute address of string _str1
  MOV EAX, _foo                ; Immediate field = absolute address of function _foo
  ```

### Exclusions (What Does NOT Require Relocation)
- **PC-Relative Jumps/Calls (`JMP label`, `CALL label`)**: Emitted as relative displacements ($target - (EIP + 5)$). Because the distance between instructions remains identical regardless of load base, relative branches require no relocations.
- **Dynamic Imports**: External dynamic library function references are handled separately by the **Import Table**.

### Format & Table Structure
The relocation table is an array of unsigned 32-bit integers (`U32 relocs[count]`), where `count = reloc_size / sizeof(U32)`:

```text
Offset 0x00: U32 rel_off_0   (Section-relative byte offset 0)
Offset 0x04: U32 rel_off_1   (Section-relative byte offset 1)
...
```

### OS Loader Relocation Algorithm
When loading a binary at `load_base` (e.g. `0x1000000`):

```c
VOID relocate_acfh_binary(U8* payload_base, U32 load_base, AC_FILE_HEADER* hdr) {
    if ((hdr->flags & AC_FLAG_HAS_RELOCS) && hdr->reloc_offset != OFFSET_NON_EXISTENT && hdr->reloc_size > 0) {
        U32* reloc_array = (U32*)(payload_base + hdr->reloc_offset);
        U32 count = hdr->reloc_size / sizeof(U32);

        for (U32 i = 0; i < count; i++) {
            U32 patch_section_offset = reloc_array[i];
            
            /* Add load_base to the 32-bit DWORD located at (payload_base + code_offset + patch_section_offset) */
            U32* patch_location = (U32*)(payload_base + hdr->code_offset + patch_section_offset);
            *patch_location += load_base;
        }
    }
}
```

---

## 4. Function Export Table (`function_table` / `ft`)

### Purpose
The **Function Export Table** allows dynamic libraries (`.lib`) or multi-tasking executables to export public function symbols, parameter stack sizes, and ABI calling conventions for external consumption.

### Structure Definitions

```c
typedef struct {
    U32 entry_count;             /* Number of exported function entries */
    U32 string_table_size;       /* Size of the string table pool in bytes */
} ATTRIB_PACKED AC_FUNC_TABLE_HDR;

typedef struct {
    U32 address;                 /* Function code-relative offset */
    U32 name_offset;             /* Byte offset into the function string table pool */
    U16 param_size;              /* Total parameter size in bytes (e.g., 12 for 3 DWORDs) */
    U16 flags;                   /* Calling convention & function attributes */
} ATTRIB_PACKED AC_FUNC_ENTRY;
```

### Function Entry Flags (`flags` field)
| Flag Bit | Constant | Value | Description |
|---|---|---|---|
| Bit 0 | `AC_FUNC_FLAG_CDECL` | `0 << 0` (0x00) | Standard cdecl ABI (caller cleans stack) |
| Bit 0 | `AC_FUNC_FLAG_STDCALL` | `1 << 0` (0x01) | stdcall ABI (callee cleans stack) |
| Bit 1 | `AC_FUNC_FLAG_VARIADIC` | `1 << 1` (0x02) | Variadic function (`...`) |

### String Table Pool
The header and array of `AC_FUNC_ENTRY` items are immediately followed by a null-terminated string table pool containing export function names (`"add_numbers\0multiply_numbers\0main\0"`).

### OS Loader Export Symbol Lookup Algorithm

```c
U32 resolve_export_symbol(U8* lib_payload, AC_FILE_HEADER* hdr, const char* symbol_name) {
    if (!(hdr->flags & AC_FLAG_HAS_FUNCS) || hdr->func_table_offset == OFFSET_NON_EXISTENT)
        return 0;

    AC_FUNC_TABLE_HDR* fhdr = (AC_FUNC_TABLE_HDR*)(lib_payload + hdr->func_table_offset);
    AC_FUNC_ENTRY* entries = (AC_FUNC_ENTRY*)((U8*)fhdr + sizeof(AC_FUNC_TABLE_HDR));
    const char* strtab = (const char*)((U8*)entries + (fhdr->entry_count * sizeof(AC_FUNC_ENTRY)));

    for (U32 i = 0; i < fhdr->entry_count; i++) {
        const char* name = strtab + entries[i].name_offset;
        if (strcmp(name, symbol_name) == 0) {
            /* Return code-relative function address */
            return entries[i].address;
        }
    }
    return 0; /* Symbol not found */
}
```

---

## 5. Import Table (`import_table` / `it`)

### Purpose
The **Import Table** lists external shared library dependencies (`graphics.lib`, `libc.lib`) and external function names required by the binary, providing exact instruction patch offsets for the OS loader to apply dynamic linkage.

### Structure Definitions

```c
typedef struct {
    U32 entry_count;             /* Number of imported function entries */
    U32 string_table_size;       /* Size of the import string table pool in bytes */
} ATTRIB_PACKED AC_IMPORT_TABLE_HDR;

typedef struct {
    U32 lib_name_offset;         /* Byte offset into string pool for library name (e.g. "graphics.lib") */
    U32 func_name_offset;        /* Byte offset into string pool for function name (e.g. "draw_rect") */
    U32 patch_offset;            /* File offset in binary where loader patches the function address */
} ATTRIB_PACKED AC_IMPORT_ENTRY;
```

### String Table Pool
Follows the array of `AC_IMPORT_ENTRY` structures, storing null-terminated library names and function names (`"graphics.lib\0draw_rect\0clear_screen\0libc.lib\0malloc\0"`).

### OS Loader Dynamic Linker Algorithm
When an OS loader links imported functions at startup:

```c
VOID link_acfh_imports(U8* app_payload, U32 app_load_base, AC_FILE_HEADER* app_hdr) {
    if (!(app_hdr->flags & AC_FLAG_HAS_IMPORTS) || app_hdr->import_table_offset == OFFSET_NON_EXISTENT)
        return;

    AC_IMPORT_TABLE_HDR* ihdr = (AC_IMPORT_TABLE_HDR*)(app_payload + app_hdr->import_table_offset);
    AC_IMPORT_ENTRY* entries = (AC_IMPORT_ENTRY*)((U8*)ihdr + sizeof(AC_IMPORT_TABLE_HDR));
    const char* strtab = (const char*)((U8*)entries + (ihdr->entry_count * sizeof(AC_IMPORT_ENTRY)));

    for (U32 i = 0; i < ihdr->entry_count; i++) {
        const char* lib_name  = strtab + entries[i].lib_name_offset;
        const char* func_name = strtab + entries[i].func_name_offset;

        /* 1. Resolve library base and target function address in kernel OS loader */
        U8* lib_payload = load_library_binary(lib_name);
        AC_FILE_HEADER* lib_hdr = (AC_FILE_HEADER*)lib_payload;
        U32 func_code_off = resolve_export_symbol(lib_payload, lib_hdr, func_name);
        U32 absolute_func_addr = (U32)lib_payload + lib_hdr->code_offset + func_code_off;

        /* 2. Compute patch target address in application payload */
        U32 patch_file_off = entries[i].patch_offset;
        U8* patch_location = app_payload + patch_file_off;

        /* 3. Patch relative CALL displacement (E8 rel32) or absolute pointer */
        U32 current_eip = (U32)patch_location + 4;
        S32 rel_displacement = (S32)absolute_func_addr - (S32)current_eip;
        *(S32*)patch_location = rel_displacement;
    }
}
```

---

## 6. CLI Invocation & Inspection

### Generating ACFH Binary Headers
Binary header emission is enabled via CLI flags:
```bash
# Output executable with relocation offset table and function export table
AstraC comp program.ac type exe ot ft

# Output dynamic library with function export table and import table
AstraC comp library.ac type lib ft it

# Output executable with all tables
AstraC comp app.ac type exe ot ft it
```

Table shortcuts:
- `offset_table` / `ot`: Relocation offset table
- `function_table` / `ft`: Function export table
- `import_table` / `it`: Import table

### Inspection with `objdump`
Inspect binary headers, section ranges, relocations, export tables, and import tables:
```bash
AstraC objdump program.BIN all
AstraC objdump program.BIN relocs
AstraC objdump program.BIN funcs
AstraC objdump program.BIN imports
```
