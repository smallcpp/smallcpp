# examples

Inputs for `parse_test`. Source files use the `.scpp` extension.

- `accept.scpp` parses. It uses `class`, `namespace`, `try`, `catch`, `throw`, and `new`.
- `reject-second-base.scpp`, `reject-designated.scpp`, and `reject-class.scpp` do not.
- `reject-errors.scpp` has more than one error. `parse_test` prints more than one diagnostic and exits non-zero.
