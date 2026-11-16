# Changelog

All notable changes to smallcpp are recorded in this file.

## Unreleased

### Fixed

- Merging the syntax tree into the error-recovery grammar dropped `%locations` and `%define parse.error custom` while leaving `yyreport_syntax_error` and the scanner's column tracking in place. The generated parser did not compile, so `cmake --build` stopped before the build finished. Those directives are restored, and `docs/smallcpp.y` and `docs/smallcpp.l` match `src/compiler` again.

### Added

- The parser builds a syntax tree. Node kinds are defined in `src/compiler/ast.h` and constructed in `src/compiler/ast.c`. The grammar actions in `src/compiler/smallcpp.y` cover translation units, namespaces, classes, methods, statements, try/catch/throw, and new. There is no code generation.
- CTest runs `accept.scpp`. That test must build the syntax tree and pass.
- CTest runs the parser examples. `accept.scpp` must pass, and each reject example must fail. New parser checks are registered in `tests/CMakeLists.txt`.
- `docs/smallcpp.y` is a copy of `src/compiler/smallcpp.y`, not a symbolic link. `docs/smallcpp.l` is a copy of `src/compiler/smallcpp.l`. The sources in `src/compiler` stay in place.
- Scanner and parser diagnostics include the file, line, and column.
- After a syntax error, the parser resynchronizes at a semicolon and continues. An invalid character is reported and skipped, so one file can report more than one error.
- `tests/examples/reject-errors.scpp` contains several errors.

## 2026-11-16

### Added

- Repository seed: license, readme, and `.gitignore` (`33c0057`).
- CMake scaffold, hello-world `shell` program, and the `src`, `docs`, `scripts`, and `tests` directories (`9ab6636`, merged to main in pull request #1 as `46e46da`).
- Generated binaries go to `bin/` and libraries to `lib/` (`6030c67`, merged in pull request #2 as `9ae000f`).
- Bison grammar for smallcpp (`05dc0f2`, merged in pull request #3 as `b4321c9`).
- Flex scanner, `parse_test`, and the compiler and test CMake builds (`f883fd7`, merged in pull request #4 as `c741163`). The grammar lives in `src/compiler/smallcpp.y`, and `docs/smallcpp.y` links to it.

### Changed

- Example sources use the `.scpp` extension (`2f69bd2`, merged in pull request #5 as `0ad0eb0`).
- Keywords are the plain C++ spellings. No keyword has an `sc_` prefix. `class`, `namespace`, `public`, `private`, `protected`, `virtual`, `try`, `catch`, `throw`, `new`, and `this` are not macros. A namespace is `namespace name { ... }`. Allocation is a `new` expression.
