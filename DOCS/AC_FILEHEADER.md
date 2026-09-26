# AC Fileheader

AC fileheader is a simple header format, outputted by the AstraC compiler when the `exe` or `lib` flags are specified. It is designed to be a minimalistic header that provides essential information about the binary, such as its architecture, entry point, and size.

## Header Structure

The AC fileheader is defined in `AC_FH.h`, located in the project's root directory. The structure is as follows:

```c
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
    U32 func_table_offset;
    U32 code_size;
    U32 data_size;
    U32 rodata_size;
    U32 bss_size;
    U32 reloc_size;
    U32 func_table_size;
    U8  reserved[AC_FILE_RESERVED_SIZE]; /* 48 bytes */
} ATTRIB_PACKED AC_FILE_HEADER;
```

Fields:
- `magic`: A 4-byte magic number that identifies the file as an AC binary ("ACFH").
- `version`: A 32-bit unsigned integer representing the version of the AC fileheader format (1.0).
- `flags`: Flags describing binary properties (`AC_FLAG_EXECUTABLE = 1 << 0`, `AC_FLAG_DYNAMIC = 1 << 1`, `AC_FLAG_HAS_RELOCS = 1 << 2`, `AC_FLAG_HAS_FUNCS = 1 << 3`).
- `entry_point_offset`: Offset of the entry point from the start of the binary.
- `code_offset`: Offset of the code section.
- `data_offset`: Offset of the data section.
- `rodata_offset`: Offset of the read-only data section.
- `bss_offset`: Offset of the BSS section.
- `reloc_offset`: Offset of the relocation table (offset_table / ot).
- `func_table_offset`: Offset of the function export table (function_table / ft).
- `code_size`: Size of the code section in bytes.
- `data_size`: Size of the data section in bytes.
- `rodata_size`: Size of the read-only data section in bytes.
- `bss_size`: Size of the BSS section in bytes.
- `reloc_size`: Size of the relocation table in bytes.
- `func_table_size`: Size of the function export table in bytes (header + entries + string table).
- `reserved`: Reserved field (48 bytes) keeping total header size strictly at 108 bytes.

## Function Export Table (`function_table` / `ft`)

When `function_table` or `ft` is enabled (e.g. `type lib function_table` or `type exe ot ft`), AstraC emits a Function Export Table following the relocation table.

```c
typedef struct {
    U32 entry_count;        /* Number of exported functions */
    U32 string_table_size;  /* Total size of null-terminated string table pool in bytes */
} ATTRIB_PACKED AC_FUNC_TABLE_HDR;

typedef struct {
    U32 address;      /* File offset of function code (relative to binary start) */
    U32 name_offset;  /* Offset of function name string within string table pool */
    U16 param_size;   /* Total parameter stack size in bytes */
    U16 flags;        /* Function flags (1=cdecl, 2=stdcall, 4=variadic) */
} ATTRIB_PACKED AC_FUNC_ENTRY;
```

Function names are stored in a dynamic, contiguous string pool immediately following the array of `AC_FUNC_ENTRY` structures.

## CLI Invocation

Binary header output options are controlled via:
```
type {exe|lib} [offset_table(ot)|function_table(ft)]
```
Multiple tables can be specified together (e.g., `type exe offset_table function_table` or `type lib ot ft`).

## Inspection (`objdump`)

To inspect the header, sections, relocations, or function export table of an ACFH binary:
```
AstraC objdump <file.BIN> [all|header|tables|funcs|relocs]
```
Sub-arguments:
- `all` (default): Display header, section table, relocations, and function export table.
- `header`: Display only file header summary.
- `tables`: Display section offset and size table.
- `funcs`: Display function export table (addresses, names, parameter sizes, flags).
- `relocs`: Display relocation table offsets.

