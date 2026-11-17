# src

- `shell` — the `scpp` driver. The binary is `bin/scpp`. It coordinates phases and does not parse or emit LLVM IR.
- `compiler` — the C frontend. The binary is `bin/scpp_cc`.
- `emit` — the C++ IR emitter. `scpp_cc` links it. The C header is `emit/emit.h`.
- `preproc` — the standalone preprocessor. The binary is `bin/preproc`. The shell does not call it.
- `cli` — command-line parsing shared by the programs.
- `runtime` — the collector and the bodies of `print` and `format`.
