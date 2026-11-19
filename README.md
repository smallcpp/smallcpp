# smallcpp

Initial C++ CMake scaffold. `scpp` runs `preproc`, then `scpp_cc`, and then Clang when the result is an executable. `scpp_cc` parses one `.scpp` file and lowers it to a verified LLVM module. Several `.scpp` files on one `scpp` command are compiled separately and linked into one executable. One source and no `-o` writes `a.out`. `-k` keeps every generated file. Without it, `scpp` deletes intermediate files after a successful build. `preproc` deletes nothing. `--emit-llvm` writes LLVM IR for every input and stops. `-E` writes preprocessor output for every input and stops. With more than one input, the output base name is the input base name. `-c` writes one object and does not link. `-l` names a library and may be repeated. `-I` and `-L` accept a `:` separated list and search it in order. A path ending in `.bc` is LLVM IR. An executable defines `int main`. The runtime does not supply `main`. An executable with no `main` is an error. `-c`, `--emit-llvm`, and `-E` do not require `main`. `scpp` does not print IR as text.

## Build

`scripts/setup` configures `build/` and starts a project shell. `scripts/clean` removes `bin/`, `build/`, `lib/`, `doc/out/`, and `share/`. `scripts/showdocs` builds the CMake `docs` target and opens `doc/out/html/index.html` in Firefox. Run `showdocs` in that shell. This checkout defines no `docs` target, so that build fails.

Start at the checkout root:

```sh
scripts/setup
build
build test
exit
./bin/scpp tests/examples/return-zero.scpp
./bin/scpp --emit-llvm tests/examples/return-zero.scpp program.bc
./bin/scpp --emit-llvm tests/examples/return-zero.scpp tests/examples/extern.scpp
./bin/scpp -E tests/examples/return-zero.scpp
./bin/scpp tests/examples/return-zero.scpp program
./bin/scpp -c -o program.o tests/examples/return-zero.scpp
./bin/scpp -o ledger tests/examples/ledger/money.scpp tests/examples/ledger/account.scpp tests/examples/ledger/book.scpp tests/examples/ledger/report.scpp tests/examples/ledger/main.scpp
./bin/parse_test tests/examples/accept.scpp
```

`scripts/setup [--clean] [debug|release|profile]` works from any directory when you invoke this checkout's `scripts/setup`. A symlink kept outside `scripts/` resolves the wrong root. `debug`, `release`, and `profile` set `CMAKE_BUILD_TYPE` to `Debug`, `Release`, and `RelWithDebInfo`. With no type, a fresh directory keeps CMake's default and a later run keeps the cached type. `--clean` runs `scripts/clean` before configuration. It may precede or follow the type. Setup refuses to start when a project shell is already active. Exit that shell first. An unknown argument exits 2.

The shell reads `.project_env`, puts `bin/` and `scripts/` on `PATH`, and changes to `build/`. `build` and `rebuild` are functions in that shell. `build` compiles the default targets. `build test` builds those targets and `command_line_test`, `prefix_test`, `parse_test`, `verify_module`, and `lsp_test`, then runs the suite. `rebuild` is a clean-first build. A name after `build` or `rebuild` is a target. `exit` leaves the shell. The `./bin/scpp` lines above run from the checkout root. `parse_test` exists after `build test`.

Binaries are written to `bin/` and libraries to `lib/`. A normal `build` writes `scpp`, `scpp_cc`, `preproc`, `scpp-lsp`, and the libraries.

## Language server

`scpp-lsp` speaks the Language Server Protocol on standard input and standard output. A normal build writes `bin/scpp-lsp`. An editor starts that binary and keeps the process. The server takes no arguments. [docs/LSP.md](docs/LSP.md) lists the methods and how diagnostics are produced.

```sh
./bin/scpp-lsp
```

## Install

A fresh configuration installs to `$HOME/opt/smallcpp`. `scripts/setup` runs that configuration, and CMake expands `HOME` then. Programs go in `bin/`, headers in `include/`, and libraries in `lib/`. No script installs. In the project shell the current directory is `build/`.

```sh
scripts/setup
build
cmake --install .
exit
```

`scripts/setup` accepts `--clean` and a build type. A prefix is a CMake option. On CMake 3.29 and newer, a new build directory takes `CMAKE_INSTALL_PREFIX` from the environment, so `CMAKE_INSTALL_PREFIX=/absolute/path scripts/setup` selects it. A later configure keeps the cached prefix. `-DCMAKE_INSTALL_PREFIX=/absolute/path` still selects another prefix, including `/usr/local`. Pass that with CMake: `cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/absolute/path`. Inside the shell, `cmake -S "$_PROJECT_LOC" -B . -DCMAKE_INSTALL_PREFIX=/absolute/path` does the same. `cmake --install . --prefix /absolute/path` applies to that install. `DESTDIR=/tmp/stage cmake --install .` only stages the files. It is not part of the recorded paths.

The install step writes `$HOME/.config/smallcpp/config.ini` and creates that directory when it is missing. `HOME` has to be absolute. A missing or relative `HOME` stops the install and does not write the file. Header search and the link read this file. An explicit `-I` or `-L` is searched first. An explicit `-l` for one of the recorded libraries is linked, and that recorded path is left off the link. When the file is missing, `scpp` and `preproc` use the prefix that contains their `bin/` directory, then the configured prefix, and the compile does not fail.

```ini
include=/home/ada/opt/smallcpp/include
library=/home/ada/opt/smallcpp/lib
runtime=/home/ada/opt/smallcpp/lib/libsmallcpp_runtime.a
libdl=/lib/x86_64-linux-gnu/libdl.a
libstdc++=/usr/lib/gcc/x86_64-linux-gnu/14/libstdc++.so
libm=/lib/x86_64-linux-gnu/libm.a
libgcc_s=/usr/lib/gcc/x86_64-linux-gnu/14/libgcc_s.so
libgcc=/usr/lib/gcc/x86_64-linux-gnu/14/libgcc.a
libc=/lib/x86_64-linux-gnu/libc.so
```

`include` is the installed headers. `library` is the default `-L` directory. `runtime` is `libsmallcpp_runtime.a`. The other keys are the system libraries the link uses. `clang++ -print-file-name` finds each one on the machine that runs the install. `libm` is `libm.a`. `scpp` still finds `preproc` and `scpp_cc` under the prefix that contains its own `bin/` directory.

## Layout

- `src/scpp` — the `scpp` driver. It coordinates `preproc`, `scpp_cc`, and the Clang link
- `src/lsp` — the `scpp-lsp` language server
- `include` — language headers. `<exceptions>` defines the base `Exception` class. `<string>`, `<array>`, `<vector>`, `<list>`, `<tree>`, and `<hash>` are the collections
- `src/runtime` — precise collector, `print` and `format`, and `Exception`. `smallcpp_alloc` traces stack maps at safepoints. `delete` does not free. An exception nothing catches aborts and prints a stack
- `src/compiler` — Bison grammar and Flex scanner
- `src/preprocessor` — source directory for a later target
- `docs` — documentation (`docs/smallcpp.y` and `docs/smallcpp.l` are copies of the grammar and scanner)
- `scripts` — `setup`, `clean`, and `showdocs`
- `tests` — `parse_test` driver
- `tests/examples` — parser inputs
