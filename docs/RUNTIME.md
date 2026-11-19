# The runtime library

These headers declare the functions a program calls. Include each one as `<name>`. A call with no declaration is still an external `int` function. The headers give the real C types. `<sqlite>` and `<gtk>` have their own pages. Group the examples by the header that declares the function.

A running example prints one line. CTest checks that line and an exit status of 0. `abort` stops the process, so its example is compiled with `-c` and is not run. Link `<math>` with `-l m` and `<pthread>` with `-l pthread`.

## exceptions

`Exception` is the base exception class. Include it as `<exceptions>`. `message` is a `const char *` and is not copied. `Exception()` stores a null message. `Exception(const char *)` stores that C string. A program constructs it with `new` and throws the object. `catch (Exception error)` reads `error.message`. An exception that nothing catches aborts and prints the message. [`docs/examples/rt-exception.scpp`](examples/rt-exception.scpp) prints `rt-exception 1 1`.

`throw`, `catch`, and `catch (...)` are language forms. [`docs/examples/lang-throw.scpp`](examples/lang-throw.scpp) prints `lang-throw missing`. [`docs/examples/lang-catch-all.scpp`](examples/lang-catch-all.scpp) prints `lang-catch-all 6`. [`docs/examples/lang-rethrow.scpp`](examples/lang-rethrow.scpp) prints `lang-rethrow 4`.

## typeinfo

`TypeInfo` holds the identity of one type. The public field is `name`. `typeid` yields a `const TypeInfo *`. `==` and `!=` compare those pointers. Include the header as `<typeinfo>`. [`docs/examples/lang-typeid.scpp`](examples/lang-typeid.scpp) prints `lang-typeid 1 1 1`.

## errno

`errno` is the calling thread's error number. A failed call sets it. A successful call leaves it as it was. The macro reads `*__errno_location()`. The named numbers in the header include `EPERM`, `EINTR`, `EAGAIN`, `EBUSY`, `EDOM`, `ERANGE`, `EDEADLK`, and `ENOSYS`. [`docs/examples/rt-errno.scpp`](examples/rt-errno.scpp) prints `rt-errno 1`.

`__errno_location` returns the address of that number. [`docs/examples/rt-errno-location.scpp`](examples/rt-errno-location.scpp) prints `rt-errno-location 1`.

`perror` writes the prefix, a colon, and the text for the current `errno` to standard error. [`docs/examples/rt-perror.scpp`](examples/rt-perror.scpp) prints `rt-perror 1`.

`strerror` returns the text for an error number. A later call may reuse that buffer. [`docs/examples/rt-strerror.scpp`](examples/rt-strerror.scpp) prints `rt-strerror 1`.

## stdlib

Include it as `<stdlib>`. `exit` ends the process and does not run destructors. `abort` stops the process immediately. [`docs/examples/rt-exit.scpp`](examples/rt-exit.scpp) prints `rt-exit 1`. [`docs/examples/rt-abort.scpp`](examples/rt-abort.scpp) compiles with `-c`.

`getenv` reads an environment value. `setenv` writes one, and the third argument replaces an existing value when it is nonzero. `unsetenv` removes a name. [`docs/examples/rt-getenv.scpp`](examples/rt-getenv.scpp) prints `rt-getenv 1`. [`docs/examples/rt-setenv.scpp`](examples/rt-setenv.scpp) prints `rt-setenv 1`. [`docs/examples/rt-unsetenv.scpp`](examples/rt-unsetenv.scpp) prints `rt-unsetenv 1`.

`atoi` and `atol` parse a leading integer. `strtol` parses in the given base and writes the first unparsed character through `end` when `end` is not null. `strtod` parses a floating value the same way. [`docs/examples/rt-atoi.scpp`](examples/rt-atoi.scpp) prints `rt-atoi 1`. [`docs/examples/rt-atol.scpp`](examples/rt-atol.scpp) prints `rt-atol 1`. [`docs/examples/rt-strtol.scpp`](examples/rt-strtol.scpp) prints `rt-strtol 1`. [`docs/examples/rt-strtod.scpp`](examples/rt-strtod.scpp) prints `rt-strtod 1`.

`malloc` returns a block the collector does not trace. `realloc` grows or shrinks that block. `free` releases it. [`docs/examples/rt-malloc.scpp`](examples/rt-malloc.scpp) prints `rt-malloc 1`. [`docs/examples/rt-realloc.scpp`](examples/rt-realloc.scpp) prints `rt-realloc 1`. [`docs/examples/rt-free.scpp`](examples/rt-free.scpp) prints `rt-free 1`.

`realpath` writes the absolute path of an existing file. `mkstemp` replaces `XXXXXX` at the end of a writable pattern, creates the file, and returns the descriptor. `mkdtemp` does the same for a directory and returns the path. [`docs/examples/rt-realpath.scpp`](examples/rt-realpath.scpp) prints `rt-realpath 1`. [`docs/examples/rt-mkstemp.scpp`](examples/rt-mkstemp.scpp) prints `rt-mkstemp 1`. [`docs/examples/rt-mkdtemp.scpp`](examples/rt-mkdtemp.scpp) prints `rt-mkdtemp 1`.

## stdio

Include it as `<stdio>`. `File` is the stream type. `fopen` opens a path. `fclose` closes the stream. The collector does not close it. [`docs/examples/rt-fopen.scpp`](examples/rt-fopen.scpp) prints `rt-fopen 1`. [`docs/examples/rt-fclose.scpp`](examples/rt-fclose.scpp) prints `rt-fclose 1`.

`fwrite` writes `count` items of `size` bytes. `fread` reads them. `fgets` reads one line into a buffer. `fputs` writes a C string. [`docs/examples/rt-fwrite.scpp`](examples/rt-fwrite.scpp) prints `rt-fwrite 1`. [`docs/examples/rt-fread.scpp`](examples/rt-fread.scpp) prints `rt-fread 1`. [`docs/examples/rt-fgets.scpp`](examples/rt-fgets.scpp) prints `rt-fgets 1`. [`docs/examples/rt-fputs.scpp`](examples/rt-fputs.scpp) prints `rt-fputs 1`.

The format parameter is named `text` because `format` is a keyword. `fprintf` writes a formatted line to a stream. `printf` writes it to standard output. `snprintf` writes it into a buffer and keeps the result inside `size` bytes. [`docs/examples/rt-fprintf.scpp`](examples/rt-fprintf.scpp) prints `rt-fprintf 1`. [`docs/examples/rt-printf.scpp`](examples/rt-printf.scpp) prints `rt-printf 1`. [`docs/examples/rt-snprintf.scpp`](examples/rt-snprintf.scpp) prints `rt-snprintf 1`.

`fflush` pushes a stream's buffer. `fseek` moves by `SEEK_SET`, `SEEK_CUR`, or `SEEK_END`. `ftell` returns the position. `rewind` moves to the start. [`docs/examples/rt-fflush.scpp`](examples/rt-fflush.scpp) prints `rt-fflush 1`. [`docs/examples/rt-fseek.scpp`](examples/rt-fseek.scpp) prints `rt-fseek 1`. [`docs/examples/rt-ftell.scpp`](examples/rt-ftell.scpp) prints `rt-ftell 1`. [`docs/examples/rt-rewind.scpp`](examples/rt-rewind.scpp) prints `rt-rewind 1`.

`feof` is nonzero after a read hits the end. `ferror` is nonzero after an error. `clearerr` clears both. [`docs/examples/rt-feof.scpp`](examples/rt-feof.scpp) prints `rt-feof 1`. [`docs/examples/rt-ferror.scpp`](examples/rt-ferror.scpp) prints `rt-ferror 1`. [`docs/examples/rt-clearerr.scpp`](examples/rt-clearerr.scpp) prints `rt-clearerr 1`.

`remove` deletes a file. `rename` changes a path. `getline` reads a line and grows the buffer with `malloc`. The caller frees that buffer. [`docs/examples/rt-remove.scpp`](examples/rt-remove.scpp) prints `rt-remove 1`. [`docs/examples/rt-rename.scpp`](examples/rt-rename.scpp) prints `rt-rename 1`. [`docs/examples/rt-getline.scpp`](examples/rt-getline.scpp) prints `rt-getline 1`.

`stdin`, `stdout`, and `stderr` are the standard streams. [`docs/examples/rt-stdin.scpp`](examples/rt-stdin.scpp) prints `rt-stdin 1`. [`docs/examples/rt-stdout.scpp`](examples/rt-stdout.scpp) prints `rt-stdout 1`. [`docs/examples/rt-stderr.scpp`](examples/rt-stderr.scpp) prints `rt-stderr 1`.

## stat

Include it as `<stat>`. `stat` fills a `struct stat` for a path. `lstat` does the same and does not follow a symbolic link. `S_ISDIR` and `S_ISREG` read `st_mode`. `mkdir` creates a directory with the given mode. [`docs/examples/rt-stat.scpp`](examples/rt-stat.scpp) prints `rt-stat 1`. [`docs/examples/rt-lstat.scpp`](examples/rt-lstat.scpp) prints `rt-lstat 1`. [`docs/examples/rt-mkdir.scpp`](examples/rt-mkdir.scpp) prints `rt-mkdir 1`.

## unistd

Include it as `<unistd>`. A return of 0 is success and -1 is failure, except `read`, `write`, `lseek`, and `copy_file_range`, which return a count or -1.

`unlink` deletes a file. `rmdir` deletes an empty directory. `chdir` changes the working directory. `getcwd` writes that directory into a buffer. `access` tests `F_OK`, `R_OK`, `W_OK`, or `X_OK`. [`docs/examples/rt-unlink.scpp`](examples/rt-unlink.scpp) prints `rt-unlink 1`. [`docs/examples/rt-rmdir.scpp`](examples/rt-rmdir.scpp) prints `rt-rmdir 1`. [`docs/examples/rt-chdir.scpp`](examples/rt-chdir.scpp) prints `rt-chdir 1`. [`docs/examples/rt-getcwd.scpp`](examples/rt-getcwd.scpp) prints `rt-getcwd 1`. [`docs/examples/rt-access.scpp`](examples/rt-access.scpp) prints `rt-access 1`.

`read` and `write` move bytes on a descriptor. `close` releases it. `lseek` moves the position. `copy_file_range` copies bytes from one descriptor to another. A null offset pointer uses the descriptor's position. [`docs/examples/rt-read.scpp`](examples/rt-read.scpp) prints `rt-read 1`. [`docs/examples/rt-write.scpp`](examples/rt-write.scpp) prints `rt-write 1`. [`docs/examples/rt-close.scpp`](examples/rt-close.scpp) prints `rt-close 1`. [`docs/examples/rt-lseek.scpp`](examples/rt-lseek.scpp) prints `rt-lseek 1`. [`docs/examples/rt-copy-file-range.scpp`](examples/rt-copy-file-range.scpp) prints `rt-copy-file-range 1`.

`pipe` writes two descriptors. The first reads and the second writes. `isatty` returns 1 when the descriptor is a terminal. [`docs/examples/rt-pipe.scpp`](examples/rt-pipe.scpp) prints `rt-pipe 1`. [`docs/examples/rt-isatty.scpp`](examples/rt-isatty.scpp) prints `rt-isatty 1`.

## dirent

Include it as `<dirent>`. `opendir` opens a directory. `readdir` returns the next `struct dirent`, whose `d_name` is the entry name, or null at the end. `closedir` closes the directory. [`docs/examples/rt-opendir.scpp`](examples/rt-opendir.scpp) prints `rt-opendir 1`. [`docs/examples/rt-readdir.scpp`](examples/rt-readdir.scpp) prints `rt-readdir 1`. [`docs/examples/rt-closedir.scpp`](examples/rt-closedir.scpp) prints `rt-closedir 1`.

## fcntl

Include it as `<fcntl>`. `open` opens a path. The flags include `O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREAT`, `O_TRUNC`, and `O_APPEND`. The third argument is the mode when `O_CREAT` is set. [`docs/examples/rt-open.scpp`](examples/rt-open.scpp) prints `rt-open 1`.

## libgen

Include it as `<libgen>`. `basename` returns the last component of a path. `dirname` returns the directory. Both may write into the buffer the caller passes. [`docs/examples/rt-basename.scpp`](examples/rt-basename.scpp) prints `rt-basename 1`. [`docs/examples/rt-dirname.scpp`](examples/rt-dirname.scpp) prints `rt-dirname 1`.

## glob

Include it as `<glob>`. `glob` fills a `glob_t` with sorted matches of `*`, `?`, and `[...]`. `gl_pathc` is the count and `gl_pathv` is the paths. `GLOB_NOMATCH` means there were none. The error callback may be null. `globfree` releases the result. [`docs/examples/rt-glob.scpp`](examples/rt-glob.scpp) prints `rt-glob 1`. [`docs/examples/rt-globfree.scpp`](examples/rt-globfree.scpp) prints `rt-globfree 1`.

## math

Include it as `<math>` and link with `-l m`. Each double function has a float twin whose name ends in `f`. `labs` is the absolute value of a `long`.

`fabs` returns the absolute value of a double. [`docs/examples/rt-fabs.scpp`](examples/rt-fabs.scpp) prints `rt-fabs 1`.

`fmin` returns the smaller of two doubles. [`docs/examples/rt-fmin.scpp`](examples/rt-fmin.scpp) prints `rt-fmin 1`.

`fmax` returns the larger of two doubles. [`docs/examples/rt-fmax.scpp`](examples/rt-fmax.scpp) prints `rt-fmax 1`.

`floor` rounds a double down to an integer value. [`docs/examples/rt-floor.scpp`](examples/rt-floor.scpp) prints `rt-floor 1`.

`ceil` rounds a double up to an integer value. [`docs/examples/rt-ceil.scpp`](examples/rt-ceil.scpp) prints `rt-ceil 1`.

`round` rounds a double to the nearest integer value. [`docs/examples/rt-round.scpp`](examples/rt-round.scpp) prints `rt-round 1`.

`trunc` drops the fractional part of a double. [`docs/examples/rt-trunc.scpp`](examples/rt-trunc.scpp) prints `rt-trunc 1`.

`sqrt` returns the square root. [`docs/examples/rt-sqrt.scpp`](examples/rt-sqrt.scpp) prints `rt-sqrt 1`.

`cbrt` returns the cube root. [`docs/examples/rt-cbrt.scpp`](examples/rt-cbrt.scpp) prints `rt-cbrt 1`.

`pow` raises a base to an exponent. [`docs/examples/rt-pow.scpp`](examples/rt-pow.scpp) prints `rt-pow 1`.

`exp` returns e raised to the argument. [`docs/examples/rt-exp.scpp`](examples/rt-exp.scpp) prints `rt-exp 1`.

`log` returns the natural logarithm. [`docs/examples/rt-log.scpp`](examples/rt-log.scpp) prints `rt-log 1`.

`log10` returns the base-10 logarithm. [`docs/examples/rt-log10.scpp`](examples/rt-log10.scpp) prints `rt-log10 1`.

`log2` returns the base-2 logarithm. [`docs/examples/rt-log2.scpp`](examples/rt-log2.scpp) prints `rt-log2 1`.

`sin` returns the sine of radians. [`docs/examples/rt-sin.scpp`](examples/rt-sin.scpp) prints `rt-sin 1`.

`cos` returns the cosine of radians. [`docs/examples/rt-cos.scpp`](examples/rt-cos.scpp) prints `rt-cos 1`.

`tan` returns the tangent of radians. [`docs/examples/rt-tan.scpp`](examples/rt-tan.scpp) prints `rt-tan 1`.

`asin` returns the arc sine. [`docs/examples/rt-asin.scpp`](examples/rt-asin.scpp) prints `rt-asin 1`.

`acos` returns the arc cosine. [`docs/examples/rt-acos.scpp`](examples/rt-acos.scpp) prints `rt-acos 1`.

`atan` returns the arc tangent. [`docs/examples/rt-atan.scpp`](examples/rt-atan.scpp) prints `rt-atan 1`.

`atan2` returns the arc tangent of y/x, using the signs of both arguments. [`docs/examples/rt-atan2.scpp`](examples/rt-atan2.scpp) prints `rt-atan2 1`.

`sinh` returns the hyperbolic sine. [`docs/examples/rt-sinh.scpp`](examples/rt-sinh.scpp) prints `rt-sinh 1`.

`cosh` returns the hyperbolic cosine. [`docs/examples/rt-cosh.scpp`](examples/rt-cosh.scpp) prints `rt-cosh 1`.

`tanh` returns the hyperbolic tangent. [`docs/examples/rt-tanh.scpp`](examples/rt-tanh.scpp) prints `rt-tanh 1`.

`fabsf` is `fabs` for `float`. [`docs/examples/rt-fabsf.scpp`](examples/rt-fabsf.scpp) prints `rt-fabsf 1`.

`fminf` is `fmin` for `float`. [`docs/examples/rt-fminf.scpp`](examples/rt-fminf.scpp) prints `rt-fminf 1`.

`fmaxf` is `fmax` for `float`. [`docs/examples/rt-fmaxf.scpp`](examples/rt-fmaxf.scpp) prints `rt-fmaxf 1`.

`floorf` is `floor` for `float`. [`docs/examples/rt-floorf.scpp`](examples/rt-floorf.scpp) prints `rt-floorf 1`.

`ceilf` is `ceil` for `float`. [`docs/examples/rt-ceilf.scpp`](examples/rt-ceilf.scpp) prints `rt-ceilf 1`.

`roundf` is `round` for `float`. [`docs/examples/rt-roundf.scpp`](examples/rt-roundf.scpp) prints `rt-roundf 1`.

`truncf` is `trunc` for `float`. [`docs/examples/rt-truncf.scpp`](examples/rt-truncf.scpp) prints `rt-truncf 1`.

`sqrtf` is `sqrt` for `float`. [`docs/examples/rt-sqrtf.scpp`](examples/rt-sqrtf.scpp) prints `rt-sqrtf 1`.

`cbrtf` is `cbrt` for `float`. [`docs/examples/rt-cbrtf.scpp`](examples/rt-cbrtf.scpp) prints `rt-cbrtf 1`.

`powf` is `pow` for `float`. [`docs/examples/rt-powf.scpp`](examples/rt-powf.scpp) prints `rt-powf 1`.

`expf` is `exp` for `float`. [`docs/examples/rt-expf.scpp`](examples/rt-expf.scpp) prints `rt-expf 1`.

`logf` is `log` for `float`. [`docs/examples/rt-logf.scpp`](examples/rt-logf.scpp) prints `rt-logf 1`.

`log10f` is `log10` for `float`. [`docs/examples/rt-log10f.scpp`](examples/rt-log10f.scpp) prints `rt-log10f 1`.

`log2f` is `log2` for `float`. [`docs/examples/rt-log2f.scpp`](examples/rt-log2f.scpp) prints `rt-log2f 1`.

`sinf` is `sin` for `float`. [`docs/examples/rt-sinf.scpp`](examples/rt-sinf.scpp) prints `rt-sinf 1`.

`cosf` is `cos` for `float`. [`docs/examples/rt-cosf.scpp`](examples/rt-cosf.scpp) prints `rt-cosf 1`.

`tanf` is `tan` for `float`. [`docs/examples/rt-tanf.scpp`](examples/rt-tanf.scpp) prints `rt-tanf 1`.

`asinf` is `asin` for `float`. [`docs/examples/rt-asinf.scpp`](examples/rt-asinf.scpp) prints `rt-asinf 1`.

`acosf` is `acos` for `float`. [`docs/examples/rt-acosf.scpp`](examples/rt-acosf.scpp) prints `rt-acosf 1`.

`atanf` is `atan` for `float`. [`docs/examples/rt-atanf.scpp`](examples/rt-atanf.scpp) prints `rt-atanf 1`.

`atan2f` is `atan2` for `float`. [`docs/examples/rt-atan2f.scpp`](examples/rt-atan2f.scpp) prints `rt-atan2f 1`.

`sinhf` is `sinh` for `float`. [`docs/examples/rt-sinhf.scpp`](examples/rt-sinhf.scpp) prints `rt-sinhf 1`.

`coshf` is `cosh` for `float`. [`docs/examples/rt-coshf.scpp`](examples/rt-coshf.scpp) prints `rt-coshf 1`.

`tanhf` is `tanh` for `float`. [`docs/examples/rt-tanhf.scpp`](examples/rt-tanhf.scpp) prints `rt-tanhf 1`.

`labs` returns the absolute value of a `long`. [`docs/examples/rt-labs.scpp`](examples/rt-labs.scpp) prints `rt-labs 1`.

## socket

Include it as `<socket>`. The calls are IPv4 TCP. `AF_INET`, `SOCK_STREAM`, and `IPPROTO_TCP` select that family. `close`, from `<unistd>`, releases a socket.

`socket` creates a descriptor. `bind` attaches it to an address. `listen` marks it as accepting connections. `connect` connects to an address. `accept` takes one pending connection. `send` and `recv` move bytes. [`docs/examples/rt-socket.scpp`](examples/rt-socket.scpp) prints `rt-socket 1`. [`docs/examples/rt-bind.scpp`](examples/rt-bind.scpp) prints `rt-bind 1`. [`docs/examples/rt-listen.scpp`](examples/rt-listen.scpp) prints `rt-listen 1`. [`docs/examples/rt-connect.scpp`](examples/rt-connect.scpp) prints `rt-connect 1`. [`docs/examples/rt-accept.scpp`](examples/rt-accept.scpp) prints `rt-accept 1`. [`docs/examples/rt-send.scpp`](examples/rt-send.scpp) prints `rt-send 1`. [`docs/examples/rt-recv.scpp`](examples/rt-recv.scpp) prints `rt-recv 1`.

`setsockopt` sets an option. Level 1 is the socket level, and `SO_RCVTIMEO` takes a `struct timeval`. [`docs/examples/rt-setsockopt.scpp`](examples/rt-setsockopt.scpp) prints `rt-setsockopt 1`.

`getaddrinfo` resolves a host and service into a list of `struct addrinfo`. `freeaddrinfo` releases that list. A nonzero result is not `errno`. `gai_strerror` describes it. [`docs/examples/rt-getaddrinfo.scpp`](examples/rt-getaddrinfo.scpp) prints `rt-getaddrinfo 1`. [`docs/examples/rt-freeaddrinfo.scpp`](examples/rt-freeaddrinfo.scpp) prints `rt-freeaddrinfo 1`. [`docs/examples/rt-gai-strerror.scpp`](examples/rt-gai-strerror.scpp) prints `rt-gai-strerror 1`.

`htons` and `htonl` convert a short and an int to network byte order. `inet_pton` parses an address into a `sin_addr` and returns 1 on success. [`docs/examples/rt-htons.scpp`](examples/rt-htons.scpp) prints `rt-htons 1`. [`docs/examples/rt-htonl.scpp`](examples/rt-htonl.scpp) prints `rt-htonl 1`. [`docs/examples/rt-inet-pton.scpp`](examples/rt-inet-pton.scpp) prints `rt-inet-pton 1`.

## time

Include it as `<time>`. `time` returns Unix time and writes it through `out` when `out` is not null. `clock_gettime` fills a `struct timespec`. `CLOCK_REALTIME` is Unix time and `CLOCK_MONOTONIC` is an interval clock. `nanosleep` waits for the requested time. A signal that wakes it early returns -1 and sets `errno` to `EINTR`. [`docs/examples/rt-time.scpp`](examples/rt-time.scpp) prints `rt-time 1`. [`docs/examples/rt-clock.scpp`](examples/rt-clock.scpp) prints `rt-clock 1`. [`docs/examples/rt-nanosleep.scpp`](examples/rt-nanosleep.scpp) prints `rt-nanosleep 1`.

## spawn

Include it as `<spawn>`. `posix_spawnp` starts a program. A name with no slash is searched in `PATH`. The result is 0 or an error number, and `errno` is left unchanged. `waitpid` reaps the child. `WIFEXITED` and `WEXITSTATUS` read the status. [`docs/examples/rt-posix-spawnp.scpp`](examples/rt-posix-spawnp.scpp) prints `rt-posix-spawnp 1`. [`docs/examples/rt-waitpid.scpp`](examples/rt-waitpid.scpp) prints `rt-waitpid 1`.

`posix_spawn_file_actions_init` prepares a file-action list. `posix_spawn_file_actions_destroy` releases it. `posix_spawn_file_actions_adddup2` duplicates a descriptor in the child. `posix_spawn_file_actions_addclose` closes one. [`docs/examples/rt-spawn-actions-init.scpp`](examples/rt-spawn-actions-init.scpp) prints `rt-spawn-actions-init 1`. [`docs/examples/rt-spawn-actions-destroy.scpp`](examples/rt-spawn-actions-destroy.scpp) prints `rt-spawn-actions-destroy 1`. [`docs/examples/rt-spawn-adddup2.scpp`](examples/rt-spawn-adddup2.scpp) prints `rt-spawn-adddup2 1`. [`docs/examples/rt-spawn-addclose.scpp`](examples/rt-spawn-addclose.scpp) prints `rt-spawn-addclose 1`.

`posix_spawnattr_init` prepares spawn attributes. `posix_spawnattr_destroy` releases them. `posix_spawnattr_setflags` sets flags such as `POSIX_SPAWN_SETPGROUP`. `posix_spawnattr_setpgroup` sets the child's process group. [`docs/examples/rt-spawnattr-init.scpp`](examples/rt-spawnattr-init.scpp) prints `rt-spawnattr-init 1`. [`docs/examples/rt-spawnattr-destroy.scpp`](examples/rt-spawnattr-destroy.scpp) prints `rt-spawnattr-destroy 1`. [`docs/examples/rt-spawnattr-setflags.scpp`](examples/rt-spawnattr-setflags.scpp) prints `rt-spawnattr-setflags 1`. [`docs/examples/rt-spawnattr-setpgroup.scpp`](examples/rt-spawnattr-setpgroup.scpp) prints `rt-spawnattr-setpgroup 1`.

## signal

Include it as `<signal>`. `kill` sends a signal to a process. Signal 0 tests that the process exists. `sigaction` installs a handler. `SIG_IGN` ignores the signal. `SIGPIPE` is the signal a write to a dead pipe would raise. [`docs/examples/rt-kill.scpp`](examples/rt-kill.scpp) prints `rt-kill 1`. [`docs/examples/rt-sigaction.scpp`](examples/rt-sigaction.scpp) prints `rt-sigaction 1`.

## ioctl

Include it as `<ioctl>`. `ioctl` performs a device request. `TIOCGWINSZ` asks a terminal for its window size and fails on a pipe. [`docs/examples/rt-ioctl.scpp`](examples/rt-ioctl.scpp) prints `rt-ioctl 1`.

## random

Include it as `<random>`. `getrandom` writes random bytes and returns the count, or -1 and sets `errno`. `GRND_NONBLOCK` returns `EAGAIN` instead of waiting. A short count means the caller asks again. [`docs/examples/rt-getrandom.scpp`](examples/rt-getrandom.scpp) prints `rt-getrandom 1`.

## pthread

Include it as `<pthread>` and link with `-l pthread`. Thread calls return 0 or an error number. They do not set `errno`. `sem_wait` and `sem_post` return 0 or -1.

`pthread_create` starts a thread. The start function takes one `void *` and returns `void *`. `pthread_join` waits for that thread and may write its return through a `void **`. `pthread_detach` lets the thread release its resources when it returns, so the caller does not join it. `pthread_self` returns the calling thread. [`docs/examples/rt-pthread-create.scpp`](examples/rt-pthread-create.scpp) prints `rt-pthread-create 1`. [`docs/examples/rt-pthread-join.scpp`](examples/rt-pthread-join.scpp) prints `rt-pthread-join 1`. [`docs/examples/rt-pthread-detach.scpp`](examples/rt-pthread-detach.scpp) prints `rt-pthread-detach 1`. [`docs/examples/rt-pthread-self.scpp`](examples/rt-pthread-self.scpp) prints `rt-pthread-self 1`.

`pthread_mutex_init` prepares a mutex. A null attribute uses the default. `pthread_mutex_destroy` releases it. `pthread_mutex_lock` takes it. `pthread_mutex_unlock` releases it. [`docs/examples/rt-mutex-init.scpp`](examples/rt-mutex-init.scpp) prints `rt-mutex-init 1`. [`docs/examples/rt-mutex-destroy.scpp`](examples/rt-mutex-destroy.scpp) prints `rt-mutex-destroy 1`. [`docs/examples/rt-mutex-lock.scpp`](examples/rt-mutex-lock.scpp) prints `rt-mutex-lock 1`. [`docs/examples/rt-mutex-unlock.scpp`](examples/rt-mutex-unlock.scpp) prints `rt-mutex-unlock 1`.

`pthread_cond_init` prepares a condition. `pthread_cond_destroy` releases it. `pthread_cond_wait` waits while holding the mutex, and it reacquires the mutex before returning. `pthread_cond_signal` wakes one waiter. `pthread_cond_broadcast` wakes every waiter. [`docs/examples/rt-cond-init.scpp`](examples/rt-cond-init.scpp) prints `rt-cond-init 1`. [`docs/examples/rt-cond-destroy.scpp`](examples/rt-cond-destroy.scpp) prints `rt-cond-destroy 1`. [`docs/examples/rt-cond-wait.scpp`](examples/rt-cond-wait.scpp) prints `rt-cond-wait 1`. [`docs/examples/rt-cond-signal.scpp`](examples/rt-cond-signal.scpp) prints `rt-cond-signal 1`. [`docs/examples/rt-cond-broadcast.scpp`](examples/rt-cond-broadcast.scpp) prints `rt-cond-broadcast 1`.

`sem_init` prepares an unnamed semaphore. The shared argument is 0 for a semaphore used inside one process. `sem_destroy` releases it. `sem_post` adds one. `sem_wait` subtracts one and waits when the value is zero. [`docs/examples/rt-sem-init.scpp`](examples/rt-sem-init.scpp) prints `rt-sem-init 1`. [`docs/examples/rt-sem-destroy.scpp`](examples/rt-sem-destroy.scpp) prints `rt-sem-destroy 1`. [`docs/examples/rt-sem-post.scpp`](examples/rt-sem-post.scpp) prints `rt-sem-post 1`. [`docs/examples/rt-sem-wait.scpp`](examples/rt-sem-wait.scpp) prints `rt-sem-wait 1`.

## stdarg

Include it as `<stdarg>`. A variadic function names the extra arguments with `...`. `va_list` holds the position. `va_start` names the last fixed parameter. `va_arg` returns the next argument as the type named. `va_copy` copies a list, and the two lists then walk independently. `va_end` finishes a list and is safe on a copy. The four macros are used together. [`docs/examples/rt-stdarg.scpp`](examples/rt-stdarg.scpp) prints `rt-stdarg 1`.

## cli

Include it as `<cli>`. The classes are `Option`, `Positional`, and `CommandLine`, in namespace `Cli`. `parse` copies the argument text it keeps. A usage error throws `Exception` with message `usage`. `CLI_HELP` and `CLI_VERSION` stop before the required checks.

`Option()` starts a flag. `value_type` is `CLI_FLAG`, `CLI_COUNTER`, `CLI_STRING`, or `CLI_NUMBER`. `is_list` splits a value on `list_separator`, which defaults to `:`. `action` is `CLI_NONE`, `CLI_HELP`, or `CLI_VERSION`. `short_name`, `long_name`, and `name` are the spellings `parse` recognizes and the key later queries use. [`docs/examples/rt-cli-option.scpp`](examples/rt-cli-option.scpp) prints `rt-cli-option 1`.

`Positional()` starts a positional argument. `required` defaults to false. `is_list` gathers the remaining positionals into one slot. [`docs/examples/rt-cli-positional.scpp`](examples/rt-cli-positional.scpp) prints `rt-cli-positional 1`.

`CommandLine(program, summary, version)` builds the parser. `help_text` starts with `Usage:` and names the program. `version_text` is `program: vversion`. [`docs/examples/rt-cli-command.scpp`](examples/rt-cli-command.scpp) prints `rt-cli-command 1`. [`docs/examples/rt-cli-help.scpp`](examples/rt-cli-help.scpp) prints `rt-cli-help 1`. [`docs/examples/rt-cli-version.scpp`](examples/rt-cli-version.scpp) prints `rt-cli-version 1`.

`set_capture_remainder` keeps the arguments after `--`. [`docs/examples/rt-cli-capture.scpp`](examples/rt-cli-capture.scpp) prints `rt-cli-capture 1`.

`add_option` registers an option. `add_positional` registers a positional. `parse` reads `argc` and `argv`, including `argv[0]`. [`docs/examples/rt-cli-add-option.scpp`](examples/rt-cli-add-option.scpp) prints `rt-cli-add-option 1`. [`docs/examples/rt-cli-add-positional.scpp`](examples/rt-cli-add-positional.scpp) prints `rt-cli-add-positional 1`. [`docs/examples/rt-cli-parse.scpp`](examples/rt-cli-parse.scpp) prints `rt-cli-parse 1`.

`was_provided` is true when that option was written. `switch_value` is true for a flag that was written. `count` is how many times a `CLI_COUNTER` was written, so `-vv` is 2. [`docs/examples/rt-cli-was-provided.scpp`](examples/rt-cli-was-provided.scpp) prints `rt-cli-was-provided 1`. [`docs/examples/rt-cli-switch.scpp`](examples/rt-cli-switch.scpp) prints `rt-cli-switch 1`. [`docs/examples/rt-cli-count.scpp`](examples/rt-cli-count.scpp) prints `rt-cli-count 1`.

`has_value` is true when a string or number option received a value. `value` returns that text. `value_count` is the number of list items. `value_at` returns one item. `contains_item` tests one item. [`docs/examples/rt-cli-has-value.scpp`](examples/rt-cli-has-value.scpp) prints `rt-cli-has-value 1`. [`docs/examples/rt-cli-value.scpp`](examples/rt-cli-value.scpp) prints `rt-cli-value 1`. [`docs/examples/rt-cli-value-count.scpp`](examples/rt-cli-value-count.scpp) prints `rt-cli-value-count 1`. [`docs/examples/rt-cli-value-at.scpp`](examples/rt-cli-value-at.scpp) prints `rt-cli-value-at 1`. [`docs/examples/rt-cli-contains.scpp`](examples/rt-cli-contains.scpp) prints `rt-cli-contains 1`.

`positional_count` and `positional_at` read the positionals. `remainder_count` and `remainder_at` read the arguments after `--`. `current_action` is `CLI_NONE`, `CLI_HELP`, or `CLI_VERSION` after `parse`. [`docs/examples/rt-cli-positional-count.scpp`](examples/rt-cli-positional-count.scpp) prints `rt-cli-positional-count 1`. [`docs/examples/rt-cli-positional-at.scpp`](examples/rt-cli-positional-at.scpp) prints `rt-cli-positional-at 1`. [`docs/examples/rt-cli-remainder-count.scpp`](examples/rt-cli-remainder-count.scpp) prints `rt-cli-remainder-count 1`. [`docs/examples/rt-cli-remainder-at.scpp`](examples/rt-cli-remainder-at.scpp) prints `rt-cli-remainder-at 1`. [`docs/examples/rt-cli-action.scpp`](examples/rt-cli-action.scpp) prints `rt-cli-action 1`.
