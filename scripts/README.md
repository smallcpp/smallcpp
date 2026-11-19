# scripts

Helper scripts for this checkout. The root `README.md` is the build, test, and install guide.

- `setup [--clean] [debug|release|profile]` configures `build/` and starts a project shell. `build` in that shell compiles. `build test` runs the suite. `--clean` runs `clean` first. `debug`, `release`, and `profile` select the CMake build type.
- `clean` removes `bin/`, `build/`, `lib/`, `doc/out/`, and `share/`.
- `showdocs` builds the `docs` target and opens `doc/out/html/index.html` in Firefox. Run it inside the shell `setup` starts. This checkout defines no `docs` target.

No script installs, selects a prefix, or applies `DESTDIR`. Those steps are `cmake --install` from the project shell.
