# scpp

`scpp` coordinates compilation. It runs `preproc`, then `scpp_cc`, and, for an executable, Clang. Several source files are compiled one at a time and linked together. It does not parse, and it does not emit LLVM IR. Each option is passed only to the phase that uses it.

```sh
./bin/scpp --emit-llvm tests/examples/return-zero.scpp answer.bc
./bin/scpp tests/examples/return-zero.scpp answer
./bin/scpp -c -o answer.o tests/examples/return-zero.scpp
./bin/scpp -o ledger tests/examples/ledger/money.scpp tests/examples/ledger/account.scpp tests/examples/ledger/book.scpp tests/examples/ledger/report.scpp tests/examples/ledger/main.scpp
```

`-D` and `-I` go to `preproc`. `-i` and `--ignore-environment` go to `preproc` and import no environment macros. `-o` names the file written by the last phase: the object file with `-c`, the IR file with `--emit-llvm`, and the executable otherwise. A path ending in `.bc` is LLVM IR. `-c`, `--emit-llvm`, and a `.bc` path take one source. `-g` goes to `scpp_cc` and to the Clang link. `-L` goes to the Clang link. `preproc` deletes nothing, and `scpp` leaves the intermediate files in place. One source still leaves `output.i` and `output.o`. Several sources leave `output.0.i`, `output.0.o`, and the same pair for each later file.
