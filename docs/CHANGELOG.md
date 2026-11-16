# Changelog

All notable changes to smallcpp are recorded in this file.

## Unreleased

### Added

- Flex scanner `src/compiler/smallcpp.l` for the smallcpp grammar.
- `parse_test` in `tests/parse_main.c`, and example inputs under `tests/examples/`.
- CMake builds for `src/compiler` and `tests`. The parser library is written to `lib/`, and `parse_test` to `bin/`.

### Changed

- Moved the grammar to `src/compiler/smallcpp.y`. `docs/smallcpp.y` is a relative symbolic link to that file.

## 2026-11-16

### Added

- Repository seed: license, readme, and `.gitignore` (`33c0057`).
- CMake scaffold, hello-world `shell` program, and the `src`, `docs`, `scripts`, and `tests` directories (`9ab6636`, merged to main in pull request #1 as `46e46da`).
- Generated binaries go to `bin/` and libraries to `lib/` (`6030c67`, merged in pull request #2 as `9ae000f`).
- Bison grammar for smallcpp (`05dc0f2`, merged in pull request #3 as `b4321c9`).
