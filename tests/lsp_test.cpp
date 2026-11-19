#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

pid_t child_pid = -1;

void fail(const std::string &message)
{
    if (child_pid > 0)
        kill(child_pid, SIGKILL);
    throw std::runtime_error(message);
}

void require(bool condition, const std::string &message)
{
    if (!condition)
        fail(message);
}

void write_all(int fd, const std::string &bytes)
{
    const char *data = bytes.data();
    std::size_t left = bytes.size();

    while (left > 0) {
        ssize_t wrote = write(fd, data, left);
        if (wrote < 0) {
            if (errno == EINTR)
                continue;
            fail(std::string("write: ") + std::strerror(errno));
        }
        data += wrote;
        left -= static_cast<std::size_t>(wrote);
    }
}

void send_message(int fd, const std::string &body)
{
    const std::string header =
        "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
    write_all(fd, header);
    write_all(fd, body);
}

int wait_fd(int fd)
{
    pollfd ready;
    int result;

    ready.fd = fd;
    ready.events = POLLIN;
    ready.revents = 0;
    result = poll(&ready, 1, 60000);
    if (result == 0)
        fail("timed out waiting for the server");
    if (result < 0) {
        if (errno == EINTR)
            return wait_fd(fd);
        fail(std::string("poll: ") + std::strerror(errno));
    }
    return ready.revents;
}

std::string read_message(int fd)
{
    std::string header;
    char byte = 0;

    while (header.size() < 8192) {
        ssize_t got;

        wait_fd(fd);
        got = read(fd, &byte, 1);
        if (got < 0) {
            if (errno == EINTR)
                continue;
            fail(std::string("read: ") + std::strerror(errno));
        }
        if (got == 0)
            fail("server closed stdout\n" + header);
        header.push_back(byte);
        if (header.size() >= 4
                && header.compare(header.size() - 4, 4, "\r\n\r\n") == 0)
            break;
        if (header.size() >= 2
                && header.compare(header.size() - 2, 2, "\n\n") == 0)
            break;
    }
    const std::size_t mark = header.find("Content-Length:");
    require(mark != std::string::npos, "missing Content-Length\n" + header);
    const long length = std::stol(header.substr(mark + 15));
    require(length >= 0, "bad Content-Length\n" + header);
    std::string body;
    body.resize(static_cast<std::size_t>(length));
    std::size_t filled = 0;
    while (filled < body.size()) {
        ssize_t got;

        wait_fd(fd);
        got = read(fd, &body[filled], body.size() - filled);
        if (got < 0) {
            if (errno == EINTR)
                continue;
            fail(std::string("read: ") + std::strerror(errno));
        }
        if (got == 0)
            fail("short body\n" + body);
        filled += static_cast<std::size_t>(got);
    }
    return body;
}

void expect_has(const std::string &body, const std::string &needle)
{
    if (body.find(needle) == std::string::npos)
        fail("expected '" + needle + "' in\n" + body);
}

long range_line(const std::string &body)
{
    const std::string key = "\"start\":{\"line\":";
    const std::size_t at = body.find(key);
    require(at != std::string::npos, "missing start line\n" + body);
    return std::stol(body.substr(at + key.size()));
}

} // namespace

int main(int argc, char **argv)
{
    int to_server[2];
    int from_server[2];
    try {
        require(argc == 2, "usage: lsp_test /path/to/scpp-lsp");
        require(pipe(to_server) == 0, "pipe");
        require(pipe(from_server) == 0, "pipe");
        child_pid = fork();
        require(child_pid >= 0, "fork");
        if (child_pid == 0) {
            dup2(to_server[0], 0);
            dup2(from_server[1], 1);
            close(to_server[0]);
            close(to_server[1]);
            close(from_server[0]);
            close(from_server[1]);
            execl(argv[1], argv[1], (char *)nullptr);
            std::perror("execl");
            _exit(127);
        }
        close(to_server[0]);
        close(from_server[1]);
        alarm(90);

        send_message(to_server[1],
            "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\","
            "\"params\":{\"processId\":null,\"rootUri\":null,"
            "\"capabilities\":{}}}");
        const std::string hello = read_message(from_server[0]);
        expect_has(hello, "\"id\":1");
        expect_has(hello, "definitionProvider");
        expect_has(hello, "completionProvider");
        expect_has(hello, "\"change\":1");
        expect_has(hello, "scpp-lsp");

        send_message(to_server[1],
            "{\"jsonrpc\":\"2.0\",\"method\":\"initialized\","
            "\"params\":{}}");

        const std::string open =
            std::string("{\"jsonrpc\":\"2.0\",\"method\":"
                        "\"textDocument/didOpen\",\"params\":{"
                        "\"textDocument\":{\"uri\":"
                        "\"file:///tmp/lsp-unit.scpp\",\"languageId\":"
                        "\"scpp\",\"version\":1,\"text\":")
            + "\""
            + "int answer(void) {\\n"
              "    return missing;\\n"
              "}\\n"
              "\\n"
              "int main(void) {\\n"
              "    return answer();\\n"
              "}\\n"
            + "\"}}}";
        send_message(to_server[1], open);
        const std::string opened = read_message(from_server[0]);
        expect_has(opened, "publishDiagnostics");
        expect_has(opened, "unknown name");

        send_message(to_server[1],
            "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":"
            "\"textDocument/definition\",\"params\":{"
            "\"textDocument\":{\"uri\":\"file:///tmp/lsp-unit.scpp\"},"
            "\"position\":{\"line\":5,\"character\":11}}}");
        const std::string definition = read_message(from_server[0]);
        expect_has(definition, "\"id\":2");
        require(range_line(definition) == 0,
                "definition did not land on the function\n" + definition);

        send_message(to_server[1],
            "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":"
            "\"textDocument/completion\",\"params\":{"
            "\"textDocument\":{\"uri\":\"file:///tmp/lsp-unit.scpp\"},"
            "\"position\":{\"line\":5,\"character\":11}}}");
        const std::string items = read_message(from_server[0]);
        expect_has(items, "\"id\":3");
        expect_has(items, "\"label\":\"answer\"");
        expect_has(items, "\"label\":\"main\"");
        expect_has(items, "\"label\":\"int\"");

        const std::string fixed =
            "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\","
            "\"params\":{\"textDocument\":{"
            "\"uri\":\"file:///tmp/lsp-unit.scpp\",\"version\":2},"
            "\"contentChanges\":[{\"text\":"
            "\"int answer(void) {\\n    return 0;\\n}\\n\\n"
            "int main(void) {\\n    return answer();\\n}\\n\"}]}}";
        send_message(to_server[1], fixed);
        const std::string clean = read_message(from_server[0]);
        expect_has(clean, "\"diagnostics\":[]");

        const std::string saved =
            "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didSave\","
            "\"params\":{\"textDocument\":{"
            "\"uri\":\"file:///tmp/lsp-unit.scpp\"},\"text\":"
            "\"int answer(void) {\\n    return 0;\\n}\\n\\n"
            "int main(void) {\\n    return answer();\\n}\\n\"}}";
        send_message(to_server[1], saved);
        const std::string after_save = read_message(from_server[0]);
        expect_has(after_save, "\"diagnostics\":[]");

        const std::string broken =
            "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\","
            "\"params\":{\"textDocument\":{"
            "\"uri\":\"file:///tmp/lsp-unit.scpp\",\"version\":3},"
            "\"contentChanges\":[{\"text\":\"int x = ;\\n\"}]}}";
        send_message(to_server[1], broken);
        const std::string syntax = read_message(from_server[0]);
        expect_has(syntax, "expected");

        send_message(to_server[1],
            "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"shutdown\","
            "\"params\":null}");
        const std::string stopped = read_message(from_server[0]);
        expect_has(stopped, "\"id\":4");
        expect_has(stopped, "\"result\":null");

        send_message(to_server[1],
            "{\"jsonrpc\":\"2.0\",\"method\":\"exit\"}");
        close(to_server[1]);

        int status = 0;
        require(waitpid(child_pid, &status, 0) == child_pid, "waitpid");
        child_pid = -1;
        require(WIFEXITED(status), "server did not exit");
        require(WEXITSTATUS(status) == 0, "server exit status");
    } catch (const std::exception &error) {
        std::cerr << error.what() << "\n";
        if (child_pid > 0) {
            kill(child_pid, SIGKILL);
            waitpid(child_pid, nullptr, 0);
        }
        return 1;
    }
    return 0;
}
