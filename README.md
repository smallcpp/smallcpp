# smallcpp

Initial C++ CMake scaffold. The `shell` target parses one `.scpp` file and writes a verified LLVM bitcode module.

## Build

```sh
cmake -S . -B build
cmake --build build
(cd build && ctest --output-on-failure)
./bin/shell tests/examples/return-zero.scpp answer.bc
./bin/parse_test tests/examples/accept.scpp
```

Binaries are written to `bin/` and libraries to `lib/`.

## Layout

- `src/shell` — parser driver that writes a verified LLVM bitcode module
- `src/compiler` — Bison grammar and Flex scanner
- `src/preprocessor` — source directory for a later target
- `docs` — documentation (`docs/smallcpp.y` and `docs/smallcpp.l` are copies of the grammar and scanner)
- `scripts` — helper scripts
- `tests` — `parse_test` driver
- `tests/examples` — parser inputs
