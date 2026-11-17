#include "smallcpp/CommandLine.hpp"

#include <cerrno>
#include <cstdio>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <vector>

extern char **environ;

#ifndef SMALLCPP_VERSION
#define SMALLCPP_VERSION "0.1"
#endif

#ifndef SMALLCPP_CC
#define SMALLCPP_CC "scpp_cc"
#endif

static smallcpp::CommandLine make_command_line()
{
    smallcpp::CommandLine command_line("scpp",
                                       "Compile one smallcpp source file to bitcode or a program.",
                                       SMALLCPP_VERSION);
    smallcpp::CommandLineOption help;
    help.short_name = 'h';
    help.long_name = "help";
    help.name = "help";
    help.description = "Print this helpful information";
    help.action = smallcpp::CommandLineAction::Help;
    command_line.add_option(help);

    smallcpp::CommandLineOption version;
    version.short_name = 'V';
    version.long_name = "version";
    version.name = "version";
    version.description = "Show the program version";
    version.action = smallcpp::CommandLineAction::Version;
    command_line.add_option(version);

    smallcpp::CommandLineOption output;
    output.short_name = 'o';
    output.long_name = "output";
    output.name = "output";
    output.description = "Write bitcode or a program to FILE";
    output.value_type = smallcpp::CommandLineValueType::String;
    command_line.add_option(output);

    command_line.add_positional(
        {"file", "input.scpp, and the output path when -o is omitted", true, true});
    return command_line;
}

static int usage_error(const smallcpp::CommandLine &command_line, const char *message)
{
    std::fprintf(stderr, "%s\n%s", message, command_line.help_text().c_str());
    return 2;
}

/* Run the C frontend. scpp does not parse and does not emit IR. */
static int run_frontend(const std::string &input, const std::string &output)
{
    const char *cc = SMALLCPP_CC;
    char *argv[] = {
        const_cast<char *>(cc),
        const_cast<char *>("-o"),
        const_cast<char *>(output.c_str()),
        const_cast<char *>(input.c_str()),
        nullptr};
    pid_t pid = 0;
    int status = posix_spawn(&pid, cc, nullptr, nullptr, argv, environ);
    if (status != 0) {
        errno = status;
        std::perror(cc);
        return 1;
    }
    int wait_status = 0;
    if (waitpid(pid, &wait_status, 0) < 0) {
        std::perror(cc);
        return 1;
    }
    if (WIFEXITED(wait_status))
        return WEXITSTATUS(wait_status);
    return 1;
}

int main(int argc, char **argv)
{
    smallcpp::CommandLine command_line = make_command_line();
    std::vector<std::string> arguments;
    for (int index = 1; index < argc; ++index)
        arguments.emplace_back(argv[index]);
    try {
        command_line.parse(arguments);
    } catch (const std::invalid_argument &error) {
        return usage_error(command_line, error.what());
    }
    if (command_line.action() == smallcpp::CommandLineAction::Help) {
        std::fputs(command_line.help_text().c_str(), stdout);
        return 0;
    }
    if (command_line.action() == smallcpp::CommandLineAction::Version) {
        std::fputs(command_line.version_text().c_str(), stdout);
        return 0;
    }

    const std::vector<std::string> &files = command_line.positional_values();
    std::string input;
    std::string output = command_line.value("output").value_or("");
    if (files.size() == 1) {
        input = files[0];
    } else if (files.size() == 2) {
        if (command_line.was_provided("output"))
            return usage_error(command_line, "output is already set");
        input = files[0];
        output = files[1];
    } else {
        return usage_error(command_line, "only one input file is supported");
    }
    if (output.empty())
        return usage_error(command_line, "required argument not found: output");
    return run_frontend(input, output);
}
