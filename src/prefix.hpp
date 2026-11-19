#pragma once

#include <string>

/* Prefix that contains bin/, include/, and lib/.
 * An executable in bin/ uses the directory above bin/.
 * compiled_prefix is the configure-time install prefix. It is used when
 * the executable is not inside bin/. An empty compiled prefix yields
 * an empty result.
 */
std::string prefix_from_path(const std::string &executable,
                             const char *compiled_prefix);

std::string install_prefix(const char *argv0, const char *compiled_prefix);
