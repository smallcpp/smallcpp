# compiler

`scpp_cc` is the C frontend. Bison (`smallcpp.y`), Flex (`smallcpp.l`), and the syntax tree (`ast.h`, `ast.c`) build the in-memory tree. CMake builds that parser as the static library `smallcpp_parse`. `scpp_cc` links the C++ IR emitter. The two sides meet at `src/emit/emit.h`.

`-o FILE` passes the tree to the emitter. `--emit-llvm`, or a path ending in `.bc`, writes LLVM IR and does not link. `-c` writes an object and does not link. Any other path is an executable, and `-g` is passed to Clang on that link. `--ast-text FILE` writes a structured text form of the tree. `--ast-dot FILE` writes a Graphviz DOT file. Any combination of the dump files and one code output is accepted, including any one of them alone.

A typedef name is recorded so a later use is a type name.
