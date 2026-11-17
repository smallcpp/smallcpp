# src

- `scpp` — the `scpp` driver. The binary is `bin/scpp`. It runs `preproc`, then `scpp_cc`, then Clang when linking. It does not parse or emit LLVM IR.
- `compiler` — the C frontend. The binary is `bin/scpp_cc`.
- `emit` — the C++ IR emitter. `scpp_cc` links it. The C header is `emit/emit.h`.
- `preproc` — the preprocessor. The binary is `bin/preproc`. `scpp` runs it. It deletes nothing.
- `cli` — command-line parsing shared by the programs.
- `runtime` — the collector, `print` and `format`, and the base `exception` class. An exception nothing catches aborts and prints a stack.
