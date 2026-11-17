# scpp

`scpp` coordinates one compilation. It reads the command line and runs `scpp_cc` with the input file and the output path. It does not parse, and it does not emit LLVM IR.

```sh
./bin/scpp tests/examples/return-zero.scpp answer.bc
./bin/scpp tests/examples/return-zero.scpp answer
./bin/scpp -o if.bc tests/examples/if.scpp
```

A path ending in `.bc` is bitcode. Any other path is an executable. `scpp_cc` does the parse and the lowering.
