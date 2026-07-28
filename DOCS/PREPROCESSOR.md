# AstraC preprocessor documentation

AstraC preprocessor is a simple C-like preprocessor that supports macros, conditional compilation, and file inclusion. It is designed to be used with the AstraC assembler and compiler.

## Good to know

After preprocessing, the output is a single file that can be processed by the assembler or compiler. This file can be found on Windows inside the `C:\TMP\` folder, and on Linux inside the `/TMP/` folder. The output file is named `00.AC` or `00.AS`. For example, if the input file is `main.ac`, the output file will be `00.AC`. If the input file is `main.as`, the output file will be `00.AS`.

The lexers and parsers of the assembler and compiler do not support multiple input files, so the preprocessor is necessary to combine all included files into a single output file. The row and column numbers in the original source files are not preserved in this `00.AC` or `00.AS` file, so if there are errors in the output file, the line numbers will not match the original source files!

## Macros

AstraC preprocessor supports object-like macros. Object-like macros are simple text substitutions. They can be defined using the `#define` directive.

```c
#define MAX_VALUE 100
```

## Conditional Compilation

AstraC preprocessor supports conditional compilation using `#ifdef`, `#elif`, `#else`, and `#endif` directives. This allows you to include or exclude parts of the code based on certain conditions.

```c
#ifdef DEBUG
    // Debug code here
#elif RELEASE
    // Release code here
#else
    // Code for other configurations
#endif
```

## File Inclusion

AstraC preprocessor supports file inclusion using the `#include` directive. You can include other files in your source code, which allows for modular code organization.

File inclusion is necessary for using the AstraC assembler and compiler if user wants the code to be inside multiple files, since the assembler and compiler do NOT support multiple input files. The preprocessor will combine all included files into a single output file that can be processed by the assembler or compiler.

```c
// Works with ac
#include "header.ah"
// And as
#include "header.as"
```