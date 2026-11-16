# tests

`parse_test` runs the scanner and parser on one `.scpp` file and, when the parse succeeds, checks that the syntax tree was built. The build parses `examples/accept.scpp`. CTest runs that accept example, which must pass.

`compile_return` runs the shell on `examples/return-zero.scpp`, then checks that the written bitcode module verifies and defines `int answer`. `compile_scalars` runs the shell on `examples/scalars.scpp` and checks that `int scale(int)` has a conditional branch and a loop back-edge.
