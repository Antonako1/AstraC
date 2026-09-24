# AstraC - A Simple Assembler and Compiler for x86 Architecture

<div style="display: flex; justify-content: center;">
  <img src="./DOCS/AC.png" alt="AstraC Source Logo" width="100" style="border-right: 1px solid #555; padding-right: 20px; margin-right: 20px;">
  <img src="./DOCS/AH.png" alt="AstraC Header Logo" width="100" style="border-right: 1px solid #555; padding-right: 20px; margin-right: 20px;">
  <img src="./DOCS/AS.png" alt="AstraC Assembly Logo" width="125">
</div>

AstraC is a lightweight assembler and compiler for the Intel I386 and Intel I286 architecture.
  Designed and built for use on emulators and raw machines that run raw binary.

- See the [documentation](./DOCS/README.md) for more information on how to use AstraC.
- See the [VS Code extension](https://marketplace.visualstudio.com/items?itemName=Antonako1.ac-language-support) for syntax highlighting and integration with Visual Studio Code.

## Features

- Lightweight assembler and compiler for x86 architecture (i386 / i286)
- Assembler features automatic multi-pass jump relaxation and `SHORT`/`NEAR`/`FAR` distance specifiers
- Compiler supports C-like syntax with global initializers, array brace-initialization, variadic functions (`va_start`/`va_arg`/`va_end`), top-level inline assembly, and cdecl ABI
- Shared C-style preprocessor supporting object-like and function-like macros, conditional compilation (`#if`, `#ifdef`, `#ifndef`), and `#push`/`#pop` pragmas
- Outputs raw binary, ACFH executables/libraries (`exe`/`lib`), or assembly files
- Cross-platform for Windows and Linux with colorized diagnostics and source line previews (`showline`)
- Own standard library for linkage to other operating systems
- Whole executable source compiled as one file (unity build)

## Install

### Windows

Build the NSIS installer (requires [NSIS](https://nsis.sourceforge.io/)):

```bat
SCRIPTS\WIN\CREATE_NSIS.BAT
```

Output: `build\AstraC-Setup-<version>.exe`

### Linux (curl)

One-liner (downloads a release tarball, or builds from source if none matches your arch):

```bash
curl -fsSL https://raw.githubusercontent.com/Antonako1/AstraC/main/SCRIPTS/SH/get-astrac.sh | sh
```

User install (no root, `~/.local`):

```bash
curl -fsSL https://raw.githubusercontent.com/Antonako1/AstraC/main/SCRIPTS/SH/get-astrac.sh | sh -s -- --user
```

### Linux (tarball)

From a GitHub Release asset `AstraC-<version>-linux-<arch>.tar.gz`:

```bash
tar -xzf AstraC-*-linux-*.tar.gz
cd AstraC-*-linux-*
sudo ./install.sh          # system → /usr/local
# or:  ./install.sh --user # → ~/.local
```

Build a release tarball yourself (same contents as the NSIS package: binary, icons, LICENSE, examples, docs):

```bash
./SCRIPTS/SH/CREATE_PACKAGE.sh           # build + package
./SCRIPTS/SH/CREATE_PACKAGE.sh -nobuild  # package existing binary
./SCRIPTS/SH/CREATE_PACKAGE.sh -with-src # also include source tree
```

Output: `build/AstraC-<version>-linux-<arch>.tar.gz`

Uninstall:

```bash
sudo /usr/local/share/astrac/uninstall.sh
# or: ~/.local/share/astrac/uninstall.sh
```

## Usage

```
ASTRAC.EXE [options] [flags]

Options:
  asm <file.AS>                    ; Assemble input file
  comp <file.AC>                   ; Compile input file
  disasm <file.BIN>                ; Disassemble input file
  objdump <file.BIN>               ; Dump ACFH binary header and tables
  strdump <file.BIN>               ; Dump strings from ACFH binary rodata section
  preproc <file.AC|file.AS>        ; Preprocess file
  info <mnemonic>                  ; Show information about a mnemonic
  showline <AS|AC> <ctx> <start> [end] ; Show source lines around a line number
  version                          ; Show version information
  help                             ; Show this help message

Flags:
  macro <name> <value>             ; Define a macro for preprocessing
  stepoff <level>                  ; Levels: 1=After preprocessing, 2=After assembling 3=After compiling
  verbose                          ; Verbose output
  debug                            ; Debug output to files. (AC->AS, AS->ASD)
  arch <architecture>              ; Specify target architecture: i386 or i286. Default=i386
  exe                              ; Specify to output a binary file with a simple header. Off by default.
  lib                              ; Specify to output a binary file with a simple header. Off by default.
  bits <16|32>                     ; Force 16-bit or 32-bit instruction encoding
  org <address>                    ; Specify memory origin address for raw binaries (e.g., 0x7C00)
  entry <label>                    ; Define the entry point for executables
  warn <level>                     ; Warning level (0=none, 1=standard, 2=all, err=treat as errors)
  debug                           ; Emit source-line comments in generated .AS for debugging
```

### Examples

Compile an AstraC source file to a raw 32-bit binary:

```Bash
ASTRAC.EXE comp kernel.AC arch i386
```

Assemble an assembly file to a 16-bit executable with verbose logging:

```Bash
ASTRAC.EXE asm bootloader.AS arch i286 exe verbose
```

Preprocess a file and stop (useful for debugging macros):

```Bash
ASTRAC.EXE comp main.AC stepoff 1
```

## Testing

AstraC includes an automated compiler test suite under `TESTS/COMPILER/` covering all compiler language features. Run the test suite:

**Windows (PowerShell):**
```powershell
powershell -ExecutionPolicy Bypass -File TESTS/RUN_COMPILER_TESTS.ps1
```

**Linux/macOS:**
```bash
./TESTS/RUN_COMPILER_TESTS.sh
```

## License

Distributed under the MIT License. See `LICENSE` for more information.