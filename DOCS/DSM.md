# AstraC Disassembler

The AstraC disassembler converts raw x86 binary files into human-readable
assembly listings. It performs linear-sweep disassembly in both 16-bit and
32-bit x86 modes.

## Quick start

```sh
AstraC.exe disasm file.BIN
AstraC.exe disasm file.BIN bits 16 org 7C00 verbose
```

## CLI reference

```
AstraC.exe disasm <file.BIN> [flags]

Flags:
    bits <16|32>    ; Disassembly mode (default: 32)
    org <address>   ; Origin offset for displayed addresses (hex, no 0x prefix)
    verbose         ; Print progress information
```

- `bits 16` interprets the binary as 16-bit code (real mode, 16-bit addressing).
- `bits 32` interprets the binary as 32-bit code (protected mode, 32-bit addressing).
- `org` adds a constant offset to all displayed addresses. Use `org 7C00` for
  bootloader binaries loaded at the classic 0x7C00 segment. The value must be
  a hexadecimal number **without** the `0x` prefix.

Output is written to `<input>`.DSM by default (e.g. `MAIN.BIN` → `MAIN.DSM`).

## How it works

The disassembler uses **linear-sweep disassembly** — it walks the binary from
start to end, decoding each instruction sequentially without following control
flow. No symbol table or relocation information is consumed; all addresses and
immediates are rendered as hex constants.

### Pipeline

1. **Read** — The entire binary is loaded into memory.
2. **Prefix scan** — Legacy prefixes (`66h`, `F2h`, `F3h`) and the address-size
   override (`67h`) are consumed before each instruction.
3. **Opcode lookup** — The mnemonic table (shared with the assembler) is searched
   for a match on prefix, opcode byte(s), and ModRM `/digit` extension.
4. **Operand decode** — ModRM, SIB, displacement, and immediate bytes are
   consumed according to the matched table entry. Register names and addressing
   modes adapt to the selected `bits` mode.
5. **Format & emit** — One line per instruction in a fixed-layout hex dump.

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
| Disassembly | 32 chars (left-aligned) | Decoded mnemonic and operands |
| ASCII | 16 chars | Printable representation (`.` for non-printable) |

Unrecognised opcode bytes are emitted as `db 0xNN`.

### Example

```
00007C00  55                                               push ebp                         |U|
00007C01  89 E5                                            mov ebp, esp                     |..|
00007C03  B8 01 00 00 00                                   mov eax, 0x00000001              |.....|
00007C08  74 05                                            jz 0x00007C0F                    |t.|
00007C0A  55 AA                                            db 0x55                          |U.|
```

## Known limitations

- **Linear sweep** — The disassembler does not follow control flow, so data
  embedded in code sections is decoded as instructions.
- **No symbol resolution** — Labels and variable names are not recovered.
- **No disassembly of data sections** — The binary is treated as a flat code
  stream. The separate `.data` and `.rodata` sections from the assembler
  appear at the start of the binary and will be decoded as instructions.

## Return codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Bad or missing arguments |
| 8 | Disassembly failed (I/O error) |
