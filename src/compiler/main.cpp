#include "dump.h"
#include "emit.h"
#include "smallcpp/CommandLine.hpp"

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include "diagnostic.h"
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
    smallcpp::CommandLine command_line(
        "scpp_cc",
        "Parse one smallcpp source file. Pass the syntax tree to the IR emitter, "
        "write it as text, or write a Graphviz DOT file. Any combination is accepted.",
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
    output.description = "Write LLVM IR with --emit-llvm or a .bc path, an object with -c, "
                         "otherwise an executable";
    output.value_type = smallcpp::CommandLineValueType::String;
    command_line.add_option(output);

    smallcpp::CommandLineOption compile_only;
    compile_only.short_name = 'c';
    compile_only.name = "compile";
    compile_only.description = "Compile and assemble, and do not link";
    command_line.add_option(compile_only);

    smallcpp::CommandLineOption debug;
    debug.short_name = 'g';
    debug.name = "debug";
    debug.description = "Record the source file, line numbers, functions, and types";
    command_line.add_option(debug);

    smallcpp::CommandLineOption debug_file;
    debug_file.long_name = "debug-file";
    debug_file.name = "debug-file";
    debug_file.description = "Name the source file recorded by -g. The default is the input file";
    debug_file.value_type = smallcpp::CommandLineValueType::String;
    command_line.add_option(debug_file);

    smallcpp::CommandLineOption emit_llvm;
    emit_llvm.long_name = "emit-llvm";
    emit_llvm.name = "emit-llvm";
    emit_llvm.description = "Write LLVM IR and stop before the native link";
    command_line.add_option(emit_llvm);

    smallcpp::CommandLineOption ast_text;
    ast_text.long_name = "ast-text";
    ast_text.name = "ast-text";
    ast_text.description = "Write a structured text form of the syntax tree to FILE";
    ast_text.value_type = smallcpp::CommandLineValueType::String;
    command_line.add_option(ast_text);

    smallcpp::CommandLineOption ast_dot;
    ast_dot.long_name = "ast-dot";
    ast_dot.name = "ast-dot";
    ast_dot.description = "Write a Graphviz DOT file of the syntax tree to FILE";
    ast_dot.value_type = smallcpp::CommandLineValueType::String;
    command_line.add_option(ast_dot);

    command_line.add_positional({"file", "input.scpp", true, false});
    return command_line;
}

static int usage_error(const smallcpp::CommandLine &command_line, const char *message)
{
    std::fprintf(stderr, "%s\n%s", message, command_line.help_text().c_str());
    return 2;
}

static int parse_file(const std::string &input)
{
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
    return 0;
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
    const std::string text_path = command_line.value("ast-text").value_or("");
    const std::string dot_path = command_line.value("ast-dot").value_or("");
    const bool compile_only = command_line.switch_value("compile");
    const bool emit_llvm = command_line.switch_value("emit-llvm");
    const bool debug = command_line.switch_value("debug");
    if (compile_only && emit_llvm)
        return usage_error(command_line, "-c and --emit-llvm cannot be combined");
    if (output.empty() && text_path.empty() && dot_path.empty())
        return usage_error(command_line, "name an output with -o, --ast-text, or --ast-dot");

    int status = parse_file(input);
    if (status != 0)
        return status;
    if (!text_path.empty())
        status = ast_write_text(smallcpp_ast, text_path.c_str());
    if (status == 0 && !dot_path.empty())
        status = ast_write_dot(smallcpp_ast, dot_path.c_str());
    if (status == 0 && !output.empty()) {
        const std::size_t length = output.size();
        const bool bitcode_path = length >= 3 && output.compare(length - 3, 3, ".bc") == 0;
        const std::string named = command_line.value("debug-file").value_or("");
        const char *source = nullptr;
        if (debug)
            source = named.empty() ? input.c_str() : named.c_str();
        const int debug_flag = debug ? 1 : 0;
        if (emit_llvm || (bitcode_path && !compile_only))
            status = smallcpp_emit_bitcode(smallcpp_ast, output.c_str(), debug_flag, source);
        else if (compile_only)
            status = smallcpp_emit_object(smallcpp_ast, output.c_str(), debug_flag, source);
        else
            status = smallcpp_emit_executable(smallcpp_ast, output.c_str(), debug_flag, source);
    }
    ast_free(smallcpp_ast);
    smallcpp_ast = NULL;
    return status;
}
