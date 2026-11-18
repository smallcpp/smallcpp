# Standard library reference

The standard library is a set of headers in `include/`, the same directory
as `<exceptions>`. The preprocessor searches that directory for
`#include <name>` before `/usr/local/include` and `/usr/include`. A header
name has no suffix.

```smallcpp
#include <math>
#include <sqlite>

int main(void) {
    double root;

    root = sqrt(9.0);
    print("{}\n", root);
    return 0;
}
```

`print` and `format` are part of the language. They take a literal format
string in C++23 syntax. They are not declared by these headers.

## How a library is reached

Most headers only declare C functions. A declaration is `extern` plus the
C type of each parameter and the result. The header also typedefs a POSIX
name to the matching compiler type, defines a platform struct the caller
has to allocate, and defines a macro where the C library uses a macro for
a constant or for `errno`. A direct header has no class, no function body,
and no second declaration language.

```smallcpp
extern double sqrt(double x);
```

The object name is the identifier in that declaration. One C function has
one declaration. A second function of the same name is an overload, and
the compiler then gives every overload a signature-based object name, so
the link no longer finds the C symbol.

A call with no header and no prototype is already an external `int`
function. The arguments at the call are the parameters. `atoi` can be
reached that way. `sqrt` returns `double`, and `fopen` returns a pointer,
so those calls go through a header. The header is the library interface.

The bodies are the platform C libraries. `scpp -l` names a library other
than libc. libc is on the link already. These headers do not include a
system `.h` file. A system header is C that this compiler does not accept
(`long double` is one example).

`<sqlite>` and `<gtk>` are the exception. Each public header declares
smallcpp classes. The implementation of those classes is smallcpp, and
that implementation calls the C library with the same `extern`
declarations. The public header does not list `sqlite3_*` or `gtk_*`.
`<cli>` is also smallcpp classes: there is no C library underneath it.
`<exceptions>` is the base class and is already shipped.

A class in a namespace is used by its class name after
`using namespace Sqlite;` or `using Sqlite::Database;`. `new` takes that
class name. A declaration's type is that name too. `Sqlite::Database` is
not a type specifier.

`const char *` stored in an object is not copied, except where a section
below says the library copies. The caller keeps those characters alive.
An empty string is a valid string. A null pointer is not a string.
`nullptr` and `NULL` are the null pointer.

Memory from `malloc` is not a collected object. Release it with `free`.
The collector does not free it. Memory from `new` is collected. `delete`
type-checks the pointer and runs the destructor. It does not deallocate.
After `delete`, the collector does not run that destructor again. When the
collector reclaims an object whose destructor has not run, it runs the
destructor. Reclaim order is not a sequence the program can rely on.

## Types

The compiler's integer and floating types are the library's numeric types.

| Compiler type | Width | POSIX names the headers typedef |
| --- | --- | --- |
| `int` | 32-bit signed | `pid_t`, `clockid_t` |
| `unsigned` | 32-bit unsigned | `socklen_t` |
| `long` | 64-bit signed | `ssize_t`, `off_t`, `time_t` |
| `unsigned long` | 64-bit unsigned | `size_t` |
| `double` | 64-bit | |
| `float` | 32-bit | |

`long` is 64 bits in this compiler. On the LP64 hosts the link uses, that
matches the platform `long`. A C function that returns `int` is declared
with `int`, including the functions that use a nonzero value as a
condition. Declaring one of those as `bool` would be a different ABI.

There is no `buffer`, `dict`, `int64`, `uint64`, or `fp64` type.
`String` is a growable array of `char`. `Array` is a growable array of
bytes. A program defines a `Vector`, `List`, `Tree`, or `Hash` with the
macros `VECTOR`, `LIST`, `TREE`, and `HASH`, one class for one type.
A `const char *` that is not a `String` is still a C string. A counted
sequence that is not one of those classes is a pointer plus a `long`
or an `unsigned long`. Bytes are `void *` plus a length.

## `<exceptions>`

`Exception` is the base exception class. Include it as `<exceptions>`.

```smallcpp
class Exception {
public:
    const char *message;
    Exception();
    Exception(const char *text);
};
```

`Exception()` stores a null message. `Exception(const char *)` stores that
pointer in `message` and does not copy it. A program allocates it with
`new` and throws the object:

```smallcpp
throw *new Exception("missing");
```

`catch (Exception error)` reads `error.message`. `catch (...)` catches
every type. A bare `throw` keeps the exception in flight. An exception
that nothing catches aborts the program and prints the message and a call
stack with line numbers.

Direct C calls do not throw. They return the C failure value and set
`errno`. `<sqlite>`, `<gtk>`, and `<cli>` throw `Exception` for the
failures named in their sections.

## `<stdarg>`

`#include <stdarg>`. A function with a trailing `...` reads the extra
arguments. `va_list` holds the position. `va_start` names the last
fixed parameter. `va_arg` returns the next argument as the type the
caller names. An `int` and a `const char *` are both read that way.
`va_copy` copies a list, and the two lists then walk independently.
`va_end` finishes a list. It is safe on the original and on a copy.
`format` and `print` take their values at the call and do not accept a
`va_list`. `va_list` only walks a function's own extra arguments.

```smallcpp
#include <stdarg>

int show(int n, ...) {
    va_list args;
    va_list copy;
    int value;
    const char *text;
    int again;

    va_start(args, n);
    va_copy(copy, args);
    value = va_arg(args, int);
    text = va_arg(args, const char *);
    again = va_arg(copy, int);
    va_end(args);
    va_end(copy);
    print("{} {} {:s} {}\n", n, value, text, again);
    return again;
}
```

## `<typeinfo>`

`#include <typeinfo>`. `typeid(expression)` and `typeid(Type)` yield a
`const TypeInfo *`. `==` and `!=` compare those pointers. A polymorphic
object seen through a base pointer uses the dynamic type from the vtable.

```smallcpp
#include <typeinfo>

class Base {
public:
    virtual int id(void);
};

int Base::id(void) {
    return 1;
}

class Derived : public Base {
public:
    int id(void);
};

int Derived::id(void) {
    return 2;
}

int main(void) {
    Base *object;
    const TypeInfo *info;

    object = new Derived();
    info = typeid(*object);
    if (info == typeid(Derived))
        print("derived\n");
    return 0;
}
```

## Collections

The collections are a small stand-in for a growable string, an array, a
list, a tree, and a hash table. They are not the C++ standard template
library. There are no templates, references, `auto`, lambdas, or
operator overloads. The language has no template syntax. A macro defines
the class for the type the program names.

A program allocates each object with `new`. A local declaration does not
run a constructor. Include `<string>` and `<array>` in one translation
unit, because those headers define the methods. Expand `VECTOR`, `LIST`,
`TREE`, or `HASH` for one class name in one translation unit.

`Array::address`, `String::at`, and `at` from `VECTOR`, `LIST`, and
`TREE` count a negative index from the end. `-1` is the last element.
`-2` is the one before it. An index past either end throws `Exception`
with message `index`. `replace`, `remove`, and `next` on `VECTOR`,
`LIST`, and `TREE` use that same rule. `insert` counts a negative
index from the end, appends when the index is past the end, and
prepends when the index is before the start. `from_end` takes a
non-negative index and walks from the tail.

`next` on `VECTOR`, `LIST`, and `TREE` is the iteration. Start a
`long` cursor at 0. While the cursor is less than `size`, call `next`
with the cursor's address. `next` returns the element at the cursor
and writes the following index. A null cursor throws `Exception` with
message `index`. A cursor past either end throws the same message. A
negative cursor counts from the end, and the stored cursor is the
following non-negative index.

`new` memory is collected. Growing an array abandons the old buffer.
The collector reclaims it. `delete` does not free it.

### `<array>`

`Array` is a growable array of bytes. `stride` is the element size.
`count` is how many elements are in use. `room` is how many elements
`bytes` can hold. The first allocation holds 4 elements.

```text
class Array {
public:
    char *bytes;
    long count;
    long room;
    long stride;
    Array(long element);
    void reserve(long need);
    char *address(long index);
    long size(void);
};
```

`Array(long)` stores that stride, or 1 when the argument is less than 1.
`reserve` grows `room` by doubling until it can hold `need` elements and
copies the bytes already in use. `address` returns the element at
`index`. A negative index counts from the end. `size` returns `count`.
`Vector` and `Hash` use `Array` as a base. `String` does too, with a
stride of 1.

### `<string>`

`String` is a growable array of `char` with a trailing null. `add`,
`append`, `prepend`, `insert`, and `replace` copy characters into the
string's own buffer. They do not keep the caller's pointer. An empty
string is valid. A null `const char *` or `String *` throws `Exception`
with message `string`.

```text
class String : public Array {
public:
    String();
    String(const String *other);
    void add(char value);
    void add(const char *text);
    long capacity(void);
    char at(long index);
    const char *chars(void) const;
    const char *c_str(void) const;
    int strip(void);
    int strip(char value);
    long find(char value);
    long find(const char *text);
    long find(const String *text);
    long find(long index, char value);
    long find(long index, const char *text);
    long find(long index, const String *text);
    long rfind(char value);
    long rfind(const char *text);
    long rfind(const String *text);
    long rfind(long index, char value);
    long rfind(long index, const char *text);
    long rfind(long index, const String *text);
    long cmp(const String *left, const String *right);
    long cmp(const char *left, const char *right);
    long cmp(const char *left, const String *right);
    long cmp(const String *left, const char *right);
    void append(char value);
    void append(const char *text);
    void append(const String *text);
    void prepend(char value);
    void prepend(const char *text);
    void prepend(const String *text);
    void insert(long index, char value);
    void insert(long index, const char *text);
    void insert(long index, const String *text);
    void truncate(long from, long to);
    void replace(char needle, char value);
    void replace(char needle, const char *text);
    void replace(char needle, const String *text);
    void replace(const char *needle, char value);
    void replace(const char *needle, const char *text);
    void replace(const char *needle, const String *text);
    void replace(const String *needle, char value);
    void replace(const String *needle, const char *text);
    void replace(const String *needle, const String *text);
    StringVec *split(const char *separator);
    StringVec *split(void);
    String *join(StringVec *source, const char *separator);
    String *join(StringVec *source);
    const char *token(long *cursor, char separator);
    const char *token(long *cursor, const char *separator);
};
```

`String(const String *)` copies the characters into a new buffer.
`size`, inherited from `Array`, is the character count. `capacity` is
the allocated byte count, including the null. `at` returns the character
at `index`. A negative index counts from the end. `chars` and `c_str`
return the buffer. It stays valid until the next operation that
replaces it.

`strip()` removes `' '`, `'\n'`, and `'\r'` from both ends.
`strip(char)` removes every instance of that character. Both return
how many characters were removed. `find` returns the index where the
match begins, or `-1`. `rfind` searches toward the front. A negative
start counts from the end. A `find` start past the end returns `-1`.
An `rfind` index is the last place a match may begin. An index past
the end searches the whole string. `cmp` returns what `strcmp` would
return. `append` adds at the end and `prepend` inserts at the front.
`insert` appends when the index is past the end and prepends when it
is before the start. `truncate` removes characters from the first
index up to but not including the second, and clamps each bound. When
the first bound is past the second, nothing is removed. `replace`
replaces the first match. A missing needle leaves the string unchanged.

`<string>` defines `StringVec` with `VECTOR(StringVec, String *)`.
`split(const char *)` returns a new `StringVec`. The caller
receives that pointer. Each piece is a new `String *`. A missing
separator returns one copy of the whole string. Consecutive
separators yield an empty string. An empty separator throws
`Exception` with message `string`. A null separator does too.
`split()` separates on `' '`, `'\n'`, and `'\r'`, the characters
`strip()` treats as space. Each of those characters is a
separator, so a run of them yields an empty piece.

`join(StringVec *, const char *)` returns a new `String *`. It
does not clear this string. The separator is written between
pieces. An empty separator concatenates them. `join(StringVec *)`
uses one space. A null source, a null separator, or a null element
throws `Exception` with message `string`.

```smallcpp
String *text;
StringVec *parts;
String *joined;

text = new String();
text->append("a,,b");
parts = text->split(",");
joined = text->join(parts, "-");
```

`joined` is `a--b`. `text` is still `a,,b`.

`token` walks the string from a caller-owned `long` cursor. The
cursor starts at 0. A `char` separator is that character. A
`const char *` separator is the set of separator characters. `token`
skips those characters and does not return an empty token. It
returns null when no token remains. An empty separator or a null
cursor throws `Exception` with message `string`. The pointer is a
null-terminated copy in storage the string owns. It stays valid
until the next call that changes the string. `token` does not write
the character buffer.

```smallcpp
#include <string>

int main(void) {
    String *text;

    text = new String();
    text->add("hello");
    text->add(" world");
    print("{:s}\n", text->chars());
    return 0;
}
```

### `<vector>`

`VECTOR(Name, Type)` defines a class `Name` that stores `Type` by
assignment. The class extends `Array`. `sizeof(Type)` is the stride.

```text
class Name : public Array {
public:
    Name();
    void add(Type value);
    Type at(long index);
    long find(Type value);
    void replace(long index, Type value);
    void insert(long index, Type value);
    void remove(long index);
    Type next(long *cursor);
};
```

`size` is `Array::size`. `at` returns the value at `index`. A negative
index counts from the end. `find` returns the index of the first `==`
match, or `-1`. `replace` stores the value at the index. `insert`
shifts the later values up by one. `remove` shifts them down and drops
the last slot. `next` is the cursor walk above. A program writes the
macro once:

```smallcpp
#include <vector>

VECTOR(IntVec, int)

int main(void) {
    IntVec *values;

    values = new IntVec();
    values->add(10);
    print("{}\n", values->at(0));
    return 0;
}
```

### `<list>`

`LIST(Name, Type)` defines a doubly linked list. `add` appends. `at`
walks from the head. A negative index counts from the end, so `at(-1)`
is the last value. `from_end` walks from the tail with a non-negative
index. `size` is the number of nodes. `find`, `replace`, `insert`,
`remove`, and `next` follow the vector rules. `insert` links a node at
the index. `remove` unlinks that node. `from_end` stays.

```smallcpp
#include <list>

LIST(IntList, int)
```

`IntList *items; items = new IntList(); items->add(4);` then
`items->at(0)` is 4 and `items->from_end(0)` is the last value.

### `<tree>`

`TREE(Name, Type)` defines a balanced binary tree. Order is the virtual
method `compare(Type, Type)`. A negative result means the first value
comes first, zero means equal, and a positive result means the first
value comes later, the same as `strcmp`. The default calls `String.cmp`
when `Type` is `const char *` or `String *`. Any other type uses `<`.
A subclass overrides `compare` to change the order. `add` and `find`
call that method. `add` places a value with `compare`. A value that
compares equal to one already in the tree is left in place. `insert`
places the value at the in-order index. A negative index counts from
the end. An index past the end appends, and an index before the start
prepends. After `insert`, `at` of that index is the value. `replace`
writes the value at the index and leaves it there. `root` returns the
root value, or throws `Exception` with message `empty` when the tree
has no nodes. `at` walks the values in order. A negative index counts
from the end. `size` is the number of nodes. `find` returns the
in-order index of the first zero result, or `-1`. `remove` deletes the
value at the in-order index. `next` walks in order.

```smallcpp
#include <tree>

TREE(IntTree, int)
```

`add(1)`, then `add(2)`, then `add(3)` leaves 2 at the root. `at(0)`,
`at(1)`, and `at(2)` are 1, 2, and 3. `at(-1)` is 3.

### `<hash>`

`HASH(Name, Key, Value)` defines an open hash table. The slots live in
an `Array`. Equality is the virtual method `compare(Key, Key)`. Zero
means equal. The default calls `String.cmp` when `Key` is `const char *`
or `String *`. Any other type uses `==`. A subclass overrides `compare`
to change equality. There is no hash callback. `const char *` and
`String *` are hashed by their characters, and any other key is hashed
as its bytes. `put`, `find`, `replace`, and `remove` call `compare` on
the filled slots, so an override does not have to agree with that hash.
`put` inserts or replaces. `find` writes `*out` and returns 1, or
returns 0 when the key is absent. A null `out` throws `Exception` with
message `hash`. `size` is the number of stored keys. `replace` stores
the value when the key is present and puts the pair when the key is
missing. `remove` deletes that key. A missing key leaves the table
unchanged. There is no second insert. `next(long *, Key *, Value *)`
walks filled slots. The cursor starts at 0 and names a slot. A call
that finds a filled slot returns 1, writes the key and value, and
stores the following slot. A finished walk returns 0. A null cursor,
key, or value pointer throws `Exception` with message `hash`. The order
is slot order.

```smallcpp
#include <hash>

HASH(IntMap, int, int)

int main(void) {
    IntMap *map;
    int found;

    map = new IntMap();
    map->put(1, 10);
    found = 0;
    map->find(1, &found);
    print("{}\n", found);
    return 0;
}
```

## Direct headers

`errno` is the calling thread's error number, defined by `<errno>` from
the platform accessor. A successful call does not clear it. `perror` and
`strerror` turn the number into text. The text `strerror` returns may be
overwritten by the next call that uses it.

`exit(int code)` ends the process with the low 8 bits of `code` as the
status. It flushes C stdio. It does not run destructors, `catch` blocks,
or the collector. `return` from `main` is the path that leaves the
process through the language. `abort` ends the process without flushing.

### Program arguments and the environment

`main` is `int main(int argc, char **argv)`. Those parameters are the
command line. The program may ignore them and write `int main(void)`.

`<stdlib>` declares:

```text
void exit(int code);
void abort(void);
char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
int atoi(const char *text);
long atol(const char *text);
long strtol(const char *text, char **end, int base);
double strtod(const char *text, char **end);
void *malloc(unsigned long size);
void *realloc(void *ptr, unsigned long size);
void free(void *ptr);
```

`getenv` returns a pointer into the environment, or `nullptr` when the
name is unset. The program does not free it. `setenv` and `unsetenv`
return 0 on success and -1 on failure, and set `errno`. `overwrite` is
nonzero to replace an existing value.

### Files

`<stdio>` declares the stream operations. `File` is the stdio stream.
`stdin`, `stdout`, and `stderr` are `File *`.

```text
File *fopen(const char *path, const char *mode);
int fclose(File *stream);
unsigned long fread(void *data, unsigned long size,
    unsigned long count, File *stream);
unsigned long fwrite(const void *data, unsigned long size,
    unsigned long count, File *stream);
char *fgets(char *data, int size, File *stream);
int fputs(const char *text, File *stream);
int fprintf(File *stream, const char *text, ...);
int printf(const char *text, ...);
int snprintf(char *data, unsigned long size,
    const char *text, ...);
int fflush(File *stream);
int fseek(File *stream, long offset, int whence);
long ftell(File *stream);
void rewind(File *stream);
int feof(File *stream);
int ferror(File *stream);
void clearerr(File *stream);
int remove(const char *path);
int rename(const char *old_path, const char *new_path);
```

The format argument is the parameter `text`, because `format` is a
keyword. `SEEK_SET`, `SEEK_CUR`, and `SEEK_END` are the `whence`
values. A binary mode is `"rb"`, `"wb"`, `"ab"`, or `"r+b"`. `fopen`
returns `nullptr` and sets `errno` when the file cannot be opened.

`fread` and `fwrite` return the number of members transferred. A short
read at end of file is a short count with `feof` true and `ferror` false.
A read error sets `ferror` and `errno`. `fgets` keeps the newline when it
fits, and returns `nullptr` at end of file or on error. `fputs` returns
a nonnegative value on success and `EOF` on failure.

`fseek` returns 0 on success and -1 on failure. `ftell` returns the
position, or -1. `fflush` returns 0 on success and `EOF` on failure.
`fclose` returns 0 on success and `EOF` on failure. Close a stream when
the program is finished with it. The collector does not close it.

`<stdio>` also declares `getline` as POSIX provides it:

```text
long getline(char **line, unsigned long *cap, File *stream);
```

`getline` allocates with `malloc` and keeps the delimiter. The caller owns
`*line` and releases it with `free`. The collector does not free it.

### Filesystem

`<stat>` declares `stat`, `lstat`, and `mkdir`. `<stdlib>` declares
`realpath`. `<unistd>` declares `unlink`, `rmdir`, `chdir`, `getcwd`,
`access`, `read`, `write`, `close`, `lseek`, and `copy_file_range`.
`<dirent>` reads a directory. `<fcntl>` declares `open`.

```text
int stat(const char *path, struct stat *buf);
int lstat(const char *path, struct stat *buf);
int mkdir(const char *path, unsigned mode);
int unlink(const char *path);
int rmdir(const char *path);
int chdir(const char *path);
char *getcwd(char *buf, unsigned long size);
char *realpath(const char *path, char *resolved);
int access(const char *path, int mode);
DIR *opendir(const char *path);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dir);
int open(const char *path, int flags, ...);
long read(int fd, void *buf, unsigned long count);
long write(int fd, const void *buf, unsigned long count);
int close(int fd);
long lseek(int fd, long offset, int whence);
```

`struct stat` matches the platform. The fields a program reads are
`st_mode` and `st_size`. `S_ISREG(st_mode)` and `S_ISDIR(st_mode)` are the
type tests. `st_size` is a `long`. `stat` and `lstat` return 0 on success
and -1 on failure.

`mkdir`, `unlink`, `rmdir`, `chdir`, `access`, and `close` return 0 on
success and -1 on failure. `access` modes are `F_OK`, `R_OK`, `W_OK`, and
`X_OK`. `open` returns a descriptor or -1. Its flags include `O_RDONLY`,
`O_WRONLY`, `O_RDWR`, `O_CREAT`, `O_TRUNC`, `O_APPEND`, and `O_CLOEXEC`.
`read` and `write` return the byte count, 0 at end of file for `read`, or
-1. A short write is a short count.

`getcwd` stores the path in `buf` and returns `buf`, or returns `nullptr`
and sets `errno`. `realpath` resolves an existing path, including symbolic
links. When `resolved` is `nullptr` it allocates with `malloc` and the
caller frees the pointer. A missing path returns `nullptr`.

`readdir` returns one entry, or `nullptr` at the end or on error. `errno`
is unchanged at a normal end, and set on error. `d_name` is the name. `.`
and `..` appear. `closedir` returns 0 on success and -1 on failure.

`<libgen>` declares `basename` and `dirname`. Each may modify the buffer
it is given and may return a pointer into that buffer or into static
storage. The program copies the result when it needs to keep it. There is
no library call that joins two paths or that returns a file extension.
`copy_file_range`, declared by `<unistd>`, copies a range of bytes on
Linux:

```text
long copy_file_range(int in_fd, long *off_in, int out_fd,
    long *off_out, unsigned long len, unsigned int flags);
```

It returns the bytes copied, or -1. The caller repeats it until the file
is copied. A null offset pointer uses the descriptor's current position.

Temporary files use `<stdlib>`:

```text
int mkstemp(char *pattern);
char *mkdtemp(char *pattern);
```

`pattern` ends in `XXXXXX`, which these calls replace. `mkstemp` creates
a file with mode `0600` and returns an open descriptor, or -1. The caller
closes the descriptor. `mkdtemp` creates a directory and returns
`template`, or `nullptr`.

### Glob

`<glob>` declares POSIX `glob`. Patterns are `*`, `?`, and `[...]`.
Results are sorted. A relative pattern produces relative paths. Recursive
`**` and brace expansion are outside this header.

```text
int glob(const char *pattern, int flags,
    int (*errfunc)(const char *path, int error), glob_t *out);
void globfree(glob_t *out);
```

`glob_t` matches the platform. The program reads `gl_pathc` and
`gl_pathv`. `glob` returns 0 when it finds matches. `GLOB_NOMATCH` means
there were none, and `gl_pathc` is 0. Any other result is an error.
`globfree` releases the result. `errfunc` may be `nullptr`.

### Standard streams

`stdin`, `stdout`, and `stderr` are the process streams. `fread`, `fgets`,
`read`, and `getline` consume `stdin`. `fwrite`, `fputs`, `fprintf`, and
`printf` write `stdout`. `fprintf(stderr, ...)` writes `stderr`. `fflush`
on a stream, or `fflush(nullptr)` for every stream, pushes the buffers.
`print` is the formatted output that checks a literal format string.
`printf` is the C function and does not check the format at compile time.

```smallcpp
#include <stdio>

int main(void) {
    fputs("hello\n", stdout);
    fflush(stdout);
    return 0;
}
```

### `<math>`

`<math>` externs the `double` functions in libm, and the `float` function
of each name with an `f` suffix (`sqrtf`, `sinf`, and the rest). Link with
`-l m`.

```text
double fabs(double x);
double fmin(double a, double b);
double fmax(double a, double b);
double floor(double x);
double ceil(double x);
double round(double x);
double trunc(double x);
double sqrt(double x);
double cbrt(double x);
double pow(double base, double exp);
double exp(double x);
double log(double x);
double log10(double x);
double log2(double x);
double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);
double sinh(double x);
double cosh(double x);
double tanh(double x);
long labs(long x);
```

Trigonometric functions take radians. A degree value converts as
`radians * (MATH_PI / 180.0)`, and the other way as
`degrees * (180.0 / MATH_PI)`. The header defines `MATH_PI`, `MATH_E`, and
`MATH_TAU`.

Results follow libm. A domain error may return a NaN and set `errno` to
`EDOM`. A range error may set `errno` to `ERANGE`. The header does not
turn those into `Exception`.

```smallcpp
#include <math>

int main(void) {
    double root;

    root = sqrt(9.0);
    print("{}\n", root);
    return 0;
}
```

### Sockets

`<socket>` is IPv4 TCP. Link with libc.

```text
int socket(int domain, int type, int protocol);
int connect(int fd, const struct sockaddr *addr, socklen_t len);
int bind(int fd, const struct sockaddr *addr, socklen_t len);
int listen(int fd, int backlog);
int accept(int fd, struct sockaddr *addr, socklen_t *len);
long send(int fd, const void *buf, unsigned long len, int flags);
long recv(int fd, void *buf, unsigned long len, int flags);
int setsockopt(int fd, int level, int optname,
    const void *value, socklen_t len);
int getaddrinfo(const char *host, const char *service,
    const struct addrinfo *hints, struct addrinfo **out);
void freeaddrinfo(struct addrinfo *out);
const char *gai_strerror(int err);
unsigned short htons(unsigned short value);
unsigned int htonl(unsigned int value);
int inet_pton(int family, const char *text, void *dst);
```

`AF_INET`, `SOCK_STREAM`, and `IPPROTO_TCP` select IPv4 TCP.
`struct sockaddr_in` has `sin_family`, `sin_port`, and `sin_addr`.
`getaddrinfo` fills `out`. `freeaddrinfo` releases it. A nonzero
`getaddrinfo` result is an error, and `gai_strerror` describes it. That
result is not `errno`.

`socket`, `connect`, `bind`, `listen`, `accept`, and `setsockopt` return
-1 on failure and set `errno`. `accept` returns a new descriptor.
`send` and `recv` return the byte count, or -1. `recv` returns 0 when the
peer has closed. `close` releases the descriptor. A receive timeout is
`SO_RCVTIMEO` with a `struct timeval` (`tv_sec`, `tv_usec`). The caller
picks a nonnegative timeout.

`send` and `recv` move bytes. They do not treat the buffer as UTF-8.

### Time

`<time>` declares wall time and a monotonic clock.

```text
time_t time(time_t *out);
int clock_gettime(clockid_t id, struct timespec *out);
int nanosleep(const struct timespec *req, struct timespec *rem);
```

`CLOCK_REALTIME` is Unix wall time. `CLOCK_MONOTONIC` is for elapsed
intervals. `struct timespec` has `tv_sec` and `tv_nsec`. `time` returns
the seconds, or -1. `clock_gettime` returns 0, or -1 when the clock
cannot be read. `nanosleep` returns 0, or -1. When `errno` is `EINTR`,
`rem` holds the remaining time and the caller can sleep again.

### Processes

`<spawn>` launches a process without a shell. It declares
`posix_spawnp`, the file-action and attribute calls, and `waitpid`.
`<signal>` declares `kill` and the signal macros. `<unistd>` declares
`pipe`. The executable is `posix_spawnp`. A name with no `/` is searched
in `PATH`. The argument vector includes argv[0]. Spaces, quotes, `$`,
`;`, `|`, `*`, and empty strings are ordinary characters in that vector.

```text
int posix_spawnp(pid_t *pid, const char *file,
    const posix_spawn_file_actions_t *actions,
    const posix_spawnattr_t *attr, char *const argv[],
    char *const envp[]);
int posix_spawn_file_actions_init(posix_spawn_file_actions_t *a);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *a);
int posix_spawn_file_actions_adddup2(
    posix_spawn_file_actions_t *a, int fd, int new_fd);
int posix_spawn_file_actions_addclose(
    posix_spawn_file_actions_t *a, int fd);
int posix_spawnattr_init(posix_spawnattr_t *attr);
int posix_spawnattr_destroy(posix_spawnattr_t *attr);
int posix_spawnattr_setflags(posix_spawnattr_t *attr, short flags);
int posix_spawnattr_setpgroup(posix_spawnattr_t *attr, pid_t group);
int waitpid(pid_t pid, int *status, int options);
int kill(pid_t pid, int sig);
int pipe(int *fds);
```

`envp` null means the child inherits the environment. File actions bind
the child's standard descriptors to pipes. `POSIX_SPAWN_SETPGROUP` with a
group of 0 puts the child in its own process group. A spawn result of 0
stores the pid. Any other result is an error number, and `errno` is
left unchanged.

`waitpid` reaps the child. `WIFEXITED` and `WEXITSTATUS` read a normal
exit. `WIFSIGNALED` and `WTERMSIG` read a signal death. `WNOHANG` returns
0 when the child is still running. The caller reaps every child it
starts. A forgotten child keeps running, and an unreaped child is a
zombie. Nothing in the collector kills a child.

Read both output pipes while the child runs. A child that fills a pipe
the parent is not reading will block. `pipe` takes `int *fds`, the
address of the first of two descriptors. A loop of `read` on each end
is the whole capture. Bytes are bytes. A partial UTF-8 sequence is the
caller's problem.

`kill(-pid, SIGTERM)` signals the child's process group, and
`kill(-pid, SIGKILL)` ends it when the grace period has passed. Signal
only a pid that `waitpid` has not reaped yet. `<signal>` declares
`SIGTERM`, `SIGKILL`, `SIGPIPE`, and `sigaction`. A parent that writes a
pipe sets `SIGPIPE` to `SIG_IGN` when a dead child should not kill the
parent.

There is no environment-override helper beyond the `envp` argument, and
no pseudo-terminal helper. This is a POSIX host.

### Terminal

`<unistd>` declares `isatty`. `<ioctl>` declares `ioctl` and
`struct winsize`.

```text
int isatty(int fd);
int ioctl(int fd, unsigned long request, ...);
```

`isatty` returns 1 when the descriptor is a terminal and 0 otherwise.
`TIOCGWINSZ` fills `ws_col` and `ws_row`. When stdout is not a terminal,
or the size cannot be read, the caller treats the size as 0. These calls
do not change the terminal mode.

Color is a property of the environment. Stdout is a color terminal when
`isatty` is true for it, `getenv("TERM")` is set and is not `dumb`, and
`getenv("NO_COLOR")` is `nullptr`.

### Random bytes

`<random>` declares the Linux `getrandom` system call.

```text
long getrandom(void *buf, unsigned long size, unsigned int flags);
```

It returns the number of bytes written, or -1 and sets `errno`.
`GRND_NONBLOCK` returns `EAGAIN` instead of blocking. A short count means
the caller asks for the rest. `open("/dev/urandom", O_RDONLY)` and `read`
are the fallback when `getrandom` returns `ENOSYS`. The caller checks a
size before converting it to `unsigned long`.

### Threads and synchronization

`<pthread>` externs pthreads and unnamed POSIX semaphores. Link with
`-l pthread`. A thread is an operating-system thread. A blocking call
blocks that thread. There is no language runtime lock.

```text
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
    void *(*start)(void *), void *arg);
int pthread_join(pthread_t thread, void **result);
int pthread_detach(pthread_t thread);
pthread_t pthread_self(void);
int pthread_mutex_init(pthread_mutex_t *mutex,
    const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_cond_init(pthread_cond_t *cond,
    const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int sem_init(sem_t *sem, int shared, unsigned value);
int sem_destroy(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);
```

The mutex, condition, attribute, and `sem_t` types match the platform.
Initialize a mutex with `pthread_mutex_init`. The default mutex is not
recursive. `pthread_mutex_lock` blocks until the mutex is free. A second
lock by the owner returns `EDEADLK`. `pthread_mutex_unlock` by a thread
that does not own it returns `EPERM`. `pthread_mutex_destroy` on a locked
mutex returns `EBUSY`. These functions return 0 on success and the error
number on failure. They do not set `errno`.

`pthread_cond_wait` requires the caller to own the mutex. It unlocks the
mutex while it waits and locks it again before returning. `signal` wakes
one waiter. `broadcast` wakes all. The caller rechecks its condition after
every wake. `pthread_cond_destroy` with waiters returns `EBUSY`.

`sem_init` with a nonzero `shared` is a process-shared semaphore. This
library uses 0. A negative initial count is a caller bug: the parameter
is `unsigned`. `sem_wait` and `sem_post` return 0, or -1 and set `errno`.
`sem_wait` can return `EINTR`. `sem_destroy` runs after no thread waits
or posts.

`pthread_create` returns 0 and stores the thread id. `start` is a
function pointer. The argument pointer stays live until the thread reads
it. `pthread_join` reaps the thread. Destroy a mutex, condition, or
semaphore only after its users have stopped.

## `<cli>`

`<cli>` is a command-line parser written in smallcpp, for programs written
in smallcpp. The compiler driver keeps its own C++ parser. This header
does not replace that parser.

The classes are `CommandLine`, `Option`, and `Positional`, in namespace
`Cli`. Enumerators are `CLI_FLAG`, `CLI_COUNTER`, `CLI_STRING`,
`CLI_NUMBER`, `CLI_NONE`, `CLI_HELP`, and `CLI_VERSION`.

```text
class Option {
public:
    const char *short_name;
    const char *long_name;
    const char *name;
    int value_type;
    bool is_list;
    char list_separator;
    int action;
    Option();
};

class Positional {
public:
    const char *name;
    bool required;
    bool is_list;
    Positional();
};

class CommandLine {
public:
    CommandLine(const char *program, const char *summary,
        const char *version);
    void set_capture_remainder(bool enabled);
    void add_option(Option *option);
    void add_positional(Positional *positional);
    void parse(int argc, char **argv);
    bool was_provided(const char *name);
    bool switch_value(const char *name);
    int count(const char *name);
    bool has_value(const char *name);
    const char *value(const char *name);
    int value_count(const char *name);
    const char *value_at(const char *name, int index);
    bool contains_item(const char *name, const char *item);
    int positional_count();
    const char *positional_at(int index);
    int remainder_count();
    const char *remainder_at(int index);
    int current_action();
    const char *help_text();
    const char *version_text();
};
```

`Option()` sets `value_type` to `CLI_FLAG`, `action` to `CLI_NONE`,
`list_separator` to `:`, and the pointers to null. `Positional()` sets
`required` and `is_list` to false and `name` to null. The caller fills
the fields, then passes the object to `add_option` or `add_positional`.
The `CommandLine` keeps that pointer. The caller keeps the object alive.

`parse` copies the argument text it retains. `value`, `value_at`,
`positional_at`, and `remainder_at` stay valid until the next `parse` or
until the `CommandLine` is destroyed. A list option splits on
`list_separator`. Short options accept an attached value (`-oFILE`,
`-o=FILE`, `-IFOO:BAR`).

`CLI_HELP` and `CLI_VERSION` stop `parse` before required checks.
`current_action` is that enumerator, and the program prints `help_text`
or `version_text` and calls `exit`. Any other usage error throws
`Exception` with message `usage`. `set_capture_remainder(true)` sends
arguments after `--` to the remainder accessors.

```smallcpp
#include <cli>
#include <stdlib>

using namespace Cli;

int main(int argc, char **argv) {
    CommandLine *command;
    Option *output;
    Positional *input;

    command = new CommandLine("demo", "demo options", "1.0");
    output = new Option();
    output->short_name = "o";
    output->long_name = "output";
    output->name = "output";
    output->value_type = CLI_STRING;
    command->add_option(output);
    input = new Positional();
    input->name = "input";
    input->required = true;
    command->add_positional(input);
    command->parse(argc, argv);
    print("{:s}\n", command->value("output"));
    return 0;
}
```

## `<sqlite>`

`<sqlite>` is a smallcpp database interface. The public header declares
classes. It does not name sqlite3 and it does not declare `sqlite3_*`.
The implementation calls those symbols with `extern`. Link a program that
uses the wrapper with `-l smallcpp_sqlite -l sqlite3`. A program that
does not use it does not link sqlite3. Building the wrapper requires the
sqlite3 development files.

The classes are in namespace `Sqlite`. `Database` is the virtual
interface. `Connection` is the class the sqlite3 library implements. A
caller holds a `Database *` and does not call through `Connection`. A
subclass of `Database` overrides the virtual methods for another server.
The value kinds are `SQL_NULL`, `SQL_INTEGER`, `SQL_REAL`, `SQL_TEXT`,
and `SQL_BLOB`, with values 0 through 4.

```text
class Value {
public:
    Value();
    Value(long number);
    Value(double number);
    Value(const char *text);
    Value(const void *bytes, long size);
    int kind() const;
    bool is_null() const;
    long as_integer() const;
    double as_real() const;
    const char *as_text() const;
    const void *as_blob() const;
    long blob_size() const;
};

class Database {
public:
    virtual ~Database();
    virtual bool is_open() const = 0;
    virtual const char *error() const = 0;
    virtual bool execute(const char *sql) = 0;
    virtual Statement *prepare(const char *sql) = 0;
    virtual bool set_busy_timeout(long milliseconds) = 0;
    virtual Transaction *begin_transaction() = 0;
    virtual bool close() = 0;
};

class Connection : public Database {
public:
    Connection(const char *path);
    ~Connection();
    bool is_open() const;
    const char *error() const;
    bool execute(const char *sql);
    Statement *prepare(const char *sql);
    bool set_busy_timeout(long milliseconds);
    Transaction *begin_transaction();
    bool close();
};

class Statement {
public:
    ~Statement();
    const char *error() const;
    bool bind(int index, Value *value);
    bool step();
    bool has_row() const;
    bool is_done() const;
    bool failed() const;
    Value *value(int column) const;
    bool close();
};

class Transaction {
public:
    ~Transaction();
    bool is_active() const;
    bool commit();
    bool rollback();
    bool close();
};
```

`Value()` is SQL NULL. NULL is that value. It is not a missing argument,
a zero, or an empty string. `Value(const char *)` throws `Exception` when
the pointer is null, and copies the bytes. `Value(const void *, long)`
throws when the pointer is null and the size is not 0. An empty blob is
a null pointer with size 0, or any pointer with size 0. The accessors
throw `Exception` when `kind()` is a different variant. They do not
convert.

`Connection(path)` opens or creates a read/write database. Store that
object in a `Database *`. The calls on that pointer are virtual. Check
`is_open()` and read `error()` when it is false. An empty path asks
SQLite for a temporary database, which SQLite deletes when the connection
closes. `":memory:"` is an in-memory database. `error()` points at text
the connection or statement owns until the next call that changes it.

`execute` runs one SQL string, including a PRAGMA, and returns false on
failure. `prepare` compiles one statement. Trailing whitespace is
accepted. A second statement, or a compile error, returns `nullptr` and
sets `Connection::error()`. Bind indexes start at 1. Column indexes start
at 0. `bind` stores a copy of text and blob bytes, so the `Value` only
has to live through the call. Binding is parameterized. It does not
interpolate text into the SQL.

`step` returns true for a row and false for completion or for an error.
`is_done` and `failed` tell those apart. After completion or error,
another `step` returns false and does not call SQLite again, so a
finished insert is not run twice. `value` returns a `Value` the
statement owns, and it is valid while `has_row()` is true. The wrapper
copies text and blob bytes out of SQLite. The pointers from `as_text`
and `as_blob` stay valid until the next `step` or `close` on that
statement. The kind comes from SQLite's storage class for that result,
not from the column's declared affinity.

Close each statement, then the connection. `Connection::close` returns
false, leaves the connection open, and sets `error()` when a statement or
a transaction is still live. `Transaction::close` rolls back an active
transaction. A transaction starts with `BEGIN IMMEDIATE`.

The destructor is the fallback. `~Statement` finalizes an open statement.
`~Transaction` rolls back an active transaction. `~Connection` closes an
open connection after those are finished. `delete` runs that destructor
and does not deallocate. Explicit `close` is the path that reports a
failure to the program. A `catch` that is leaving closes the statement
and the connection, then uses a bare `throw`. A close failure on that
path stays in `error()` and leaves the original exception in flight.

`set_busy_timeout` is one of the virtual methods. The `Connection`
implementation sets the sqlite busy timeout. A subclass overrides it.
The SQL text is SQLite's SQL.

```smallcpp
#include <exceptions>
#include <sqlite>
#include <string>

using namespace Sqlite;

int main(void) {
    Database *database;
    Statement *insert;
    Statement *query;
    Value *stored;

    database = new Connection(":memory:");
    if (!database->is_open())
        throw *new Exception(database->error());
    if (!database->execute("CREATE TABLE sample (value)"))
        throw *new Exception(database->error());
    insert = database->prepare(
        "INSERT INTO sample VALUES (?)");
    if (insert == nullptr)
        throw *new Exception(database->error());
    stored = new Value("parameterized text");
    if (!insert->bind(1, stored))
        throw *new Exception(insert->error());
    insert->step();
    if (insert->failed() || !insert->is_done())
        throw *new Exception(insert->error());
    insert->close();
    query = database->prepare("SELECT value FROM sample");
    if (query == nullptr || !query->step())
        throw *new Exception(database->error());
    if (strcmp(query->value(0)->as_text(),
            "parameterized text") != 0)
        throw *new Exception("roundtrip mismatch");
    query->close();
    database->close();
    print("round trip\n");
    return 0;
}
```

## `<gtk>`

`<gtk>` is a smallcpp wrapper around GTK 4. It is the classes below. It
is not a complete GTK binding, and the public header does not declare the
GTK C functions. The implementation calls those functions with `extern`.
Link with `-l smallcpp_gtk -l gtk-4 -l gio-2.0 -l gobject-2.0 -l glib-2.0
-l pango-1.0 -l pangocairo-1.0 -l cairo`. Building the wrapper requires
the GTK 4 development files. A program that does not use the wrapper does
not link those libraries.

The classes are in namespace `Gtk`. `Widget` is the base of `Window`,
`Box`, `Label`, `Button`, `Entry`, `CheckButton`, `ScrolledWindow`,
`HeaderBar`, `MenuButton`, `Popover`, `Paned`, `Notebook`,
`ColumnView`, `Dialog`, `DropDown`, `SpinButton`, `Picture`,
`ProgressBar`, `Spinner`, and `TextView`. `AlertDialog`, `FileDialog`,
`TextBuffer`, `TextMark`, and `TextIter` are not widgets. Enumerators
are `GTK_HORIZONTAL` (0), `GTK_VERTICAL` (1), `GTK_POLICY_ALWAYS`,
`GTK_POLICY_AUTOMATIC`, `GTK_POLICY_NEVER`, and `GTK_POLICY_EXTERNAL`.

```text
class SignalConnection {
public:
    void disconnect();
    bool connected() const;
};

class Application {
public:
    Application(const char *id);
    SignalConnection on_activate(void (*action)(void *),
        void *data);
    int run();
    long post(void (*action)(void *), void *data);
    bool cancel(long token);
};

class Widget {
public:
    virtual ~Widget();
    void dispose();
    bool disposed() const;
    void set_visible(bool value);
    void set_sensitive(bool value);
    void set_h_expand(bool value);
    void set_v_expand(bool value);
    void set_width_request(long width);
    void add_css_class(const char *name);
    void remove_css_class(const char *name);
    void set_tooltip(const char *text);
};

class Window : public Widget {
public:
    Window(const char *title);
    void set_title(const char *title);
    const char *title() const;
    void set_default_size(long width, long height);
    void set_child(Widget *child);
    void remove(Widget *child);
    void present();
    void close();
    SignalConnection on_close_request(bool (*action)(void *),
        void *data);
};

class Box : public Widget {
public:
    Box(int orientation, int spacing);
    void append(Widget *child);
    void remove(Widget *child);
};

class Label : public Widget {
public:
    Label(const char *text);
    void set_text(const char *text);
    const char *text() const;
};

class Button : public Widget {
public:
    Button(const char *label);
    void set_label(const char *label);
    const char *label() const;
    SignalConnection on_clicked(void (*action)(void *),
        void *data);
};

class Entry : public Widget {
public:
    Entry(const char *text);
    void set_text(const char *text);
    const char *text() const;
    SignalConnection on_changed(void (*action)(void *),
        void *data);
};

class CheckButton : public Widget {
public:
    CheckButton(const char *label);
    void set_label(const char *label);
    const char *label() const;
    void set_active(bool value);
    bool active() const;
    SignalConnection on_toggled(void (*action)(void *, bool),
        void *data);
};

class ScrolledWindow : public Widget {
public:
    ScrolledWindow();
    void set_child(Widget *child);
    void remove(Widget *child);
    void set_policy(int horizontal, int vertical);
};

class HeaderBar : public Widget {
public:
    HeaderBar();
    void set_title(const char *title);
    const char *title() const;
    void set_show_title_buttons(bool value);
    bool show_title_buttons() const;
    void pack_start(Widget *child);
    void pack_end(Widget *child);
};

class MenuButton : public Widget {
public:
    MenuButton(const char *label);
    void set_label(const char *label);
    const char *label() const;
    void set_menu(const char **labels, long count,
        void (*action)(void *, const char *), void *data);
};

class Popover : public Widget {
public:
    Popover();
    void set_child(Widget *child);
    void popup();
    void popdown();
};

class Paned : public Widget {
public:
    Paned(int orientation);
    int orientation() const;
    void set_start_child(Widget *child);
    void set_end_child(Widget *child);
    void set_position(long position);
    long position() const;
};

class Notebook : public Widget {
public:
    Notebook();
    long append_page(Widget *child, const char *title);
    void remove_page(long index);
    long current_page() const;
    void set_current_page(long index);
    SignalConnection on_switch_page(
        void (*action)(void *, long), void *data);
};

class ColumnView : public Widget {
public:
    ColumnView();
    void append_column(const char *title);
    void append_row(const char **cells, long count);
    void clear_rows();
    long row_count() const;
    long column_count() const;
    const char *cell(long row, long column) const;
    SignalConnection on_activate(
        void (*action)(void *, long), void *data);
};

class Dialog : public Widget {
public:
    Dialog(const char *title);
    void set_title(const char *title);
    const char *title() const;
    void set_content(Widget *child);
    void add_button(const char *label, int response);
    SignalConnection on_response(
        void (*action)(void *, int), void *data);
    void present();
};

class AlertDialog {
public:
    AlertDialog(const char *message, const char *detail);
    const char *message() const;
    const char *detail() const;
    void present(void (*confirm)(void *), void *data);
};

class FileDialog {
public:
    FileDialog();
    void set_title(const char *title);
    const char *title() const;
    void open(void (*action)(void *, const char *),
        void *data);
    void save(void (*action)(void *, const char *),
        void *data);
};

class DropDown : public Widget {
public:
    DropDown();
    void set_items(const char **items, long count);
    void set_selected(long index);
    long selected() const;
    const char *selected_string() const;
    SignalConnection on_changed(void (*action)(void *),
        void *data);
};

class SpinButton : public Widget {
public:
    SpinButton(double min, double max, double value);
    double min() const;
    double max() const;
    double value() const;
    void set_value(double value);
    SignalConnection on_changed(void (*action)(void *),
        void *data);
};

class Picture : public Widget {
public:
    Picture();
    void set_file(const char *path);
    const char *file() const;
};

class ProgressBar : public Widget {
public:
    ProgressBar();
    void set_fraction(double fraction);
    double fraction() const;
};

class Spinner : public Widget {
public:
    Spinner();
    void start();
    void stop();
    bool spinning() const;
};

class TextIter {
public:
    TextIter(long line, long offset);
    long line() const;
    long offset() const;
};

class TextMark {
public:
    const char *name() const;
    void move(long line, long offset);
    long line() const;
    long offset() const;
};

class TextBuffer {
public:
    TextBuffer();
    void set_text(const char *text);
    const char *get_text();
    void insert(TextIter *at, const char *text);
    void delete_range(TextIter *start, TextIter *end);
    void place_cursor(TextIter *at);
    void select_range(TextIter *start, TextIter *end);
    bool selection_bounds(TextIter *start, TextIter *end);
    void create_tag(const char *name,
        const char *foreground);
    void apply_tag(const char *name,
        TextIter *start, TextIter *end);
    void remove_tag(const char *name,
        TextIter *start, TextIter *end);
    void undo();
    void redo();
    SignalConnection on_changed(void (*action)(void *),
        void *data);
    TextMark *create_mark(const char *name,
        long line, long offset);
};

class TextView : public Widget {
public:
    TextView(TextBuffer *buffer);
    void set_editable(bool value);
    bool editable() const;
    void set_monospace(bool value);
    bool monospace() const;
    void scroll_to(long line, long offset);
    SignalConnection on_move_cursor(
        void (*action)(void *, long, long), void *data);
};
```

Construct one `Application`. The id is a GApplication id, such as
`"org.example.Demo"`. GTK copies it. Install `on_activate` before `run`.
Create widgets on the GUI thread during activation, or later on that same
thread. `run` calls GTK's application run, including single-instance
activation. A second process with the same id delivers activation to the
first process, and the second process does not run its own activate
handler. `run` is single-use. When it returns, the wrapper cancels
pending posts, disconnects signals, disposes every widget, and releases
the native application.

`post` is the one method a thread other than the GUI thread may call. It
queues `action(data)` on the GUI thread and returns a positive token.
`cancel` and every widget operation run on the GUI thread. Calling them
from another thread throws `Exception` with message `thread`. `cancel`
returns true when the token was still pending. A token is not reused.
`post` before `run`, `post` after `run` has returned, and a full queue of
64 posts throw `Exception` with message `post`. A second `run` throws
`run`. A second `Application` throws `gtk`. The first widget constructor
calls `gtk_init_check`. When that fails, the constructor throws `display`.

A callback is a function pointer plus a `void *`. There is no closure
type. The wrapper retains `data` until `disconnect` or `dispose`. When
`data` points at a `new` object, that field keeps the object reachable.
`on_close_request` returns true to reject the close. `on_toggled` passes
the new active state. `on_changed` runs after the text changes, and the
program reads `text()`. `MenuButton::set_menu` installs one action for
every label. That action receives the chosen label. `Notebook`
`on_switch_page` and `ColumnView` `on_activate` pass an index.
`Dialog` `on_response` passes the response id. `AlertDialog::present`
calls its confirm function when the button choice is 0.
`FileDialog::open` and `save` call their function with the chosen path,
or with null when the chooser is cancelled. The path stays reachable from
the dialog until the next open or save finishes. `TextView`
`on_move_cursor` runs after the cursor moves and passes the new line and
character offset.

`dispose` disposes the widget and its children and invalidates aliases. A
use after `dispose` throws `Exception` with message `disposed`. The
destructor does not dispose. `delete` on a wrapper therefore does not
tear the GTK object down, and the collector does not tear it down either.
Call `dispose` when the native widget should go away. `AlertDialog`,
`FileDialog`, and `TextBuffer` release their native object from the
destructor.

Text is UTF-8. An empty string is valid. A null pointer throws
`Exception` with message `null`. Invalid UTF-8 throws `utf8`. Getters
return `""` when GTK has no text. A getter's pointer stays valid until
the next setter or `dispose` on that object. A child argument is a
`Widget *`. A `Label *` or other subclass is passed with a cast, as in
`(Widget *)label`, because a call matches a pointer argument exactly.
The child is live and has no parent. A window or a dialog is not a
child. Either case throws `parent`. `Window`, `ScrolledWindow`,
`Popover`, and a dialog's content each have one child. Setting another
before `remove` throws `child`. `Box` holds several children.
`HeaderBar` packs several at the start and the end. `Paned` keeps one
start child and one end child. An orientation other than
`GTK_HORIZONTAL` or `GTK_VERTICAL`, a policy outside 0 through 3, or an
index that does not name an existing row, page, or character throws
`range`. A GTK call that does not produce the object throws `gtk`.

`HeaderBar` title text is a label the bar owns. `Picture::set_file`
loads that path. There is no separate image class. `ProgressBar`
stores a fraction. `Spinner::start` and `stop` change `spinning()`.
`DropDown` holds a list of strings. `selected()` is `-1` when nothing
is selected. `SpinButton` uses a step of 1 and reads `min`, `max`, and
`value`. `ColumnView` is a text grid: `append_column` adds a title,
`append_row` adds one string per column, and `cell` reads that string.
`on_activate` passes the row index. The factory that paints the cells
stays inside the wrapper.

`TextIter` stores a line and a character offset. It is not a live GTK
iterator. Copy `line()` and `offset()` to keep a position, and pass a
`TextIter` built from those numbers back to the buffer. `TextMark` is
created with `create_mark`. `move` places it, and `line()` and
`offset()` read it. The mark keeps its place in the text when the
buffer changes. An insertion at the mark leaves the mark after the new
text. `TextBuffer` edits UTF-8. `delete_range` removes a half-open
range. `create_tag` names a tag and a foreground color. `apply_tag` and
`remove_tag` use that name. `undo` and `redo` walk the buffer's own
history. Scrolling is `TextView::scroll_to`, not a buffer method.
`TextView` is constructed with the buffer it shows.

```smallcpp
#include <gtk>

using namespace Gtk;

static void activate(void *data) {
    Application *app;
    Window *window;
    Label *label;

    app = (Application *)data;
    window = new Window("Hello");
    label = new Label("Hello");
    window->set_child((Widget *)label);
    window->present();
}

int main(void) {
    Application *app;

    app = new Application("org.example.Hello");
    app->on_activate(activate, app);
    app->run();
    return 0;
}
```

## Outside this library

The language has no `import`, no package alias, and no package version
directory. Inline C, a C facade of runtime functions, and a callback
registration runtime are not part of the library. URL parsing, a
portable SQL layer for a second database, and a process object with a
pump thread are not headers. Integer range syntax and collection slices
are not operators of this language.
