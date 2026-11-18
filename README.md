# smallcpp

Initial C++ CMake scaffold. `scpp` runs `preproc`, then `scpp_cc`, and then Clang when the result is an executable. `scpp_cc` parses one `.scpp` file and lowers it to a verified LLVM module. Several `.scpp` files on one `scpp` command are compiled separately and linked into one executable. One source and no `-o` writes `a.out`. `-k` keeps every generated file. Without it, `scpp` deletes intermediate files after a successful build. `preproc` deletes nothing. `--emit-llvm` writes LLVM IR for every input and stops. `-E` writes preprocessor output for every input and stops. With more than one input, the output base name is the input base name. `-c` writes one object and does not link. `-l` names a library and may be repeated. `-I` and `-L` accept a `:` separated list and search it in order. A path ending in `.bc` is LLVM IR. An executable defines `int main`. The runtime does not supply `main`. An executable with no `main` is an error. `-c`, `--emit-llvm`, and `-E` do not require `main`. `scpp` does not print IR as text.

## Build

```sh
cmake -S . -B build
cmake --build build
cmake --build build --target test
./bin/scpp tests/examples/return-zero.scpp
./bin/scpp --emit-llvm tests/examples/return-zero.scpp program.bc
./bin/scpp --emit-llvm tests/examples/return-zero.scpp tests/examples/extern.scpp
./bin/scpp -E tests/examples/return-zero.scpp
./bin/scpp tests/examples/return-zero.scpp program
./bin/scpp -c -o program.o tests/examples/return-zero.scpp
./bin/scpp -o ledger tests/examples/ledger/money.scpp tests/examples/ledger/account.scpp tests/examples/ledger/book.scpp tests/examples/ledger/report.scpp tests/examples/ledger/main.scpp
./bin/parse_test tests/examples/accept.scpp
```

Binaries are written to `bin/` and libraries to `lib/`. A normal build writes `scpp`, `scpp_cc`, `preproc`, and the libraries. `command_line_test`, `parse_test`, and `verify_module` are built by the `test` target, and CTest runs after that.

## Layout

- `src/scpp` — the `scpp` driver. It coordinates `preproc`, `scpp_cc`, and the Clang link
- `include` — language headers. `<exceptions>` defines the base `Exception` class
- `src/runtime` — precise collector, `print` and `format`, and `Exception`. `smallcpp_alloc` traces stack maps at safepoints. `delete` does not free. An exception nothing catches aborts and prints a stack
- `src/compiler` — Bison grammar and Flex scanner
- `src/preprocessor` — source directory for a later target
- `docs` — documentation (`docs/smallcpp.y` and `docs/smallcpp.l` are copies of the grammar and scanner)
- `scripts` — helper scripts
- `tests` — `parse_test` driver
- `tests/examples` — parser inputs
