# shell

The shell parses one `.scpp` file, lowers the syntax tree to an LLVM module, verifies that module, and writes bitcode.

```sh
./bin/shell tests/examples/return-zero.scpp answer.bc
./bin/shell tests/examples/if.scpp if.bc
```

This slice lowers scalar functions, calls, aggregates, and namespaces: scalar types, parameters, locals, file-scope scalars, operators, control flow, calls, overloads, default arguments, varargs, `extern`, `static`, `inline`, `typedef`, `register`, string literals, structs, unions, enums, arrays, pointers, bit-fields, initializers, `namespace`, qualified names, and `using`. `const` and `volatile` are qualifiers. Empty `[]` on a parameter is a pointer. A namespace is not a runtime value. Classes are rejected. `print` and `format` stay builtins for a later C runtime. The IR is not printed as text, and no executable is linked.
