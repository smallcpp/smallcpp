#include "prefix.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void expect_prefix(const std::string &executable, const char *compiled,
                   const std::string &expected)
{
    const std::string got = prefix_from_path(executable, compiled);
    const std::string message = "prefix of '" + executable + "' is '" + got
                               + "', expected '" + expected + "'";
    require(got == expected, message);
}

}

int main(int argc, char **argv)
{
    try {
        expect_prefix("/home/user/opt/smallcpp/bin/scpp", "/other",
                      "/home/user/opt/smallcpp");
        expect_prefix("/tmp/tree/bin/preproc", "", "/tmp/tree");
        expect_prefix("/bin/scpp", "/nope", "/");
        expect_prefix("scpp", "/opt/smallcpp", "/opt/smallcpp");
        expect_prefix("", nullptr, "");
        expect_prefix("/opt/smallcpp/lib/scpp", "/fallback", "/fallback");

        const std::string live = install_prefix(argv[0],
                                                "/compiled/should-not-win");
        require(live != "/compiled/should-not-win",
                "running prefix used the compiled fallback: " + live);
        require(live.find("/bin/") == std::string::npos,
                "running prefix still contains bin: " + live);
        require(!live.empty(), "running prefix is empty");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
