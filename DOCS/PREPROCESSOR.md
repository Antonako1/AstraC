# AstraC preprocessor documentation

AstraC preprocessor is a simple C-like preprocessor that supports macros, conditional compilation, and file inclusion. It is designed to be used with the AstraC assembler and compiler.

## Good to know

After preprocessing, the output is a single file that can be processed by the assembler or compiler. This file can be found on Windows inside the `C:\TMP\` folder, and on Linux inside the `/tmp/` folder. The output file is named `00.AC` or `00.AS`. For example, if the input file is `main.ac`, the output file will be `00.AC`. If the input file is `main.as`, the output file will be `00.AS`.

The lexers and parsers of the assembler and compiler do not support multiple input files, so the preprocessor is necessary to combine all included files into a single output file. The row and column numbers in the original source files are not preserved in this `00.AC` or `00.AS` file, so if there are errors in the output file, the line numbers will not match the original source files!

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
| `#include "file"` | Includes external header or source file |
| `#define NAME value` | Defines an object-like macro |
| `#define NAME(a,b) expr` | Defines a function-like macro |
| `#undef NAME` | Undefines an existing macro |
| `#ifdef NAME` | Evaluates true if macro `NAME` is defined |
| `#ifndef NAME` | Evaluates true if macro `NAME` is NOT defined |
| `#if expr` | Evaluates integer constant expression `expr` |
| `#elif expr` | Else-if branch evaluating `expr` |
| `#else` | Else branch |
| `#endif` | Terminates conditional block |
| `#error "msg"` | Halts preprocessing and outputs error message |
| `#warning "msg"` | Outputs preprocessor warning message |
| `#push <instruction>` | Pushes preprocessor/parser directive setting |
| `#pop <instruction>` | Pops preprocessor/parser directive setting |

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

