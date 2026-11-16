# Changelog

All notable changes to smallcpp are recorded in this file.

## Unreleased

### Changed

- `docs/smallcpp.y` is a copy of `src/compiler/smallcpp.y`, not a symbolic link. `docs/smallcpp.l` is a copy of `src/compiler/smallcpp.l`. The sources in `src/compiler` stay in place.

## 2026-11-16

### Added

- Repository seed: license, readme, and `.gitignore` (`33c0057`).
- CMake scaffold, hello-world `shell` program, and the `src`, `docs`, `scripts`, and `tests` directories (`9ab6636`, merged to main in pull request #1 as `46e46da`).
- Generated binaries go to `bin/` and libraries to `lib/` (`6030c67`, merged in pull request #2 as `9ae000f`).
- Bison grammar for smallcpp (`05dc0f2`, merged in pull request #3 as `b4321c9`).
- Flex scanner, `parse_test`, and the compiler and test CMake builds (`f883fd7`, merged in pull request #4 as `c741163`). The grammar lives in `src/compiler/smallcpp.y`.

### Changed

- Example sources use the `.scpp` extension (`2f69bd2`, merged in pull request #5 as `0ad0eb0`).
- Keywords are the plain C++ spellings. No keyword has an `sc_` prefix. `class`, `namespace`, `public`, `private`, `protected`, `virtual`, `try`, `catch`, `throw`, `new`, and `this` are not macros. A namespace is `namespace name { ... }`. Allocation is a `new` expression.
