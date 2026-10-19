# AstraC preprocessor documentation

AstraC preprocessor is a simple C-like preprocessor that supports macros, conditional compilation, and file inclusion. It is designed to be used with the AstraC assembler and compiler.

## Good to know

After preprocessing, the output is a per-target single file that is processed by the assembler or compiler:
- For AC compiler inputs (e.g. `main.AC`), the preprocessed file is `<base>.I` (`main.I`).
- For Assembler inputs (e.g. `boot.AS`), the preprocessed file is `<base>.ASI` (`boot.ASI`).

Using per-target temporary files allows parallel `make`/`cmake` builds without race conditions. The temporary `.I` / `.ASI` file is automatically deleted when compilation or assembly finishes, unless `stepoff 1`, `preproc`, or `debug` is active.

Furthermore, the preprocessor automatically emits `#line <line> "<file>"` markers at the start of every file, upon entering and returning from `#include` directives, and after skipped comment or conditional blocks. Both the AC compiler and the assembler track these `#line` markers so all error/warning diagnostics and `local` file-scoped symbols report the exact original source file and line number!

## Macros

AstraC preprocessor supports both **object-like** and **function-like** macros. Up to 1024 macros can be defined in total (`MAX_MACROS 1024`), including up to 64 defined via CLI `macro <name> <value>`.

### Object-like Macros

Object-like macros perform simple text substitution:

```c
#define MAX_VALUE 100
#define BUFFER_SIZE (MAX_VALUE * 4)
```

Use `#undef` to remove a macro definition:

```c
#undef MAX_VALUE
```

### Function-like Macros

Function-like macros accept parameter lists and substitute arguments into the macro body. Nested parentheses in arguments are respected, and top-level arguments are whitespace-trimmed:

```c
#define ADD(a, b) ((a) + (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

int x = ADD(5, 10);
```

## Directives Reference

| Directive | Description |
|-----------|-------------|
| `#include "file"` | Includes external header or source file (emits `#line` enter/return markers) |
| `#import funcs from "lib"` | Specifies external dynamic library function imports for ACFH binaries |
| `#define NAME value` | Defines an object-like macro |
| `#define NAME(a,b) expr` | Defines a function-like macro |
| `#undef NAME` | Undefines an existing macro |
| `#ifdef NAME` | Evaluates true if macro `NAME` is defined |
| `#ifndef NAME` | Evaluates true if macro `NAME` is NOT defined |
| `#if expr` | Evaluates integer constant expression `expr` |
| `#elif expr` | Else-if branch evaluating `expr` |
| `#else` | Else branch |
| `#endif` | Terminates conditional block |
| `#line <line> ["file"]` | Sets the current source line number (and optional filename) for diagnostics and file scoping |
| `#error "msg"` | Halts preprocessing and outputs error message |
| `#warning "msg"` | Outputs preprocessor warning message |
| `#push <instruction>` | Pushes preprocessor/parser directive setting |
| `#pop <instruction>` | Pops preprocessor/parser directive setting |

## Dynamic Library Imports (`#import`)

The `#import` directive specifies external dynamic library dependencies and target function symbols to be resolved at runtime by an OS binary loader.

```c
#import draw_rect, clear_screen from "graphics.lib"
#import malloc, free from "libc.lib"
```

- **Preprocessing**: In C preprocessor mode, `#import` lines are passed through to the preprocessed file (`<base>.I`).
- **Compiler**: Registers the function symbols with default `cdecl` signatures and emits `.import` directives to the generated `.AS` source.
- **Assembler & ACFH Output**: The assembler records the library names, function names, and patch offsets in the ACFH binary's Import Table (`AC_IMPORT_TABLE_HDR` and `AC_IMPORT_ENTRY`).

## Constant Expression Evaluation

The `#if` and `#elif` directives support full recursive-descent integer constant expression evaluation. Supported operators include:
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Bitwise: `~`, `&`, `|`, `^`, `<<`, `>>`
- Logical & Comparison: `!`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`

```c
#if (VERSION_MAJOR >= 2) && (FEATURE_MASK & 0x01)
    // Code for version 2+ with feature bit 0 enabled
#endif
```

## Push/Pop instructions for lexer, parser, codegen

Usage:
```c
#push <instruction>
...
#pop <instruction>
```

Below is a table containing all instructions, what they do and are they supported by the compiler, assembler or both:

| Instruction           | Action  | Compiler | Assembler |
| --------              | ------- | -------  | -------   |
| PARSER_TOPLEVEL_LOG   | Enables/Disables Parser's top level logging    | X        |           |

