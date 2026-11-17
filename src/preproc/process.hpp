#pragma once

#include <string>
#include <vector>

namespace smallcpp {

struct CliMacro {
    bool undef = false;
    std::string text;
};

struct PreprocRequest {
    std::string input;
    std::vector<std::string> includes;
    std::vector<CliMacro> cli_macros;
    std::vector<std::string> force_includes;
    bool ignore_environment = false;
    bool nostdinc = false;
};

/* Preprocess one translation unit. Throws std::runtime_error on failure.
 * The message is already a diagnostic. Nothing is deleted.
 */
std::string preprocess(const PreprocRequest &request);

}
