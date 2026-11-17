#include "smallcpp/CommandLine.hpp"

#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern char **environ;

#ifndef SMALLCPP_VERSION
#define SMALLCPP_VERSION "0.1"
#endif
#ifndef SMALLCPP_PREPROC
#define SMALLCPP_PREPROC "preproc"
#endif
#ifndef SMALLCPP_CC
#define SMALLCPP_CC "scpp_cc"
#endif
#ifndef SMALLCPP_CLANGXX
#define SMALLCPP_CLANGXX "clang++"
#endif
#ifndef SMALLCPP_RUNTIME
#define SMALLCPP_RUNTIME ""
#endif

static smallcpp::CommandLine make_command_line()
{
    smallcpp::CommandLine command_line(
        "scpp",
        "Compile one smallcpp source file. Each option is passed only to the phase that uses it.",
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
    output.description = "Name the final result. With -c this is the object file. "
                         "With --emit-llvm this is LLVM IR. Otherwise this is the executable. "
                         "A path ending in .bc is LLVM IR";
    output.value_type = smallcpp::CommandLineValueType::String;
    command_line.add_option(output);

    smallcpp::CommandLineOption define;
    define.short_name = 'D';
    define.long_name = "define";
    define.name = "define";
    define.description = "Pass NAME or NAME=body to preproc";
    define.value_type = smallcpp::CommandLineValueType::String;
    define.list = true;
    define.list_separator = '\n';
    command_line.add_option(define);

    smallcpp::CommandLineOption include;
    include.short_name = 'I';
    include.long_name = "include-dir";
    include.name = "include-dir";
    include.description = "Pass DIR to preproc as an include directory";
    include.value_type = smallcpp::CommandLineValueType::String;
    include.list = true;
    include.list_separator = '\n';
    command_line.add_option(include);

    smallcpp::CommandLineOption ignore_environment;
    ignore_environment.short_name = 'i';
    ignore_environment.long_name = "ignore-environment";
    ignore_environment.name = "ignore-environment";
    ignore_environment.description = "Tell preproc to import no environment macros";
    command_line.add_option(ignore_environment);

    smallcpp::CommandLineOption compile_only;
    compile_only.short_name = 'c';
    compile_only.name = "compile";
    compile_only.description = "Compile and assemble, and do not link. -o names the object file";
    command_line.add_option(compile_only);

    smallcpp::CommandLineOption debug;
    debug.short_name = 'g';
    debug.name = "debug";
    debug.description = "Pass -g to scpp_cc and to the Clang link";
    command_line.add_option(debug);

    smallcpp::CommandLineOption library_path;
    library_path.short_name = 'L';
    library_path.long_name = "library-path";
    library_path.name = "library-path";
    library_path.description = "Pass DIR to the Clang link as a library search path";
    library_path.value_type = smallcpp::CommandLineValueType::String;
    library_path.list = true;
    library_path.list_separator = '\n';
    command_line.add_option(library_path);

    smallcpp::CommandLineOption emit_llvm;
    emit_llvm.long_name = "emit-llvm";
    emit_llvm.name = "emit-llvm";
    emit_llvm.description = "Ask scpp_cc to write LLVM IR and stop before the native link";
    command_line.add_option(emit_llvm);

    command_line.add_positional(
        {"file", "input.scpp, and the output path when -o is omitted", true, true});
    return command_line;
}

static int usage_error(const smallcpp::CommandLine &command_line, const char *message)
{
    std::fprintf(stderr, "%s\n%s", message, command_line.help_text().c_str());
    return 2;
}

static std::string tool_path(const char *env_name, const char *built_in)
{
    const char *value = std::getenv(env_name);
    if (value != nullptr && value[0] != '\0')
        return value;
    return built_in;
}

static bool ends_with(const std::string &path, const char *suffix)
{
    const std::size_t length = std::char_traits<char>::length(suffix);
    return path.size() >= length && path.compare(path.size() - length, length, suffix) == 0;
}

static int run_program(const std::string &program, const std::vector<std::string> &args,
                       const std::string &stdout_path)
{
    std::vector<char *> argv;
    argv.push_back(const_cast<char *>(program.c_str()));
    for (const std::string &arg : args)
        argv.push_back(const_cast<char *>(arg.c_str()));
    argv.push_back(nullptr);

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    int output_fd = -1;
    if (!stdout_path.empty()) {
        output_fd = open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_fd < 0) {
            std::perror(stdout_path.c_str());
            posix_spawn_file_actions_destroy(&actions);
            return 1;
        }
        posix_spawn_file_actions_adddup2(&actions, output_fd, STDOUT_FILENO);
    }

    pid_t pid = 0;
    const int spawned = posix_spawn(&pid, program.c_str(), &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    if (output_fd >= 0)
        close(output_fd);
    if (spawned != 0) {
        errno = spawned;
        std::perror(program.c_str());
        return 1;
    }
    int wait_status = 0;
    if (waitpid(pid, &wait_status, 0) < 0) {
        std::perror(program.c_str());
        return 1;
    }
    if (WIFEXITED(wait_status))
        return WEXITSTATUS(wait_status);
    return 1;
}

static std::string ignore_spelling(const std::vector<std::string> &arguments)
{
    for (const std::string &argument : arguments) {
        if (argument == "-i" || argument == "--ignore-environment")
            return argument;
    }
    return "-i";
}

static int compile_file(const smallcpp::CommandLine &command_line, const std::string &input,
                        const std::string &output, const std::string &ignore)
{
    const bool compile_only = command_line.switch_value("compile");
    bool emit_llvm = command_line.switch_value("emit-llvm");
    const bool debug = command_line.switch_value("debug");
    if (!emit_llvm && !compile_only && ends_with(output, ".bc"))
        emit_llvm = true;

    const std::string preprocessed = output + ".i";
    const std::string object = compile_only || emit_llvm ? output : output + ".o";
    const std::string preproc = tool_path("SMALLCPP_PREPROC", SMALLCPP_PREPROC);
    const std::string cc = tool_path("SMALLCPP_CC", SMALLCPP_CC);
    const std::string clang = tool_path("SMALLCPP_CLANGXX", SMALLCPP_CLANGXX);

    std::vector<std::string> preprocess;
    for (const smallcpp::CommandLineEvent &event : command_line.events()) {
        if (event.name == "define") {
            preprocess.push_back("-D");
            preprocess.push_back(event.value);
        } else if (event.name == "include-dir") {
            preprocess.push_back("-I");
            preprocess.push_back(event.value);
        } else if (event.name == "ignore-environment") {
            preprocess.push_back(ignore);
        }
    }
    preprocess.push_back(input);
    int status = run_program(preproc, preprocess, preprocessed);
    if (status != 0)
        return status;

    std::vector<std::string> frontend;
    if (debug)
        frontend.push_back("-g");
    if (emit_llvm)
        frontend.push_back("--emit-llvm");
    else
        frontend.push_back("-c");
    frontend.push_back("-o");
    frontend.push_back(object);
    frontend.push_back(preprocessed);
    status = run_program(cc, frontend, "");
    if (status != 0 || emit_llvm || compile_only)
        return status;

    std::vector<std::string> link;
    link.push_back("-no-pie");
    if (debug)
        link.push_back("-g");
    for (const smallcpp::CommandLineEvent &event : command_line.events()) {
        if (event.name == "library-path") {
            link.push_back("-L");
            link.push_back(event.value);
        }
    }
    link.push_back("-o");
    link.push_back(output);
    link.push_back(object);
    link.push_back(SMALLCPP_RUNTIME);
    link.push_back("-ldl");
    return run_program(clang, link, "");
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
    if (command_line.switch_value("compile") && command_line.switch_value("emit-llvm"))
        return usage_error(command_line, "-c and --emit-llvm cannot be combined");

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

    return compile_file(command_line, input, output, ignore_spelling(arguments));
}
