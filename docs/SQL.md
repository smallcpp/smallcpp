# SQL

Include the header as `<sqlite>`. The classes live in namespace `Sqlite`. They are `Database`, `Connection`, `Value`, `Statement`, and `Transaction`. `Database` is the virtual interface. `Connection` implements it. A program holds a `Database *` and calls the virtual methods, so another server can replace `Connection` by overriding those methods. Link with `-l smallcpp_sqlite` and the database library.

A program opens a database, runs SQL, and closes the statements and transactions before it closes the connection. `close` returns false while one of them is still live. The destructor finalizes a statement, rolls back a transaction, or closes a connection when the program did not.

## Value

`SQL_NULL`, `SQL_INTEGER`, `SQL_REAL`, `SQL_TEXT`, and `SQL_BLOB` are 0 through 4. `Value()` is SQL NULL. `Value(long)` is an integer. `Value(double)` is a real. `Value(const char *)` is text. `Value(const void *, long)` is a blob. A null text pointer throws `Exception` with message `value`. A null blob pointer throws unless the size is 0.

`kind` returns the kind. `is_null` is true for SQL NULL. `as_integer`, `as_real`, `as_text`, and `as_blob` return the stored value. An accessor throws `value` when the kind differs. `blob_size` is the number of blob bytes. [`docs/examples/sql-value.scpp`](examples/sql-value.scpp) prints `sql-value 1`.

## Database and Connection

`Connection(path)` opens a read/write database. An empty path is a temporary file. `":memory:"` is in memory. Check `is_open` after the constructor.

`error` returns the text of the last failure, or an empty string when the last call succeeded. `execute` runs one SQL string and returns true on success. A failed string leaves the text in `error`. `set_busy_timeout` sets how many milliseconds a locked database may wait. `close` closes the connection when no statement or transaction is still live.

[`docs/examples/sql-execute.scpp`](examples/sql-execute.scpp) prints `sql-execute 1`. [`docs/examples/sql-timeout.scpp`](examples/sql-timeout.scpp) prints `sql-timeout 1`.

## Statement

`prepare` compiles one statement and returns it. A second statement in the same string returns `nullptr`. Bind indexes start at 1. `bind` stores a `Value *`. `step` returns true when a row is available. After the statement finishes or fails, another `step` returns false. `has_row` is true after a step that produced a row. `is_done` is true after the statement finishes. `failed` is true after an error. `value` reads a column of the current row as a `Value *`. Column indexes start at 0. `error` is the statement's message. `close` finalizes the statement.

[`docs/examples/sql-statement.scpp`](examples/sql-statement.scpp) prints `sql-statement 1`.

## Transaction

`begin_transaction` starts a transaction with `BEGIN IMMEDIATE` and returns it. `is_active` is true until `commit` or `rollback`. `commit` keeps the writes. `rollback` undoes them. `close` finishes the object's bookkeeping after the transaction has ended. Close the transaction before closing the connection.

[`docs/examples/sql-transaction.scpp`](examples/sql-transaction.scpp) prints `sql-transaction 1`.
