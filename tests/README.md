# tests

`parse_test` runs the scanner and parser on one `.scpp` file and, when the parse succeeds, checks that the syntax tree was built. A normal build does not build `parse_test`. The `test` target does, and that build parses `examples/accept.scpp`. CTest runs that accept example, which must pass.

`compile_return` runs `scpp` on `examples/return-zero.scpp`, then checks that the written bitcode module verifies and defines `int main`. Each other `compile_*` test runs `scpp` on one short example and checks that one feature in the bitcode.

`cc_ast_text` runs `scpp_cc --ast-text` and checks the structured text. `cc_ast_dot` runs `scpp_cc --ast-dot` and checks the Graphviz file. `cc_ast_both` requests the text, the DOT file, and bitcode together.

`collect_live_root` runs `scpp` on `examples/live-root.scpp`, links that object with the collector, and runs it. The second `new` must trace the pointer returned by the first.

`run_return`, `run_print`, and `run_format` ask `scpp` for an executable and run it. `run_return` must exit 0. `run_print` must write `count is 1` and a newline. `run_format` must exit 0. `run_ledger` compiles `examples/ledger` as five translation units and requires the line `cash 40`.

`pp_*` runs `preproc` on one short input. A passing test compares the preprocessed text. A failing test checks that `preproc` exits non-zero and that the diagnostic names the problem. Examples that are not about the process environment pass `-i`, so an environment name does not change their text. The environment examples check an imported value, `-D` replacing it, `-U` removing it, and both `-i` and `--ignore-environment`. `preproc` is a normal build target. `scpp` runs it before `scpp_cc`.

`debug_answer` builds one executable with `-g` and one without. The `-g` executable names `main`, its `int` return type, and `debug-answer.scpp`. `gdb`, or `lldb` when `gdb` is absent, stops in `main` and shows `return 0`. `diag_no_main` compiles a translation unit that has no `main`, then requires `scpp` to reject the executable.

`phase_*` checks that each driver option reaches only the program that uses it. `-D`, `-I`, `-i`, and `--ignore-environment` reach `preproc`. `--emit-llvm` and `-c` reach `scpp_cc`, and `-o` then names that phase's file. `-g` reaches `scpp_cc` and Clang. `-L` reaches Clang. A program does not receive an option that belongs to another phase. `driver_default`, `driver_keep`, `driver_preprocess`, `driver_emit-llvm`, `driver_library`, `driver_include`, and `driver_library-order` check the default executable name, `-k` and `--keep`, `-E`, `--emit-llvm` for two files, `-l`, and a colon-separated `-I` or `-L` list. `driver_base-one` and `driver_base-two` check that one source and two sources write `name.i` and `name.o` from each input base name.

`install_prefix` configures a fresh tree and checks that the default prefix is `$HOME/opt/smallcpp`, including a home directory whose path contains spaces. An explicit `-DCMAKE_INSTALL_PREFIX` and a cached prefix stay in place. On CMake 3.29 and newer, the `CMAKE_INSTALL_PREFIX` environment variable selects the prefix, and `-D` wins over that variable. A missing or relative `HOME` fails unless a prefix was given. `prefix_paths` checks that an executable in `bin/` uses the directory above `bin/`, and that a compiled prefix is only the fallback. `install_layout` installs with a temporary `HOME` and `--prefix`. A missing or relative `HOME` fails the install. `DESTDIR` stages the tree and is not written into `config.ini`. The ini keys are the include path, the lib directory, the runtime archive, and the system libraries, including `libm.a`. A program includes `<exceptions>` through that file. A link of `sqrt` uses the recorded `libm` path, and `-l m` still wins. A missing ini falls back to the install prefix.

`lsp_session` starts `bin/scpp-lsp` and drives one session on its standard streams. The session initializes, opens a buffer, checks a diagnostic from `scpp`, goes to a definition, completes a name, replaces the buffer, saves it, and shuts down.

`exec_*` runs one short program. The program prints one line, and the test checks that line and the exit status. `exec_uncaught-exception` compiles with `-g`, throws, and does not catch. The program must abort, and the diagnostic must name the message and a stack with line numbers. `diag_*` feeds `scpp` a bad snippet. The compiler must exit non-zero, and the diagnostic must name the problem. `diag_bad-recover` requires the syntax error twice.
