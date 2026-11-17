#include "smallcpp/CommandLine.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <fstream>
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
        "Compile smallcpp source files and link them. Each option is passed only to the phase that uses it. "
        "preproc deletes nothing. Without -k, scpp deletes intermediate files after a successful build.",
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
    output.description = "Name the result. The default executable is a.out. "
                         "With -c this is the object file. "
                         "With one --emit-llvm or -E input, this names that file. "
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
    include.description = "Pass DIR to preproc. Repeat -I, or separate several directories with ':'. "
                          "They are searched in order";
    include.value_type = smallcpp::CommandLineValueType::String;
    include.list = true;
    include.list_separator = ':';
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

    smallcpp::CommandLineOption preprocess_only;
    preprocess_only.short_name = 'E';
    preprocess_only.name = "preprocess";
    preprocess_only.description = "Write preprocessor output for each input and stop. "
                                  "The output base name is the input base name";
    command_line.add_option(preprocess_only);

    smallcpp::CommandLineOption keep;
    keep.short_name = 'k';
    keep.long_name = "keep";
    keep.name = "keep";
    keep.description = "Keep every generated file. Without -k, scpp deletes intermediate files "
                       "after a successful build";
    command_line.add_option(keep);

    smallcpp::CommandLineOption debug;
    debug.short_name = 'g';
    debug.name = "debug";
    debug.description = "Ask scpp_cc for source locations, functions, and types, and keep them in the Clang link";
    command_line.add_option(debug);

    smallcpp::CommandLineOption library_path;
    library_path.short_name = 'L';
    library_path.long_name = "library-path";
    library_path.name = "library-path";
    library_path.description = "Pass DIR to the Clang link. Repeat -L, or separate several directories with ':'. "
                               "They are searched in order";
    library_path.value_type = smallcpp::CommandLineValueType::String;
    library_path.list = true;
    library_path.list_separator = ':';
    command_line.add_option(library_path);

    smallcpp::CommandLineOption library;
    library.short_name = 'l';
    library.long_name = "library";
    library.name = "library";
    library.description = "Link the named library into the executable. Repeat -l for another library";
    library.value_type = smallcpp::CommandLineValueType::String;
    library.list = true;
    library.list_separator = '\n';
    command_line.add_option(library);

    smallcpp::CommandLineOption emit_llvm;
    emit_llvm.long_name = "emit-llvm";
    emit_llvm.name = "emit-llvm";
    emit_llvm.description = "Write LLVM IR for each input and stop before assembling or linking. "
                            "The output base name is the input base name";
    command_line.add_option(emit_llvm);

    command_line.add_positional(
        {"file",
         "One or more input.scpp files. Without -o, two or more paths use the last path as the output. "
         "One path writes a.out, unless -E or --emit-llvm names each file from its base name",
         true, true});
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

static bool read_u16(const std::vector<unsigned char> &bytes, std::size_t at, std::uint16_t &out)
{
    if (at + 2 > bytes.size())
        return false;
    out = static_cast<std::uint16_t>(bytes[at] | (bytes[at + 1] << 8));
    return true;
}

static bool read_u32(const std::vector<unsigned char> &bytes, std::size_t at, std::uint32_t &out)
{
    if (at + 4 > bytes.size())
        return false;
    out = static_cast<std::uint32_t>(bytes[at])
        | (static_cast<std::uint32_t>(bytes[at + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[at + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[at + 3]) << 24);
    return true;
}

static bool read_u64(const std::vector<unsigned char> &bytes, std::size_t at, std::uint64_t &out)
{
    std::uint32_t lo = 0;
    std::uint32_t hi = 0;
    if (!read_u32(bytes, at, lo) || !read_u32(bytes, at + 4, hi))
        return false;
    out = static_cast<std::uint64_t>(lo) | (static_cast<std::uint64_t>(hi) << 32);
    return true;
}

static bool defines_main(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)),
                                     std::istreambuf_iterator<char>());
    if (bytes.size() < 64 || bytes[0] != 0x7f || bytes[1] != 'E' || bytes[2] != 'L' || bytes[3] != 'F')
        return false;
    if (bytes[4] != 2 || bytes[5] != 1)
        return false;
    std::uint64_t shoff = 0;
    std::uint16_t shentsize = 0;
    std::uint16_t shnum = 0;
    if (!read_u64(bytes, 40, shoff) || !read_u16(bytes, 58, shentsize) || !read_u16(bytes, 60, shnum))
        return false;
    if (shentsize < 64)
        return false;
    for (std::uint16_t section = 0; section < shnum; ++section) {
        const std::size_t header = static_cast<std::size_t>(shoff) + static_cast<std::size_t>(section) * shentsize;
        std::uint32_t type = 0;
        std::uint64_t offset = 0;
        std::uint64_t size = 0;
        std::uint32_t link = 0;
        std::uint64_t entsize = 0;
        if (!read_u32(bytes, header + 4, type))
            return false;
        if (type != 2)
            continue;
        if (!read_u64(bytes, header + 24, offset) || !read_u64(bytes, header + 32, size)
            || !read_u32(bytes, header + 40, link) || !read_u64(bytes, header + 56, entsize))
            return false;
        if (entsize < 24 || link >= shnum)
            return false;
        const std::size_t strings = static_cast<std::size_t>(shoff) + static_cast<std::size_t>(link) * shentsize;
        std::uint64_t name_offset = 0;
        std::uint64_t name_size = 0;
        if (!read_u64(bytes, strings + 24, name_offset) || !read_u64(bytes, strings + 32, name_size))
            return false;
        if (name_offset + name_size > bytes.size())
            return false;
        const std::size_t count = static_cast<std::size_t>(size / entsize);
        for (std::size_t index = 0; index < count; ++index) {
            const std::size_t sym = static_cast<std::size_t>(offset) + index * static_cast<std::size_t>(entsize);
            std::uint32_t name = 0;
            std::uint16_t shndx = 0;
            if (sym + 8 > bytes.size() || !read_u32(bytes, sym, name) || !read_u16(bytes, sym + 6, shndx))
                return false;
            const unsigned bind = bytes[sym + 4] >> 4;
            if (shndx == 0 || shndx >= 0xff00 || (bind != 1 && bind != 2))
                continue;
            if (static_cast<std::uint64_t>(name) + 5 > name_size)
                continue;
            const std::size_t text = static_cast<std::size_t>(name_offset) + name;
            const char *symbol = reinterpret_cast<const char *>(&bytes[text]);
            if (std::char_traits<char>::compare(symbol, "main", 4) == 0 && symbol[4] == '\0')
                return true;
        }
    }
    return false;
}

static int require_main(const std::vector<std::string> &objects)
{
    for (const std::string &object : objects) {
        if (defines_main(object))
            return 0;
    }
    std::fputs("program has no main\n", stderr);
    return 1;
}

enum class Stop { Link, Object, Ir, Preprocess };

struct Unit {
    std::string input;
    std::string preprocessed;
    std::string product;
};

static std::string base_name(const std::string &path)
{
    const auto slash = path.find_last_of('/');
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    const auto dot = name.find_last_of('.');
    if (dot != std::string::npos && dot != 0)
        name.resize(dot);
    return name;
}

static std::vector<std::string> split_colon(const std::string &value)
{
    std::vector<std::string> items;
    std::size_t start = 0;
    while (start <= value.size()) {
        const auto found = value.find(':', start);
        const auto end = found == std::string::npos ? value.size() : found;
        if (end > start)
            items.push_back(value.substr(start, end - start));
        if (found == std::string::npos)
            break;
        start = found + 1;
    }
    return items;
}

static int remove_intermediates(const std::vector<std::string> &paths)
{
    for (const std::string &path : paths) {
        if (unlink(path.c_str()) != 0 && errno != ENOENT) {
            std::perror(path.c_str());
            return 1;
        }
    }
    return 0;
}

static int finish(int status, bool keep, const std::vector<std::string> &intermediates)
{
    if (status != 0 || keep)
        return status;
    const int removed = remove_intermediates(intermediates);
    return removed != 0 ? removed : status;
}

static void append_preprocess_args(std::vector<std::string> &args, const smallcpp::CommandLine &command_line,
                                   const std::string &ignore)
{
    for (const smallcpp::CommandLineEvent &event : command_line.events()) {
        if (event.name == "define") {
            args.push_back("-D");
            args.push_back(event.value);
        } else if (event.name == "include-dir") {
            for (const std::string &dir : split_colon(event.value)) {
                args.push_back("-I");
                args.push_back(dir);
            }
        } else if (event.name == "ignore-environment") {
            args.push_back(ignore);
        }
    }
}

static void append_library_paths(std::vector<std::string> &args, const smallcpp::CommandLine &command_line)
{
    for (const smallcpp::CommandLineEvent &event : command_line.events()) {
        if (event.name != "library-path")
            continue;
        for (const std::string &dir : split_colon(event.value)) {
            args.push_back("-L");
            args.push_back(dir);
        }
    }
}

static void append_library_names(std::vector<std::string> &args, const smallcpp::CommandLine &command_line)
{
    for (const smallcpp::CommandLineEvent &event : command_line.events()) {
        if (event.name != "library")
            continue;
        args.push_back("-l");
        args.push_back(event.value);
    }
}

static bool ir_output_name(const std::string &path)
{
    return ends_with(path, ".bc") || ends_with(path, ".ll");
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

    const bool compile_only = command_line.switch_value("compile");
    const bool emit_flag = command_line.switch_value("emit-llvm");
    const bool preprocess_only = command_line.switch_value("preprocess");
    if (compile_only && emit_flag)
        return usage_error(command_line, "-c and --emit-llvm cannot be combined");
    if (preprocess_only && (compile_only || emit_flag))
        return usage_error(command_line, "-E cannot be combined with -c or --emit-llvm");

    const std::vector<std::string> &files = command_line.positional_values();
    if (files.empty())
        return usage_error(command_line, "required argument not found: file");

    const bool have_output = command_line.was_provided("output");
    std::string output = command_line.value("output").value_or("");
    std::vector<std::string> inputs;
    bool per_input = false;
    if (preprocess_only || emit_flag) {
        if (have_output) {
            if (files.size() != 1)
                return usage_error(command_line, "one output file cannot hold several translation units");
            inputs = files;
        } else if (files.size() == 2
                   && ((emit_flag && ir_output_name(files[1]))
                       || (preprocess_only && ends_with(files[1], ".i")))) {
            inputs.push_back(files[0]);
            output = files[1];
        } else {
            inputs = files;
            per_input = true;
        }
    } else if (have_output) {
        inputs = files;
    } else if (files.size() >= 2) {
        inputs.assign(files.begin(), files.end() - 1);
        output = files.back();
    } else {
        inputs = files;
        output = compile_only ? base_name(files[0]) + ".o" : "a.out";
    }
    if (!per_input && output.empty())
        return usage_error(command_line, "required argument not found: output");

    Stop stop = Stop::Link;
    if (preprocess_only)
        stop = Stop::Preprocess;
    else if (compile_only)
        stop = Stop::Object;
    else if (emit_flag || (!per_input && ends_with(output, ".bc")))
        stop = Stop::Ir;
    if (inputs.size() > 1 && !per_input && stop != Stop::Link)
        return usage_error(command_line, "one output file cannot hold several translation units");

    std::vector<Unit> units;
    if (stop == Stop::Link) {
        for (const std::string &input : inputs) {
            Unit unit;
            unit.input = input;
            const std::string base = base_name(input);
            unit.preprocessed = base + ".i";
            unit.product = base + ".o";
            units.push_back(unit);
        }
    } else if (per_input) {
        for (const std::string &input : inputs) {
            Unit unit;
            unit.input = input;
            const std::string base = base_name(input);
            if (stop == Stop::Preprocess)
                unit.product = base + ".i";
            else {
                unit.preprocessed = base + ".i";
                unit.product = base + ".bc";
            }
            units.push_back(unit);
        }
    } else if (inputs.size() == 1) {
        Unit unit;
        unit.input = inputs[0];
        if (stop == Stop::Preprocess)
            unit.product = output;
        else {
            unit.preprocessed = output + ".i";
            unit.product = output;
        }
        units.push_back(unit);
    }

    const std::string preproc = tool_path("SMALLCPP_PREPROC", SMALLCPP_PREPROC);
    const std::string cc = tool_path("SMALLCPP_CC", SMALLCPP_CC);
    const std::string clang = tool_path("SMALLCPP_CLANGXX", SMALLCPP_CLANGXX);
    const bool debug = command_line.switch_value("debug");
    const bool keep = command_line.switch_value("keep");
    const std::string ignore = ignore_spelling(arguments);
    std::vector<std::string> intermediates;
    std::vector<std::string> objects;

    for (const Unit &unit : units) {
        std::vector<std::string> preprocess;
        append_preprocess_args(preprocess, command_line, ignore);
        preprocess.push_back(unit.input);
        if (stop == Stop::Preprocess) {
            const int status = run_program(preproc, preprocess, unit.product);
            if (status != 0)
                return status;
            continue;
        }
        int status = run_program(preproc, preprocess, unit.preprocessed);
        if (status != 0)
            return status;
        intermediates.push_back(unit.preprocessed);

        std::vector<std::string> frontend;
        if (debug) {
            frontend.push_back("-g");
            frontend.push_back("--debug-file");
            frontend.push_back(unit.input);
        }
        if (stop == Stop::Ir)
            frontend.push_back("--emit-llvm");
        else
            frontend.push_back("-c");
        frontend.push_back("-o");
        frontend.push_back(unit.product);
        frontend.push_back(unit.preprocessed);
        status = run_program(cc, frontend, "");
        if (status != 0)
            return status;
        if (stop == Stop::Link) {
            objects.push_back(unit.product);
            intermediates.push_back(unit.product);
        }
    }
    if (stop != Stop::Link)
        return finish(0, keep, intermediates);

    const int ready = require_main(objects);
    if (ready != 0)
        return ready;

    std::vector<std::string> link;
    link.push_back("-no-pie");
    if (debug)
        link.push_back("-g");
    append_library_paths(link, command_line);
    link.push_back("-o");
    link.push_back(output);
    for (const std::string &object : objects)
        link.push_back(object);
    append_library_names(link, command_line);
    link.push_back("-Wl,-u,smallcpp_uncaught_init");
    link.push_back(SMALLCPP_RUNTIME);
    link.push_back("-ldl");
    const int status = run_program(clang, link, "");
    return finish(status, keep, intermediates);
}
