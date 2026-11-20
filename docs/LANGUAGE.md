# The smallcpp language

smallcpp compiles a `.scpp` file to a native program. The syntax is the overlap of C99 and C++11, plus classes, inheritance, exceptions, namespaces, and precise garbage collection. [`docs/smallcpp.y`](smallcpp.y) is that grammar with the compiler's actions removed. A rule in that file is a form the compiler accepts. This page says how to write those forms, and how to use the collection headers.

A program starts at `int main`. [`docs/examples/lang-main.scpp`](examples/lang-main.scpp) prints `lang-main 0`. `print` and `format` are part of the language. They are not declared in a header. `print` writes a line and flushes when the program returns. `{}` reads an `int`. `{:s}` reads a string. `{:g}` reads a `double`. `{:c}` reads a character stored as an `int`. [`docs/examples/lang-print.scpp`](examples/lang-print.scpp) prints `lang-print 2`. [`docs/examples/lang-format.scpp`](examples/lang-format.scpp) prints `lang-format n 4`. [`docs/examples/lang-string.scpp`](examples/lang-string.scpp) prints `lang-string hello`.

Source is preprocessed before it is parsed. `#include <name>` reads a header from the include path. Each `-I` directory is searched first. The next directory is the `include` key in `~/.config/smallcpp/config.ini`. `cmake --install` writes that file. The other keys are the library paths, including `libm` for `libm.a`, and they are listed in the install notes and in [`RUNTIME.md`](RUNTIME.md). When the file is missing, the header directory is `include` under the prefix that contains the compiler's `bin/` directory, then the configured prefix. The compile does not fail. The headers below are included that way. A class from one of those headers is defined in a `.shpp` file, and the extensionless header includes it. Method bodies outside the class are linked from a `.scpp` file. A method written inside the class stays in the `.shpp`.

## Types and declarations

The integer types are `char`, `short`, `int`, `long`, and their `unsigned` forms. `float` and `double` are floating types. `bool` is a type, and `true` and `false` are values. `void` is the type of a function that returns nothing, and of a pointer to memory with no element type. A cast changes the type of a value. [`docs/examples/lang-types.scpp`](examples/lang-types.scpp) prints `lang-types 1`. [`docs/examples/lang-void.scpp`](examples/lang-void.scpp) prints `lang-void 4`. [`docs/examples/lang-cast.scpp`](examples/lang-cast.scpp) prints `lang-cast 65`.

`const` marks a value that the program does not write. `volatile` marks a value that may change outside the program. [`docs/examples/lang-const.scpp`](examples/lang-const.scpp) prints `lang-const 3`. [`docs/examples/lang-volatile.scpp`](examples/lang-volatile.scpp) prints `lang-volatile 1`.

`typedef` gives a type another name. [`docs/examples/lang-typedef.scpp`](examples/lang-typedef.scpp) prints `lang-typedef 6`.

A declaration may stand at file scope. [`docs/examples/lang-global.scpp`](examples/lang-global.scpp) prints `lang-global 6`. `auto` is the C storage class. It is not a type. [`docs/examples/lang-auto.scpp`](examples/lang-auto.scpp) prints `lang-auto 3`.

`nullptr` and `NULL` are the null pointer. [`docs/examples/lang-null.scpp`](examples/lang-null.scpp) prints `lang-null 1`.

## Statements

`if` and `else` choose a statement. The `else` binds to the nearest `if`. [`docs/examples/lang-if.scpp`](examples/lang-if.scpp) prints `lang-if 2`.

`switch` matches an integer against `case` labels and `default`. [`docs/examples/lang-switch.scpp`](examples/lang-switch.scpp) prints `lang-switch 7`.

`while` tests before the body. `do` tests after the body. `for` runs an initializer, a test, and a step. The initializer may declare a variable. [`docs/examples/lang-while.scpp`](examples/lang-while.scpp) prints `lang-while 3`. [`docs/examples/lang-do.scpp`](examples/lang-do.scpp) prints `lang-do 1`. [`docs/examples/lang-for.scpp`](examples/lang-for.scpp) prints `lang-for 6`. [`docs/examples/lang-for-decl.scpp`](examples/lang-for-decl.scpp) prints `lang-for-decl 6`.

`break` leaves the nearest loop or `switch`. `continue` starts the next iteration. `goto` jumps to a label in the same function. [`docs/examples/lang-break.scpp`](examples/lang-break.scpp) prints `lang-break 6`. [`docs/examples/lang-continue.scpp`](examples/lang-continue.scpp) prints `lang-continue 8`. [`docs/examples/lang-goto.scpp`](examples/lang-goto.scpp) prints `lang-goto 1`.

An empty statement is a semicolon. A compound statement is a brace block. [`docs/examples/lang-empty.scpp`](examples/lang-empty.scpp) prints `lang-empty 7`.

## Expressions

Arithmetic, bitwise, relational, and logical operators have their C meanings. `~`, unary `-`, and `!` bind tightly. `&&` and `||` skip the right operand when the left operand decides the result. [`docs/examples/lang-ops.scpp`](examples/lang-ops.scpp) prints `lang-ops 1`. [`docs/examples/lang-short-circuit.scpp`](examples/lang-short-circuit.scpp) prints `lang-short-circuit 0 0 1 0`.

Assignment writes the left operand. `++` and `--` change a number before or after the value is read. The comma operator evaluates both sides and yields the right side. `?:` chooses one of two expressions. [`docs/examples/lang-assign.scpp`](examples/lang-assign.scpp) prints `lang-assign 2`. [`docs/examples/lang-inc.scpp`](examples/lang-inc.scpp) prints `lang-inc 4 6 6 4`. [`docs/examples/lang-comma.scpp`](examples/lang-comma.scpp) prints `lang-comma 3`. [`docs/examples/lang-conditional.scpp`](examples/lang-conditional.scpp) prints `lang-conditional 2`.

`sizeof` yields the size of a type or an expression. [`docs/examples/lang-sizeof.scpp`](examples/lang-sizeof.scpp) prints `lang-sizeof 4`.

A pointer holds an address. `*` reads the object. `&` takes the address. An array name decays to a pointer to its first element. A function pointer stores a function, and a call through it passes the arguments. An array of function pointers is indexed and then called. [`docs/examples/lang-pointer.scpp`](examples/lang-pointer.scpp) prints `lang-pointer 6`. [`docs/examples/lang-array.scpp`](examples/lang-array.scpp) prints `lang-array 5`. [`docs/examples/lang-fn-ptr.scpp`](examples/lang-fn-ptr.scpp) prints `lang-fn-ptr 7`. [`docs/examples/lang-fn-array.scpp`](examples/lang-fn-array.scpp) prints `lang-fn-array 7 3`.

## Functions

A function names its return type, its parameters, and its body. `()` means no parameters. A call passes the arguments in order. [`docs/examples/lang-call.scpp`](examples/lang-call.scpp) prints `lang-call 5`.

Two functions may share a name when their parameter lists differ. The call selects the function whose parameters match the arguments. [`docs/examples/lang-overload.scpp`](examples/lang-overload.scpp) prints `lang-overload 3 3`.

A trailing parameter may be written `name = expression`. A call may omit that argument, and the expression runs at the call. A parameter after a default has a default too. [`docs/examples/lang-default.scpp`](examples/lang-default.scpp) prints `lang-default 7 5`.

A function may take extra arguments after a named parameter. The `...` is the end of the parameter list. [`docs/examples/lang-varargs.scpp`](examples/lang-varargs.scpp) prints `lang-varargs 1`. The macros that read those arguments are in [`docs/RUNTIME.md`](RUNTIME.md).

`inline` asks the compiler to consider placing the body at the call. [`docs/examples/lang-inline.scpp`](examples/lang-inline.scpp) prints `lang-inline 6`.

## Aggregates

A `struct` groups named members. A `union` stores one of its members at a time. An `enum` names integer constants. A member may be a bit-field, written `type name : width`. [`docs/examples/lang-struct.scpp`](examples/lang-struct.scpp) prints `lang-struct 7`. [`docs/examples/lang-union.scpp`](examples/lang-union.scpp) prints `lang-union 65`. [`docs/examples/lang-enum.scpp`](examples/lang-enum.scpp) prints `lang-enum 4`. [`docs/examples/lang-bitfield.scpp`](examples/lang-bitfield.scpp) prints `lang-bitfield 5`.

A brace list after `=` initializes an aggregate. A designator names the member, as in `{ .y = 2, .x = 1 }`. [`docs/examples/lang-brace.scpp`](examples/lang-brace.scpp) prints `lang-brace 2 3`. [`docs/examples/lang-designated.scpp`](examples/lang-designated.scpp) prints `lang-designated 1 2`.

## Storage class

`static` on a function limits that name to the file. [`docs/examples/lang-static.scpp`](examples/lang-static.scpp) prints `lang-static 5`.

`extern` on a function says the definition is elsewhere. A later definition in the same file supplies the body. [`docs/examples/lang-extern.scpp`](examples/lang-extern.scpp) prints `lang-extern 3`.

`register` is accepted as a storage class on a local. [`docs/examples/lang-register.scpp`](examples/lang-register.scpp) prints `lang-register 4`.

## Namespaces

`namespace name { ... }` places declarations in that name. A use outside the namespace is `name::member`. `using name::member` brings one name into the current scope. `using namespace name` brings the namespace's names in. [`docs/examples/lang-namespace.scpp`](examples/lang-namespace.scpp) prints `lang-namespace 9`. [`docs/examples/lang-using.scpp`](examples/lang-using.scpp) prints `lang-using 2`. [`docs/examples/lang-using-namespace.scpp`](examples/lang-using-namespace.scpp) prints `lang-using-namespace 3`.

## Classes

A class is written `class Name { members }`. The default access is `private`. `public` and `protected` open a section. Methods are declared in the class and defined as `Class::name` outside the body. A field is read through `this` inside a method. [`docs/examples/lang-class.scpp`](examples/lang-class.scpp) prints `lang-class 6`. [`docs/examples/lang-this.scpp`](examples/lang-this.scpp) prints `lang-this 4`.

A method may be `const`, written after the parameter list. [`docs/examples/lang-const-method.scpp`](examples/lang-const-method.scpp) prints `lang-const-method 8`.

A constructor is a method whose name is the class name and that has no return type. It may have an initializer list: `Class::Class() : member(expr), Base() { }`. A destructor is `~Class` and may take parameters. A direct call is `pointer->~Class()`. [`docs/examples/lang-init-list.scpp`](examples/lang-init-list.scpp) prints `lang-init-list 7`. [`docs/examples/lang-dtor.scpp`](examples/lang-dtor.scpp) prints `lang-dtor 1`.

A class may list bases. `public`, `protected`, and `private` name the access of each base. A derived constructor that uses a base field calls that base constructor in the initializer list. A protected field is visible to the derived class. [`docs/examples/lang-bases.scpp`](examples/lang-bases.scpp) prints `lang-bases 6`. [`docs/examples/lang-protected.scpp`](examples/lang-protected.scpp) prints `lang-protected 4`.

`virtual` on a method selects the override through the object's vtable. A call through a base pointer reaches the derived method. `= 0` makes the method pure. The derived class implements it, and a call through the base pointer runs that implementation. A virtual destructor runs the derived destructor when `delete` is applied to a base pointer. A virtual base is shared when two paths reach it. [`docs/examples/lang-virtual.scpp`](examples/lang-virtual.scpp) prints `lang-virtual 4`. [`docs/examples/lang-pure.scpp`](examples/lang-pure.scpp) prints `lang-pure 8`. [`docs/examples/lang-virtual-dtor.scpp`](examples/lang-virtual-dtor.scpp) prints `lang-virtual-dtor 1`. [`docs/examples/lang-virtual-base.scpp`](examples/lang-virtual-base.scpp) prints `lang-virtual-base 5`.

A hidden base method is called as `Base::method()`.

## Allocation

`new Type` and `new Type()` allocate an object and return a pointer. `new Type[n]` allocates an array. The collector reclaims memory. `delete` type-checks the pointer and calls the destructor. It does not deallocate. [`docs/examples/lang-new.scpp`](examples/lang-new.scpp) prints `lang-new 5 3`. [`docs/examples/lang-delete.scpp`](examples/lang-delete.scpp) prints `lang-delete 1`.

## Exceptions and type identity

`throw` raises an exception object. `catch (Exception error)` handles that type. `catch (...)` handles every type. A bare `throw` inside a handler raises the same exception again. Include the base class as `<exceptions>`. [`docs/examples/lang-throw.scpp`](examples/lang-throw.scpp) prints `lang-throw missing`. [`docs/examples/lang-catch-all.scpp`](examples/lang-catch-all.scpp) prints `lang-catch-all 6`. [`docs/examples/lang-rethrow.scpp`](examples/lang-rethrow.scpp) prints `lang-rethrow 4`.

`typeid(expression)` and `typeid(type)` yield a `const TypeInfo *`. Include it as `<typeinfo>`. `==` and `!=` compare those pointers. One type has one `TypeInfo`. A polymorphic object seen through a base pointer uses the dynamic type. [`docs/examples/lang-typeid.scpp`](examples/lang-typeid.scpp) prints `lang-typeid 1 1 1`.

## Array

`<array>` is a growable array of bytes. `stride` is the element size. `count` is how many elements are in use. `room` is how many `bytes` can hold. `reserve` grows the buffer. `address` returns a pointer to the element at an index. A negative index counts from the end. `-1` is the last element. An index past either end throws `Exception` with message `index`. `size` is `count`. Allocate with `new`. Write the bytes, then set `count`, then call `address`. [`docs/examples/ds-array.scpp`](examples/ds-array.scpp) prints `array 2 h i`.

## String

`<string>` is a growable array of `char`. Allocate with `new String()`. `add` copies a character or a C string onto the end. `append` and `prepend` do the same with a character, a C string, or a `String *`. `insert` places text at an index. A negative index counts from the end. An index past the end appends, and an index before the start prepends. `capacity` is the allocated size, including the trailing null. `size` is the character count, from `Array`. `at` reads one character. `c_str` and `chars` are the buffer. `String(const String *)` copies the characters. [`docs/examples/ds-string-build.scpp`](examples/ds-string-build.scpp) prints `string-build ZxPQabcdeZ ZxPQabcdeZ 10 Z 1`.

`strip()` removes spaces and line endings from both ends. `strip(char)` removes every copy of that character. Both return how many characters were removed. `find` returns the index where a character, a C string, or a `String *` begins, or `-1`. A start index may be passed, and a negative start counts from the end. `rfind` searches toward the front. `cmp` returns what `strcmp` would return, for any mix of `String *` and `const char *`. `replace` changes the first match of a character, a C string, or a `String *`. `truncate` removes a half-open range. [`docs/examples/ds-string-find.scpp`](examples/ds-string-find.scpp) prints `string-find 3 1 1 3 0 Ad`.

`split(const char *)` returns a new `StringVec`. Each piece is a new `String *`. `split()` separates on space, newline, and return. `join` returns a new `String *` and leaves the receiver as it was. `join(parts, separator)` places that separator between pieces. `join(parts)` places one space. `token` walks with a caller-owned `long` cursor that starts at 0. The separator is one character or a set of characters. The pointer is valid until the next call that changes the string. [`docs/examples/ds-string-split.scpp`](examples/ds-string-split.scpp) prints `string-split 3 a-b-c a b a`.

The same method name accepts a `char`, a `const char *`, or a `String *` wherever the header lists those overloads. A null C string or `String *` throws `Exception` with message `string`. An empty C string is valid.

## Vector

`<vector>` defines `VECTOR(Name, Type)`. `Name` is the class. `Type` is copied by assignment. Expand the macro in one translation unit and allocate with `new`.

```
VECTOR(IntVec, int)
IntVec *values;
values = new IntVec();
```

`add` appends. `at` reads an index. `insert` places a value at an index. A negative index counts from the end. An index past the end appends, and an index before the start prepends. `replace` writes the value at the index. `remove` deletes that index. `find` returns the first `==` match, or `-1`. `next` returns the value at a caller-owned `long` cursor and writes the following index. The cursor starts at 0. `size` is the count. [`docs/examples/ds-vector.scpp`](examples/ds-vector.scpp) prints `vector 0 10 20 40 2`.

## List

`<list>` defines `LIST(Name, Type)`. The shape matches `VECTOR`: `add`, `at`, `insert`, `replace`, `remove`, `find`, `next`, and `size`. `from_end` takes a non-negative index and counts from the last element. The storage is a doubly linked list. [`docs/examples/ds-list.scpp`](examples/ds-list.scpp) prints `list 3 -1 8 9 2`.

## Tree

`<tree>` defines `TREE(Name, Type)`. `add` places by `compare`. `insert` places at an in-order index. `replace` writes that in-order slot and leaves it there. `at`, `find`, `remove`, `next`, and `size` follow the vector rules, and `find` is the in-order index of the value. `compare(Type, Type)` returns negative, zero, or positive. The default uses `<`, or `String.cmp` when the element type is `const char *` or `String *`. A subclass overrides `compare` and calls the base constructor. [`docs/examples/ds-tree.scpp`](examples/ds-tree.scpp) prints `tree 9 9 3 3 3`.

## Hash

`<hash>` defines `HASH(Name, Key, Value)`. `put` inserts or replaces. `replace` does the same. `remove` deletes a key and leaves a missing key as it was. `find(key, Value *)` returns 1 and writes `*out`, or returns 0. `next(cursor, key, value)` walks filled slots from a cursor that starts at 0. It returns 1 and writes the pair, or 0 when the walk is finished. `compare(Key, Key)` returns zero when the keys are equal. The default uses `==`, or `String.cmp` for `const char *` and `String *`. A subclass overrides `compare`. [`docs/examples/ds-hash.scpp`](examples/ds-hash.scpp) prints `hash 12 1 1 12`. [`docs/examples/ds-hash-equal.scpp`](examples/ds-hash-equal.scpp) prints `hash-equal 1 30 1`.
