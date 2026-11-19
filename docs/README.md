# docs

Project documentation.

- [`LANGUAGE.md`](LANGUAGE.md) is the language reference, including the collection headers.
- [`RUNTIME.md`](RUNTIME.md) is the runtime library, one section per header.
- [`SQL.md`](SQL.md) is the SQL interface.
- [`GTK.md`](GTK.md) is the GTK interface, including the text editor.
- [`STDLIB.md`](STDLIB.md) is the earlier standard-library reference.
- [`smallcpp.y`](smallcpp.y) is the grammar with no actions. [`smallcpp.l`](smallcpp.l) is the scanner. Both follow `src/compiler`.
- [`examples/`](examples) holds the programs those documents name. CTest compiles each one.
- `scpp-format` is the source formatter. The repository README says how to run it on a file, on standard input, and in place.
