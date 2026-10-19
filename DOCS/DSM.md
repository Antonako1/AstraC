# AstraC Disassembler

The AstraC disassembler converts raw x86 binary files and ACFH executables/libraries
into human-readable assembly listings. It performs linear-sweep disassembly in both
16-bit and 32-bit x86 modes, and automatically detects ACFH headers to disassemble
only the `.code` section while annotating function symbols and dynamic library imports.

## Quick start

```sh
AstraC.exe disasm file.BIN
AstraC.exe disasm file.BIN bits 16 org 0x7C00 verbose
```

## CLI reference

```
ASTRAC.EXE [options] [flags]

Options:
  asm <file.AS>                    ; Assemble input file
  comp <file.AC>                   ; Compile input file
  disasm <file.BIN>                ; Disassemble input file
  objdump <file.BIN> [sub-arg]     ; Dump ACFH binary header and tables (all, header, tables, funcs, relocs, imports)
  strdump <file.BIN>               ; Dump strings from ACFH binary rodata section
  preproc <file.AC|file.AS>        ; Preprocess file to <base>.I / <base>.ASI
  info <mnemonic>                  ; Show information about a mnemonic
  showline <file|AS|AC> <ctx> <start> [end] ; Show source lines around a line number
  version                          ; Show version information
  help                             ; Show this help message

Flags:
  macro <name> <value>             ; Define a macro for preprocessing
  stepoff <level>                  ; Levels: 1=After preprocessing, 2=After compiling (emit .AS), 3=After assembling
  verbose                          ; Verbose output
  debug                            ; Debug output to files & emit source comments in .AS
  arch <architecture>              ; Specify target architecture: i386 or i286. Default=i386
  type {exe|lib} [ot|ft|it]        ; Output binary format with header and tables (offset_table/ot, function_table/ft, import_table/it)
  exe                              ; Shortcut for executable binary header output
  lib                              ; Shortcut for library binary header output
  bits <16|32>                     ; Force 16-bit or 32-bit instruction encoding
  org <address>                    ; Specify memory origin address for raw binaries (e.g., 0x7C00)
  entry <label>                    ; Define the entry point for executables
  warn <level>                     ; Warning level (0=none, 1=standard, 2=all, err=treat as errors)
```

- `bits 16` interprets the binary as 16-bit code (real mode, 16-bit addressing).
- `bits 32` interprets the binary as 32-bit code (protected mode, 32-bit addressing).
- `org` adds a constant offset to all displayed addresses. Use `org 0x7C00` (or `org 7C00`) for
  bootloader binaries loaded at the classic 0x7C00 segment.

Output is written to `<input>`.DSM by default (e.g. `MAIN.BIN` → `MAIN.DSM`).

## How it works

The disassembler uses **linear-sweep disassembly** — it walks the code stream from
start to end, decoding each instruction sequentially.

When an **ACFH binary** is passed (`magic == "ACFH"` at byte 0):
- The disassembler automatically skips the 112-byte (`0x70`) header and disassembles only the `.code` section (`code_offset..code_offset+code_size`), avoiding decoding `.data`, `.rodata`, or trailing tables as instructions.
- If a **Function Export Table (`ft`)** is present, function headers (`0000002A <MAIN>:`) are emitted before each function's entry point, and relative `CALL`/`JMP` instructions targeting known functions are annotated (`call 0x00000012 <HELPER_PUBLIC_FN>`).
- If an **Import Table (`it`)** is present, `CALL` instructions whose displacement field matches an `AC_IMPORT_ENTRY.patch_offset` are annotated with `<lib::func>` (`call 0x00000000 <libc.lib::MALLOC>`).

### Pipeline

1. **Read & ACFH Check** — The binary is loaded into memory and checked for the `"ACFH"` header magic.
2. **Prefix scan** — Legacy prefixes (`66h`, `F2h`, `F3h`) and the address-size
   override (`67h`) are consumed before each instruction.
3. **Opcode lookup** — The mnemonic table (shared with the assembler) is searched
   for a match on prefix, opcode byte(s), and ModRM `/digit` extension.
4. **Operand decode** — ModRM, SIB, displacement, and immediate bytes are
   consumed according to the matched table entry. Register names and addressing
   modes adapt to the selected `bits` mode.
5. **Format & emit** — One line per instruction in a fixed-layout hex dump, with symbol and import annotations when ACFH tables are present.

### 16-bit vs 32-bit mode

The `bits` flag controls two independent concerns:

| Concern | `bits 32` (default) | `bits 16` |
|---------|---------------------|-----------|
| Default operand size | 32-bit (`eax`, `ecx`, ...) | 16-bit (`ax`, `cx`, ...) |
| Default address size | 32-bit (`[esi]`, `[ebp+8]`, ...) | 16-bit (`[si]`, `[bp+di]`, ...) |
| `66h` prefix meaning | Switch to 16-bit operand | Switch to 32-bit operand |
| `67h` prefix meaning | Switch to 16-bit address | Switch to 32-bit address |
| ModRM/SIB encoding | 32-bit ModRM + SIB | 16-bit ModRM (no SIB) |

### Prefix inversion logic

In the assembler's mnemonic table, 16-bit instruction entries carry `PF_66`
and 32-bit entries carry `PF_NONE`. In 16-bit disassembly mode the meaning of
the `66h` prefix byte is inverted:

- No `66h` in the binary → match `PF_66` entries (16-bit operands are default).
- `66h` present → match `PF_NONE` entries (prefix enabled 32-bit operands).

`F2h` and `F3h` (REP/REPNE) are mode-independent.

## Output format

Each line follows a fixed layout:

```
OOOOOOOO  HH HH HH HH HH HH HH HH  HH HH HH HH HH HH HH HH  MNEMONIC OPS     |ASCII........|
```

| Field | Width | Description |
|-------|-------|-------------|
| Offset | 8 hex digits | Address in the binary (`org` offset applied) |
| Hex dump | 16 bytes (48 chars) | Raw bytes of the instruction, padded with spaces |
| Disassembly | 32 chars (left-aligned) | Decoded mnemonic, operands, and `<symbol>` / `<lib::func>` annotations |
| ASCII | 16 chars | Printable representation (`.` for non-printable) |

Unrecognised opcode bytes are emitted as `db 0xNN`.

### Example (ACFH Binary with Function & Import Tables)

```
; ACFH Binary (.code offset=0x70, size=189 bytes, entry=0x5)
00000000  E9 00 00 00 00                                   jmp 0x00000005 <MAIN>            |.....|

00000005 <MAIN>:
00000005  55                                               push ebp                         |U|
00000006  89 E5                                            mov ebp, esp                     |..|
00000008  83 EC 08                                         sub esp, 0x08                    |...|
0000003F  B8 00 04 00 00                                   mov eax, 0x00000400              |.....|
00000044  50                                               push eax                         |P|
00000045  E8 B6 FF FF FF                                   call 0x00000000 <libc.lib::MALLOC> |.....|
```

## Known limitations

- **Linear sweep within `.code`** — Data embedded directly inside `.code` is decoded as instructions.
- **Raw binaries without ACFH header** — For flat raw binaries (compiled without `exe`/`lib`), no section boundaries or symbol tables exist, so the entire file is decoded as a flat instruction stream.

## Return codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Bad or missing arguments |
| 8 | Disassembly failed (I/O error) |
