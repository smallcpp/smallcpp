# smallcpp

Initial C++ CMake scaffold. The `scpp` target parses one `.scpp` file and lowers scalar functions, calls, aggregates, namespaces, classes, allocation, exceptions, `print`, and `format` to a verified LLVM module. A path ending in `.bc` is bitcode. Any other path is an executable linked with the Clang runtime, the collector, and `print` and `format`. It does not print IR as text.

## Build

```sh
cmake -S . -B build
cmake --build build
cmake --build build --target test
./bin/scpp tests/examples/return-zero.scpp answer.bc
./bin/scpp tests/examples/return-zero.scpp answer
./bin/scpp tests/examples/if.scpp if.bc
./bin/parse_test tests/examples/accept.scpp
```

Binaries are written to `bin/` and libraries to `lib/`. A normal build writes `scpp` and the libraries. `command_line_test`, `parse_test`, and `verify_module` are built by the `test` target, and CTest runs after that.

## Layout

- `src/shell` — parser driver that writes a verified LLVM bitcode module
- `src/runtime` — precise collector. `smallcpp_alloc` traces stack maps at safepoints. `delete` does not free
- `src/compiler` — Bison grammar and Flex scanner
- `src/preprocessor` — source directory for a later target
- `docs` — documentation (`docs/smallcpp.y` and `docs/smallcpp.l` are copies of the grammar and scanner)
- `scripts` — helper scripts
- `tests` — `parse_test` driver
- `tests/examples` — parser inputs
