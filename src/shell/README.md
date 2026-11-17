# shell

The shell parses one `.scpp` file, lowers the syntax tree to an LLVM module, verifies that module, and writes bitcode.

```sh
./bin/shell tests/examples/return-zero.scpp answer.bc
./bin/shell tests/examples/return-zero.scpp answer
./bin/shell tests/examples/if.scpp if.bc
```

This slice lowers scalar functions, calls, aggregates, namespaces, and classes: scalar types, parameters, locals, file-scope scalars, operators, control flow, calls, overloads, default arguments, varargs, `extern`, `static`, `inline`, `typedef`, `register`, string literals, structs, unions, enums, arrays, pointers, bit-fields, initializers, `namespace`, qualified names, `using`, methods, constructors, destructors, and bases. `const` and `volatile` are qualifiers. Empty `[]` on a parameter is a pointer. A namespace is not a runtime value. A class is an object type and `this` is a pointer. A virtual call is indirect. Access is checked at compile time. `new` calls the declared allocator `smallcpp_alloc` at a safepoint and runs a constructor when the type has one. `delete` evaluates its operand and does not deallocate. `nullptr` and `NULL` are the null pointer. A `try` body is an `invoke` and a handler is a landing pad. The personality is the declared function `__gxx_personality_v0`. `print` and `format` call the declared functions `smallcpp_print` and `smallcpp_format`. `format` returns a pointer that is recorded in the stack map. The runtime collector implements `smallcpp_alloc` and traces those stack maps. `delete` does not free. `smallcpp_print` and `smallcpp_format` are the C runtime linked into an executable. The IR is not printed as text. A path that does not end in `.bc` is that executable.
