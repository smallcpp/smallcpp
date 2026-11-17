# compiler

Bison grammar (`smallcpp.y`), Flex scanner (`smallcpp.l`), and the syntax tree (`ast.h`, `ast.c`). CMake builds them as the static library `smallcpp_parse`. The parser library builds the tree. The shell lowers scalar functions, calls, aggregates, namespaces, classes, allocation, and exceptions from that tree to LLVM IR. A typedef name is recorded so a later use is a type name.
