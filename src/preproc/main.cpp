#include "config.hpp"
#include "prefix.hpp"
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
#ifndef SMALLCPP_PREFIX
#define SMALLCPP_PREFIX ""
#endif
#ifndef SMALLCPP_STDINC
#define SMALLCPP_STDINC ""
#endif

static smallcpp::CommandLine make_command_line()
{
    smallcpp::CommandLine command_line(
        "preproc",
        "Preprocess one source file. The process environment is imported as macros "
        "unless -i is set. Directives are #define, #undef, #include, "
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
    output.description = "Write preprocessed text to FILE. Without -o, write to standard output";
    output.value_type = smallcpp::CommandLineValueType::String;
    command_line.add_option(output);

    smallcpp::CommandLineOption include;
    include.short_name = 'I';
    include.long_name = "include-dir";
    include.name = "include-dir";
    include.description = "Search DIR for #include before the language headers, /usr/local/include, and /usr/include";
    include.value_type = smallcpp::CommandLineValueType::String;
    include.list = true;
    include.list_separator = '\n';
    command_line.add_option(include);

    smallcpp::CommandLineOption force_include;
    force_include.long_name = "include";
    force_include.name = "force-include";
    force_include.single_dash = true;
    force_include.description = "Include FILE before the source, as #include \"FILE\"";
    force_include.value_type = smallcpp::CommandLineValueType::String;
    force_include.list = true;
    force_include.list_separator = '\n';
    command_line.add_option(force_include);

    smallcpp::CommandLineOption nostdinc;
    nostdinc.long_name = "nostdinc";
    nostdinc.name = "nostdinc";
    nostdinc.single_dash = true;
    nostdinc.description = "Do not search the language headers, /usr/local/include, or /usr/include";
    command_line.add_option(nostdinc);

    smallcpp::CommandLineOption define;
    define.short_name = 'D';
    define.long_name = "define";
    define.name = "define";
    define.description = "Define NAME, or NAME=body. -D replaces an environment macro of the same name";
    define.value_type = smallcpp::CommandLineValueType::String;
    define.list = true;
    define.list_separator = '\n';
    command_line.add_option(define);

    smallcpp::CommandLineOption undefine;
    undefine.short_name = 'U';
    undefine.long_name = "undef";
    undefine.name = "undef";
    undefine.description = "Remove NAME, whether it came from the environment or from -D";
    undefine.value_type = smallcpp::CommandLineValueType::String;
    undefine.list = true;
    undefine.list_separator = '\n';
    command_line.add_option(undefine);

    smallcpp::CommandLineOption ignore_environment;
    ignore_environment.short_name = 'i';
    ignore_environment.long_name = "ignore-environment";
    ignore_environment.name = "ignore-environment";
    ignore_environment.description = "Import no macros from the process environment";
    command_line.add_option(ignore_environment);

    command_line.add_positional({"file", "input file", true, false});
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
    if (files.size() != 1)
        return usage_error(command_line, "only one input file is supported");
    const std::string input = files[0];
    const std::string output = command_line.value("output").value_or("");

    smallcpp::PreprocRequest request;
    request.input = input;
    request.includes = command_line.values("include-dir");
    request.force_includes = command_line.values("force-include");
    request.ignore_environment = command_line.switch_value("ignore-environment");
    request.nostdinc = command_line.switch_value("nostdinc");
    const std::string prefix = install_prefix(argv[0], SMALLCPP_PREFIX);
    const CompilerConfig config = read_compiler_config();
    request.stdinc = config_include_path(config, prefix, SMALLCPP_STDINC);
    for (const smallcpp::CommandLineEvent &event : command_line.events()) {
        if (event.name == "define")
            request.cli_macros.push_back(smallcpp::CliMacro{false, event.value});
        else if (event.name == "undef")
            request.cli_macros.push_back(smallcpp::CliMacro{true, event.value});
    }
    std::string text;
    try {
        text = smallcpp::preprocess(request);
    } catch (const std::runtime_error &error) {
        std::fputs(error.what(), stderr);
        if (error.what()[0] != '\0' && error.what()[std::char_traits<char>::length(error.what()) - 1] != '\n')
            std::fputc('\n', stderr);
        return 1;
    }

    if (output.empty()) {
        std::cout << text;
        if (!std::cout) {
            std::perror("stdout");
            return 1;
        }
        return 0;
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
