# tests

`parse_test` runs the scanner and parser on one `.scpp` file and, when the parse succeeds, checks that the syntax tree was built. The build parses `examples/accept.scpp`. CTest runs that accept example, which must pass.

`compile_return` runs the shell on `examples/return-zero.scpp`, then checks that the written bitcode module verifies and defines `int answer`. Each other `compile_*` test runs the shell on one short example and checks that one feature in the bitcode.

`collect_live_root` runs the shell on `examples/live-root.scpp`, links that object with the collector, and runs it. The second `new` must trace the pointer returned by the first.

`run_return`, `run_print`, and `run_format` ask the shell for an executable and run it. `run_return` must exit 0. `run_print` must write `count is 1` and a newline. `run_format` must exit 0.
