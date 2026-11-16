# smallcpp

Initial C++ CMake scaffold. The `shell` target is a hello-world program.

## Build

```sh
cmake -S . -B build
cmake --build build
./bin/shell
./bin/parse_test tests/examples/accept.small
```

Binaries are written to `bin/` and libraries to `lib/`.

## Layout

- `src/shell` — shell executable
- `src/compiler` — Bison grammar and Flex scanner
- `src/preprocessor` — source directory for a later target
- `docs` — documentation (`docs/smallcpp.y` links to the grammar)
- `scripts` — helper scripts
- `tests` — `parse_test` driver
- `tests/examples` — parser inputs
