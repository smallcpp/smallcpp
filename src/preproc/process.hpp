#pragma once

#include <string>
#include <vector>

namespace smallcpp {

struct PreprocRequest {
    std::string input;
    std::vector<std::string> includes;
    std::vector<std::string> defines;
};

/* Preprocess one translation unit. Throws std::runtime_error on failure.
 * The message is already a diagnostic. Nothing is deleted.
 */
std::string preprocess(const PreprocRequest &request);

}
