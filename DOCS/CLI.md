# AstraC Command Line Interface (CLI) Reference

Complete reference for all commands, modes, flags, options, and sub-arguments supported by AstraC (`AstraC.exe` / `AstraC`).

---

## Synopsis

```sh
AstraC [options] [flags]
```

Output binary files default to input filename with `.BIN` extension (`.DSM` for disassembly, `.AS` for compiled assembly stepoff).

---

## Primary Commands

### 1. `asm <file.AS>`
Assembles an x86 assembly source file (`.AS`) into a binary output file (`.BIN`).

```sh
AstraC asm boot.AS
AstraC asm kernel.AS exe entry _main
```

### 2. `comp <file.AC>`
Compiles an AC C-like source file (`.AC`) to x86 assembly (`.AS`) and assembles it into a binary output file (`.BIN`).

```sh
AstraC comp main.AC
AstraC comp app.AC type exe offset_table,function_table,import_table
```

### 3. `disasm <file.BIN>`
Disassembles an x86 binary file (`.BIN`) into a human-readable assembly listing (`.DSM`).

```sh
AstraC disasm kernel.BIN bits 32 org 0x100000
```

### 4. `objdump <file.BIN> [sub-arg]`
Inspects and dumps ACFH binary header fields, relocation tables, function export tables, and import tables.

Sub-arguments:
- `all`: Dumps header info and all present tables (relocations, function export table, import table).
- `header` / `hdr`: Dumps only the 108-byte `AC_FILE_HEADER` metadata.
- `tables` / `tbl`: Dumps overview of present tables.
- `funcs` / `ft` / `function_table`: Dumps exported function table symbols and addresses.
- `relocs` / `rel` / `offset_table`: Dumps relocation offset table entries.
- `imports` / `it` / `import_table`: Dumps imported library names, target symbol names, and patch offsets.

```sh
AstraC objdump app.BIN all
AstraC objdump app.BIN header
AstraC objdump app.BIN funcs
AstraC objdump app.BIN imports
```

### 5. `strdump <file.BIN>`
Dumps all null-terminated ASCII string literals stored in the `.rodata` section of an ACFH binary.

```sh
AstraC strdump app.BIN
```

### 6. `preproc <file.AC|file.AS>`
Runs the preprocessor pass on a source file and outputs the preprocessed temporary file (`00.AC` or `00.AS`).

```sh
AstraC preproc main.AC macro DEBUG 1
```

### 7. `info <mnemonic>`
Displays instruction set metadata for a given x86 mnemonic (opcodes, operand forms, CPU requirements, flags affected).

```sh
AstraC info mov
AstraC info pusha
```

### 8. `showline <AS|AC> <ctx> <start> [end]`
Displays source lines from an assembly (`AS`) or AC compiler (`AC`) file centered around line numbers with line previews and context padding.

```sh
AstraC showline AC 3 45 55
```

### 9. `version`
Displays version string, version header information, build configuration, and copyright details.

```sh
AstraC version
```

### 10. `help` / `-h` / `--help`
Displays CLI command summary, options, and flag documentation.

```sh
AstraC help
AstraC -h
AstraC --help
```

---

## Flags and Options

| Flag / Option | Arguments | Description |
|---|---|---|
| `macro` | `<name> <val>` | Defines a preprocessor macro (equivalent to `#define name val`). |
| `stepoff` | `<1\|2\|3>` | Stops pipeline execution early: `1` = after preprocessing, `2` = after compile (emits `.AS`), `3` = after assemble. |
| `verbose` | *None* | Enables detailed pipeline diagnostics, pass progress output, and lists all entries included in Relocations, Function Export, and Import Tables. |
| `debug` | *None* | Emits source-line comments in generated `.AS` file and `.ASD` debug listing. |
| `arch` | `<i386\|i286>` | Specifies target architecture (`i386` default 32-bit, `i286` 16-bit). |
| `type` | `{exe\|lib} [table,...]` | Emits ACFH header with optional tables (`offset_table` / `ot`, `function_table` / `ft`, `import_table` / `it`). Multiple comma-separated or space-separated tables can be specified. |
| `exe` | *None* | Shortcut for ACFH executable format with header. |
| `lib` | *None* | Shortcut for ACFH library format with header. |
| `bits` | `<16\|32>` | Forces 16-bit (`.use16`) or 32-bit (`.use32`) instruction encoding mode. |
| `org` | `<addr>` | Sets memory origin address for raw binary output (e.g. `0x7C00` or `0x100000`). |
| `entry` | `<label>` | Sets executable entry point label (default `_start` / `main`). |
| `warn` | `<level>` | Sets warning level (`0` = suppress, `1` = standard, `2` = all, `err` = treat as errors). |

---

## Output Header & Table Selection Syntax

You can combine header flags and dynamic table options:

```sh
# Executable with relocation offset table and function export table
AstraC comp app.ac type exe offset_table,function_table

# Executable with import table for dynamic library linking
AstraC comp app.ac type exe import_table

# Library output with function export table
AstraC comp libgraphics.ac type lib ft
```

---

## Return Codes

| Code | Constant | Meaning |
|---|---|---|
| `0` | `ASTRAC_OK` | Execution completed successfully |
| `1` | `ASTRAC_ERR_ARGS` | Invalid CLI arguments or missing input file |
| `2` | `ASTRAC_ERR_PREPROCESS` | Preprocessor syntax or file include error |
| `3` | `ASTRAC_ERR_LEX` | Tokenization / lexical analysis failure |
| `4` | `ASTRAC_ERR_AST` | AST parsing failure |
| `5` | `ASTRAC_ERR_VERIFY` | Semantic verification / type checking failure |
| `6` | `ASTRAC_ERR_OPTIMIZE` | Optimization pass error |
| `7` | `ASTRAC_ERR_CODEGEN` | Code generation / binary emitter failure |
| `8` | `ASTRAC_ERR_DISASSEMBLE` | Disassembler decoding error |
| `9` | `ASTRAC_ERR_COMPILE` | Top-level compiler execution failure |
