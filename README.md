# smallcpp

Initial C++ CMake scaffold. `scpp` runs `preproc`, then `scpp_cc`, and then Clang when the result is an executable. `scpp_cc` parses one `.scpp` file and lowers it to a verified LLVM module. Several `.scpp` files on one `scpp` command are compiled separately and linked into one executable. `--emit-llvm`, or a path ending in `.bc`, writes that IR. `-c` writes an object and does not link. Those two forms still take one source. Any other path is an executable linked with the Clang runtime, the collector, and `print` and `format`. `scpp` does not print IR as text.

## Build

```sh
cmake -S . -B build
cmake --build build
cmake --build build --target test
./bin/scpp --emit-llvm tests/examples/return-zero.scpp answer.bc
./bin/scpp tests/examples/return-zero.scpp answer
./bin/scpp -c -o answer.o tests/examples/return-zero.scpp
./bin/scpp -o ledger tests/examples/ledger/money.scpp tests/examples/ledger/account.scpp tests/examples/ledger/book.scpp tests/examples/ledger/report.scpp tests/examples/ledger/main.scpp
./bin/parse_test tests/examples/accept.scpp
```

Binaries are written to `bin/` and libraries to `lib/`. A normal build writes `scpp`, `scpp_cc`, `preproc`, and the libraries. `command_line_test`, `parse_test`, and `verify_module` are built by the `test` target, and CTest runs after that.

## Layout

- `src/scpp` — the `scpp` driver. It coordinates `preproc`, `scpp_cc`, and the Clang link
- `src/runtime` — precise collector. `smallcpp_alloc` traces stack maps at safepoints. `delete` does not free
- `src/compiler` — Bison grammar and Flex scanner
- `src/preprocessor` — source directory for a later target
- `docs` — documentation (`docs/smallcpp.y` and `docs/smallcpp.l` are copies of the grammar and scanner)
- `scripts` — helper scripts
- `tests` — `parse_test` driver
- `tests/examples` — parser inputs
