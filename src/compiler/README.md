# compiler

Bison grammar (`smallcpp.y`), Flex scanner (`smallcpp.l`), and the syntax tree (`ast.h`, `ast.c`). CMake builds them as the static library `smallcpp_parse`. The parser library builds the tree. The shell lowers scalar functions from that tree to LLVM IR.
