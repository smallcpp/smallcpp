#include "codegen.h"
#include "smallcpp/CommandLine.hpp"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include "diagnostic.h"
}

extern "C" {
#include "smallcpp.tab.h"

extern FILE *yyin;
extern int yylineno;
extern int yycolumn;

static int error_count;

void yyerror(const char *message)
{
    int line = yylloc.first_line > 0 ? yylloc.first_line : 1;
    int column = yylloc.first_column > 0 ? yylloc.first_column : 1;

    smallcpp_diag_report(line, column, message);
    error_count++;
}
}

#ifndef SMALLCPP_VERSION
#define SMALLCPP_VERSION "0.1"
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

    int status;
    smallcpp_ast = NULL;
    error_count = 0;
    yylineno = 1;
    yycolumn = 1;
    yylloc.first_line = 1;
    yylloc.first_column = 1;
    yylloc.last_line = 1;
    yylloc.last_column = 1;
    smallcpp_diag_load(input.c_str());
    yyin = std::fopen(input.c_str(), "r");
    if (yyin == NULL) {
        std::perror(input.c_str());
        return 2;
    }
    status = yyparse();
    std::fclose(yyin);
    if (status != 0 || error_count != 0 || smallcpp_ast == NULL) {
        ast_free(smallcpp_ast);
        smallcpp_ast = NULL;
        return 1;
    }
    std::size_t length = output.size();
    if (length >= 3 && output.compare(length - 3, 3, ".bc") == 0)
        status = compile_to_bitcode(smallcpp_ast, output.c_str());
    else
        status = compile_to_executable(smallcpp_ast, output.c_str());
    ast_free(smallcpp_ast);
    smallcpp_ast = NULL;
    return status;
}
