# examples

Inputs for `parse_test`. Source files use the `.scpp` extension.

- `accept.scpp` parses. It uses `class`, `namespace`, `try`, `catch`, `throw`, and `new`. The test fails if those syntax-tree nodes are missing.
- `namespace.scpp` parses a namespace. The test fails if the namespace node is missing.
- `class-base.scpp` parses a class with one base and a method. The test fails if the class, base, or method node is missing.
- `try-catch.scpp` parses `try`, `catch`, and `throw`. The test fails if any of those nodes is missing.
- `variadic.scpp` parses a parameter list that ends with `...`. The test fails if the ellipsis node is missing.
- `print-format.scpp` parses. `print` and `format` are native calls. The string literal uses C++23 replacement fields, and the arguments follow it. The test fails if those nodes are missing.
- `tree-shape.scpp` parses a namespace, a derived class, `try`/`catch`/`throw`, a variadic parameter, and `print`/`format`. The test checks the shape of that tree, not only that parsing exits 0.
- `reject-second-base.scpp`, `reject-designated.scpp`, and `reject-class.scpp` do not.
- `reject-errors.scpp` has more than one error. `parse_test` prints more than one diagnostic and exits non-zero.
