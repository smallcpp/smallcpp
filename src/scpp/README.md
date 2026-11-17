# scpp

`scpp` coordinates one compilation. It runs `preproc`, then `scpp_cc`, and, for an executable, Clang. It does not parse, and it does not emit LLVM IR. Each option is passed only to the phase that uses it.

```sh
./bin/scpp --emit-llvm tests/examples/return-zero.scpp answer.bc
./bin/scpp tests/examples/return-zero.scpp answer
./bin/scpp -c -o answer.o tests/examples/return-zero.scpp
```

`-D` and `-I` go to `preproc`. `-i` and `--ignore-environment` go to `preproc` and import no environment macros. `-o` names the file written by the last phase: the object file with `-c`, the IR file with `--emit-llvm`, and the executable otherwise. A path ending in `.bc` is LLVM IR. `-g` goes to `scpp_cc` and to the Clang link. `-L` goes to the Clang link. `preproc` deletes nothing, and `scpp` leaves the intermediate files in place.
