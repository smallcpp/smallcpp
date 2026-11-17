# tests

`parse_test` runs the scanner and parser on one `.scpp` file and, when the parse succeeds, checks that the syntax tree was built. A normal build does not build `parse_test`. The `test` target does, and that build parses `examples/accept.scpp`. CTest runs that accept example, which must pass.

`compile_return` runs `scpp` on `examples/return-zero.scpp`, then checks that the written bitcode module verifies and defines `int answer`. Each other `compile_*` test runs `scpp` on one short example and checks that one feature in the bitcode.

`cc_ast_text` runs `scpp_cc --ast-text` and checks the structured text. `cc_ast_dot` runs `scpp_cc --ast-dot` and checks the Graphviz file. `cc_ast_both` requests the text, the DOT file, and bitcode together.

`collect_live_root` runs `scpp` on `examples/live-root.scpp`, links that object with the collector, and runs it. The second `new` must trace the pointer returned by the first.

`run_return`, `run_print`, and `run_format` ask `scpp` for an executable and run it. `run_return` must exit 0. `run_print` must write `count is 1` and a newline. `run_format` must exit 0.

`pp_*` runs `preproc` on one short input. A passing test compares the preprocessed text. A failing test checks that `preproc` exits non-zero and that the diagnostic names the problem. Examples that are not about the process environment pass `-i`, so an environment name does not change their text. The environment examples check an imported value, `-D` replacing it, `-U` removing it, and both `-i` and `--ignore-environment`. `preproc` is a normal build target. The shell does not call it.

`exec_*` runs one short program. The program prints one line, and the test checks that line and the exit status. `diag_*` feeds `scpp` a bad snippet. The compiler must exit non-zero, and the diagnostic must name the problem. `diag_bad-recover` requires the syntax error twice.
