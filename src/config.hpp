#pragma once

#include <string>
#include <vector>

/* One library path from ~/.config/smallcpp/config.ini.
 * key is the ini name, such as libm. path is the file.
 */
struct ConfigLibrary {
    std::string key;
    std::string path;
};

/* Paths written by cmake --install.
 * present is false when HOME is missing or relative, or the file
 * is missing. The compile then uses the prefix fallback.
 * include_path is the header directory.
 * library_path is the default -L directory.
 * runtime_path is libsmallcpp_runtime.a.
 * libraries are the system libraries, in link order.
 */
struct CompilerConfig {
    bool present;
    std::string include_path;
    std::string library_path;
    std::string runtime_path;
    std::vector<ConfigLibrary> libraries;
};

CompilerConfig read_compiler_config();

std::string config_include_path(const CompilerConfig &config,
                                const std::string &prefix,
                                const char *fallback);

std::string config_library_dir(const CompilerConfig &config,
                               const std::string &prefix);

std::string config_runtime_path(const CompilerConfig &config,
                                const std::string &prefix,
                                const char *fallback);

/* True when names holds the -l spelling of key.
 * libm matches m and libm.
 */
bool config_names_library(const std::vector<std::string> &names,
                          const std::string &key);

/* Append the recorded libraries, skipping a key the user named.
 * A missing config, or a config with no libdl entry, appends -ldl.
 */
void config_append_libraries(const CompilerConfig &config,
                             const std::vector<std::string> &named,
                             std::vector<std::string> &args);
