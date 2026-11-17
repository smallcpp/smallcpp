# shell

The shell parses one `.scpp` file, lowers the syntax tree to an LLVM module, verifies that module, and writes bitcode.

```sh
./bin/shell tests/examples/return-zero.scpp answer.bc
./bin/shell tests/examples/if.scpp if.bc
```

This slice lowers scalar functions, calls, aggregates, namespaces, and classes: scalar types, parameters, locals, file-scope scalars, operators, control flow, calls, overloads, default arguments, varargs, `extern`, `static`, `inline`, `typedef`, `register`, string literals, structs, unions, enums, arrays, pointers, bit-fields, initializers, `namespace`, qualified names, `using`, methods, constructors, destructors, and bases. `const` and `volatile` are qualifiers. Empty `[]` on a parameter is a pointer. A namespace is not a runtime value. A class is an object type and `this` is a pointer. A virtual call is indirect. Access is checked at compile time. `new`, `delete`, exceptions, `print`, and `format` stay later. The IR is not printed as text, and no executable is linked.
