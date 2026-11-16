# examples

Inputs for `parse_test`. Source files use the `.scpp` extension.

- `accept.scpp` parses. It uses `class`, `namespace`, `try`, `catch`, `throw`, and `new`. The test fails if those syntax-tree nodes are missing.
- `namespace.scpp` parses a namespace. The test fails if the namespace node is missing.
- `class-base.scpp` parses a class with one base and a method. The test fails if the class, base, or method node is missing.
- `try-catch.scpp` parses `try`, `catch`, and `throw`. The test fails if any of those nodes is missing.
- `catch-all.scpp` parses `catch (...)` and a `catch` with a parameter. The `...` is not a parameter. The test fails if either form is missing.
- `variadic.scpp` parses a parameter list that ends with `...`. The test fails if the ellipsis node is missing.
- `print-format.scpp` parses. `print` and `format` are native calls. The string literal uses C++23 replacement fields, and the arguments follow it. The test fails if those nodes are missing.
- `tree-shape.scpp` parses a namespace, a derived class, `try`/`catch`/`throw`, a variadic parameter, and `print`/`format`. The test checks the shape of that tree, not only that parsing exits 0.
- `null.scpp` uses `nullptr` and `NULL`. The test fails unless both are the same kind of node.
- `delete.scpp` uses `new` and `delete`. `new` creates a pointer. `delete` is a no-op and its operand is a pointer.
- `reject-delete.scpp` deletes a non-pointer and must fail.
- `ctor-dtor.scpp` parses a constructor and a destructor, including an initializer list, both inside the class and out of line. The test fails if those nodes are missing.
- `default-arg.scpp` parses a parameter written `name = expression`. The test fails if that default is missing.
- `using.scpp` parses `using namespace name;` and `using name;`. The test fails if either form is missing. An anonymous namespace still does not parse.
- `overload.scpp` parses two functions named `sum` with different parameter lists. The test fails unless both are present. Operator overloading does not parse.
- `multi-base.scpp` parses a class with two bases and a constructor that initializes both. The test fails if either base or either base initializer is missing.
- `super.scpp` parses `super` and `this`. `super` names the base. The test fails if either keyword is missing.
- `reject-designated.scpp` and `reject-class.scpp` do not parse.
- `reject-errors.scpp` has more than one error. `parse_test` prints more than one diagnostic and exits non-zero.
