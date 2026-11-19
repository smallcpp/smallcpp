# Language server

`scpp-lsp` is the language server for `.scpp` files. A normal
build writes it to `bin/scpp-lsp`, next to `scpp`. Install
places that same program in the prefix `bin/` directory.

The server reads Language Server Protocol messages on standard
input and writes them on standard output. It takes no
arguments. An editor starts the binary and keeps the process.
There is no window.

```sh
./bin/scpp-lsp
```

`initialize` answers with full-document sync, go to
definition, and completion. The completion triggers are `.`,
`:`, and `>`. `initialized` is ignored. `shutdown` answers
null, and the following `exit` stops the process with status
0. `textDocument/didOpen`, `textDocument/didChange`,
`textDocument/didSave`, and `textDocument/didClose` keep the
open buffer. A change replaces the whole buffer.

Diagnostics are produced by running `scpp` on the buffer.
That run is the preprocessor and the compiler frontend.
Each diagnostic is an error. The message is the compiler's
text.

Go to definition and completion use names from the parser.
The index blanks preprocessor directive lines, so names that
exist only in a header are not offered. A file with no
directives keeps the lines the editor has. The server is a
smallcpp program. The parser library supplies the names, and
`scpp` supplies the diagnostics.
