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

- Lightweight assembler and compiler for x86 architecture
- Outputs raw binary, or asm files.
- Cross-platform for Windows and Linux
- Own standard library for linkage to other operating systems
- Whole executable source compiled as one file.

## Building

## Usage

```
ASTRAC.EXE [options] [flags]

Options:
    asm <file.AS>                   ; Assemble input file
    comp <file.AC>                  ; Compile input file
    disasm <file.BIN>               ; Disassemble input file
    preproc <file.AC|file.AS>       ; Preprocess file
    version                         ; Show version information
    help                            ; Show this help message
 
Flags:
    macro <name> <value>            ; Define a macro for preprocessing
    stepoff <level>                 ; Levels: 1=After preprocessing, 2=After assembling 3=After compiling
    verbose                         ; Verbose output

    arch <architecture>             ; Specify target architecture: i386 or i286. Default=i386
    exe                             ; Specify to output a binary file with a simple header. Off by default.
    lib                             ; Specify to output a binary file with a simple header. Off by default.
    bits <16|32>                    ; Force 16-bit or 32-bit instruction encoding
    org <address>                   ; Specify memory origin address for raw binaries (e.g., 0x7C00)
    entry <label>                   ; Define the entry point for executables
    warn <level>                    ; Warning level (0=none, 1=standard, 2=all, err=treat as errors)
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

## License

Distributed under the MIT License. See `LICENSE` for more information.