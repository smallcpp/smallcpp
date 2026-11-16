# shell

The shell parses one `.scpp` file, lowers the syntax tree to an LLVM module, verifies that module, and writes bitcode.

```sh
./bin/shell tests/examples/return-zero.scpp answer.bc
```

This slice compiles one `int` function with no parameters that returns an integer literal. `print` and `format` stay builtins for a later C runtime. The IR is not printed as text, and no executable is linked.
