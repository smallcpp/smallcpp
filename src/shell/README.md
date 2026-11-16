# shell

The shell parses one `.scpp` file, lowers the syntax tree to an LLVM module, verifies that module, and writes bitcode.

```sh
./bin/shell tests/examples/return-zero.scpp answer.bc
./bin/shell tests/examples/if.scpp if.bc
```

This slice lowers scalar functions: scalar types, parameters, locals, file-scope scalars, operators, and control flow. `const` and `volatile` are qualifiers. Calls, aggregates, namespaces, and classes are rejected. `print` and `format` stay builtins for a later C runtime. The IR is not printed as text, and no executable is linked.
