#include "prefix.hpp"

#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static std::string executable_path(const char *argv0)
{
    char buffer[PATH_MAX];
    const ssize_t length = ::readlink("/proc/self/exe", buffer,
                                      sizeof(buffer) - 1);
    if (length > 0) {
        buffer[length] = '\0';
        return buffer;
    }
    if (argv0 != nullptr)
        return argv0;
    return "";
}

std::string prefix_from_path(const std::string &executable,
                             const char *compiled_prefix)
{
    const std::string marker = "/bin/";
    const std::size_t pos = executable.rfind(marker);
    if (pos != std::string::npos) {
        if (pos == 0)
            return "/";
        return executable.substr(0, pos);
    }
    if (compiled_prefix != nullptr)
        return compiled_prefix;
    return "";
}

std::string install_prefix(const char *argv0, const char *compiled_prefix)
{
    return prefix_from_path(executable_path(argv0), compiled_prefix);
}
