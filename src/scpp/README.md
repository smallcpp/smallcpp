# scpp

`scpp-lsp` is the language server beside this driver. See `docs/LSP.md`.

`scpp` coordinates compilation. It runs `preproc`, then `scpp_cc`, and, for an executable, Clang. Several source files are compiled one at a time and linked together. It does not parse, and it does not emit LLVM IR. Each option is passed only to the phase that uses it. `-o` is optional, and the default executable is `a.out`.

`scpp` reads `~/.config/smallcpp/config.ini` when that file exists. `include` is the header directory. `library` is the default `-L`, after an explicit `-L`. `runtime` is `libsmallcpp_runtime.a`. `libm` and the other system-library keys are absolute paths on the link, and `libm` is `libm.a`. An explicit `-l` for one of those libraries is linked instead of the recorded path. When the file is missing, `scpp` uses the prefix that contains its `bin/` directory, then the configured prefix. A fresh install uses `$HOME/opt/smallcpp` and writes the file. `preproc` and `scpp_cc` are `bin/preproc` and `bin/scpp_cc`.

```sh
./bin/scpp tests/examples/return-zero.scpp
./bin/scpp --emit-llvm tests/examples/return-zero.scpp program.bc
./bin/scpp --emit-llvm tests/examples/return-zero.scpp tests/examples/extern.scpp
./bin/scpp -E tests/examples/return-zero.scpp
./bin/scpp -k -o program tests/examples/return-zero.scpp
./bin/scpp tests/examples/return-zero.scpp program
./bin/scpp -c -o program.o tests/examples/return-zero.scpp
./bin/scpp -o ledger tests/examples/ledger/money.scpp tests/examples/ledger/account.scpp tests/examples/ledger/book.scpp tests/examples/ledger/report.scpp tests/examples/ledger/main.scpp
```

`-D` and `-I` go to `preproc`. `-I` and `-L` may be repeated, and one option may hold several directories separated by `:`. The list is searched in order. `-i` and `--ignore-environment` go to `preproc` and import no environment macros. `-o` names the file written by the last phase when it is present: the object file with `-c`, the IR or preprocessor file when there is one input, and the executable otherwise. A path ending in `.bc` is LLVM IR. `--emit-llvm` writes IR for every input and stops. `-E` writes preprocessor output for every input and stops. With more than one of those inputs, each file is named from the input base name. `-c` and a `.bc` path take one source. `-c`, `--emit-llvm`, `-E`, and a `.bc` path do not require `main`. An executable starts at the program's `int main`. The runtime does not supply `main`. If none of the objects define `main`, `scpp` reports `program has no main` and does not link. `-g` asks `scpp_cc` to record the source file, line numbers, functions, and types, and the Clang link keeps that information. `-L` and `-l` go to the Clang link. `-l` may be repeated. `preproc` deletes nothing. `-k` and `--keep` leave every generated file. Without them, `scpp` deletes the intermediate files after a successful build. Each source writes its base name with `.i` and `.o` before that deletion. `hello.scpp` writes `hello.i` and `hello.o`, for one file and for each file of several.
