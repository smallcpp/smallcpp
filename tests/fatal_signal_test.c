/*
 * Raise SIGSEGV and check that stderr shows a stack and the process
 * dies with that signal.
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void smallcpp_fatal_init(void);

static void crash(void)
{
    volatile int *pointer = 0;

    *pointer = 1;
}

int main(void)
{
    int fds[2];
    pid_t pid;
    char buf[8192];
    size_t got = 0;
    int status;
    ssize_t n;
    int bad = 0;

    smallcpp_fatal_init();
    if (pipe(fds) != 0)
        return 1;
    pid = fork();
    if (pid < 0)
        return 1;
    if (pid == 0) {
        close(fds[0]);
        if (dup2(fds[1], STDERR_FILENO) < 0)
            _exit(1);
        close(fds[1]);
        crash();
        _exit(1);
    }
    close(fds[1]);
    while (got + 1 < sizeof buf
           && (n = read(fds[0], buf + got, sizeof buf - 1 - got)) > 0)
        got += (size_t)n;
    buf[got] = '\0';
    close(fds[0]);
    if (waitpid(pid, &status, 0) < 0)
        return 1;
    if (!WIFSIGNALED(status) || WTERMSIG(status) != SIGSEGV)
        bad = 1;
    if (strstr(buf, "fatal signal SIGSEGV") == NULL)
        bad = 1;
    if (strstr(buf, "\n0x") == NULL)
        bad = 1;
    if (bad) {
        fprintf(stderr, "status %d\n%s\n", status, buf);
        return 1;
    }
    return 0;
}
