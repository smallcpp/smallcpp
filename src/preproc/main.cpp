#include "process.hpp"
#include "smallcpp/CommandLine.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef SMALLCPP_VERSION
#define SMALLCPP_VERSION "0.1"
#endif

static smallcpp::CommandLine make_command_line()
{
    smallcpp::CommandLine command_line(
        "preproc",
        "Preprocess one source file. Directives are #define, #undef, #include, "
        "#if, #elif, #else, #endif, #ifdef, #ifndef, #error, and #line. "
        "#pragma is rejected. Expansion repeats until it finishes.",
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
    output.description = "Write preprocessed text to FILE";
    output.value_type = smallcpp::CommandLineValueType::String;
    command_line.add_option(output);

    smallcpp::CommandLineOption include;
    include.short_name = 'I';
    include.long_name = "include";
    include.name = "include";
    include.description = "Search DIR for #include";
    include.value_type = smallcpp::CommandLineValueType::String;
    include.list = true;
    include.list_separator = '\n';
    command_line.add_option(include);

    smallcpp::CommandLineOption define;
    define.short_name = 'D';
    define.long_name = "define";
    define.name = "define";
    define.description = "Define NAME, or NAME=body, before the file is read";
    define.value_type = smallcpp::CommandLineValueType::String;
    define.list = true;
    define.list_separator = '\n';
    command_line.add_option(define);

    command_line.add_positional(
        {"file", "input file, and the output path when -o is omitted", true, true});
    return command_line;
}

static int usage_error(const smallcpp::CommandLine &command_line, const char *message)
{
    std::fprintf(stderr, "%s\n%s", message, command_line.help_text().c_str());
    return 2;
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

    smallcpp::PreprocRequest request;
    request.input = input;
    request.includes = command_line.values("include");
    request.defines = command_line.values("define");
    std::string text;
    try {
        text = smallcpp::preprocess(request);
    } catch (const std::runtime_error &error) {
        std::fputs(error.what(), stderr);
        if (error.what()[0] != '\0' && error.what()[std::char_traits<char>::length(error.what()) - 1] != '\n')
            std::fputc('\n', stderr);
        return 1;
    }

    std::ofstream out(output, std::ios::binary);
    if (!out) {
        std::perror(output.c_str());
        return 1;
    }
    out << text;
    if (!out) {
        std::perror(output.c_str());
        return 1;
    }
    return 0;
}
