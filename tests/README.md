# tests

`parse_test` runs the scanner and parser on one `.scpp` file. The build parses `examples/accept.scpp`. CTest runs that accept example and each reject example: the accept example must pass, and each reject example must fail. Add new parser checks in `tests/CMakeLists.txt`.
