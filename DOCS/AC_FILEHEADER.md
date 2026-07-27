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
    U32 code_size;
    U32 data_size;
    U32 rodata_size;
    U32 bss_size;
    U8  reserved[AC_FILE_RESERVED_SIZE];
} ATTRIB_PACKED AC_FILE_HEADER;
```

Fields:
- `magic`: A 4-byte magic number that identifies the file as an AC binary. The expected value is "ACFH".
- `version`: A 32-bit unsigned integer representing the version of the AC fileheader format.
- `flags`: A 32-bit unsigned integer representing various flags that describe the binary's properties (e.g., executable, dynamic).
- `entry_point_offset`: A 32-bit unsigned integer indicating the offset of the entry point from the start of the binary.
- `code_offset`: A 32-bit unsigned integer indicating the offset of the code section from the start of the binary.
- `data_offset`: A 32-bit unsigned integer indicating the offset of the data section from the start of the binary.
- `rodata_offset`: A 32-bit unsigned integer indicating the offset of the read-only data section from the start of the binary.
- `bss_offset`: A 32-bit unsigned integer indicating the offset of the BSS section from the start of the binary.
- `code_size`: A 32-bit unsigned integer indicating the size of the code section in bytes.
- `data_size`: A 32-bit unsigned integer indicating the size of the data section in bytes.
- `rodata_size`: A 32-bit unsigned integer indicating the size of the read-only data section in bytes.
- `bss_size`: A 32-bit unsigned integer indicating the size of the BSS section in bytes.
- `reserved`: A 8-bit reserved field for future use or alignment purposes.

