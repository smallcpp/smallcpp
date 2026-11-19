#include "config.hpp"

#include <cstdlib>
#include <fstream>
#include <string>

static std::string trim(const std::string &text)
{
    std::size_t begin = 0;
    while (begin < text.size()
           && (text[begin] == ' ' || text[begin] == '\t'
               || text[begin] == '\r'))
        ++begin;
    std::size_t end = text.size();
    while (end > begin
           && (text[end - 1] == ' ' || text[end - 1] == '\t'
               || text[end - 1] == '\r'))
        --end;
    return text.substr(begin, end - begin);
}

static bool absolute_home(std::string &home)
{
    const char *value = std::getenv("HOME");
    if (value == nullptr || value[0] == '\0')
        return false;
    if (value[0] != '/')
        return false;
    home = value;
    return true;
}

CompilerConfig read_compiler_config()
{
    CompilerConfig config = {};
    std::string home;
    if (!absolute_home(home))
        return config;
    const std::string path = home + "/.config/smallcpp/config.ini";
    std::ifstream in(path.c_str());
    if (!in)
        return config;
    config.present = true;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos || eq == 0)
            continue;
        const std::string key = trim(line.substr(0, eq));
        const std::string value = trim(line.substr(eq + 1));
        if (key.empty() || value.empty())
            continue;
        if (key == "include")
            config.include_path = value;
        else if (key == "library")
            config.library_path = value;
        else if (key == "runtime")
            config.runtime_path = value;
        else {
            ConfigLibrary library;
            library.key = key;
            library.path = value;
            config.libraries.push_back(library);
        }
    }
    return config;
}

std::string config_include_path(const CompilerConfig &config,
                                const std::string &prefix,
                                const char *fallback)
{
    if (!config.include_path.empty())
        return config.include_path;
    if (!prefix.empty())
        return prefix + "/include";
    if (fallback != nullptr)
        return fallback;
    return "";
}

std::string config_library_dir(const CompilerConfig &config,
                               const std::string &prefix)
{
    if (!config.library_path.empty())
        return config.library_path;
    if (!prefix.empty())
        return prefix + "/lib";
    return "";
}

std::string config_runtime_path(const CompilerConfig &config,
                                const std::string &prefix,
                                const char *fallback)
{
    if (!config.runtime_path.empty())
        return config.runtime_path;
    if (!config.library_path.empty())
        return config.library_path + "/libsmallcpp_runtime.a";
    if (!prefix.empty())
        return prefix + "/lib/libsmallcpp_runtime.a";
    if (fallback != nullptr)
        return fallback;
    return "";
}

static std::string short_library_name(const std::string &key)
{
    const std::string prefix = "lib";
    if (key.size() > prefix.size()
        && key.compare(0, prefix.size(), prefix) == 0)
        return key.substr(prefix.size());
    return key;
}

bool config_names_library(const std::vector<std::string> &names,
                          const std::string &key)
{
    const std::string short_name = short_library_name(key);
    const std::string with_lib = "lib" + short_name;
    for (const std::string &name : names) {
        if (name == key || name == short_name || name == with_lib)
            return true;
    }
    return false;
}

void config_append_libraries(const CompilerConfig &config,
                             const std::vector<std::string> &named,
                             std::vector<std::string> &args)
{
    bool saw_dl = false;
    if (config.present) {
        for (const ConfigLibrary &library : config.libraries) {
            if (library.key == "libdl")
                saw_dl = true;
            if (config_names_library(named, library.key))
                continue;
            args.push_back(library.path);
        }
    }
    if (!config.present || !saw_dl)
        args.push_back("-ldl");
}
