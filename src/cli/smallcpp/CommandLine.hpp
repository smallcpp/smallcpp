/*
 * Registration-based command-line parsing for smallcpp programs.
 * Adapted from the cwhip compiler parser in include/cwhip/CommandLine.hpp.
 */
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace smallcpp {

enum class CommandLineValueType { Switch, Counter, String, Number };
enum class CommandLineAction { None, Help, Version };

struct CommandLineOption {
    char short_name = '\0';
    std::string long_name;
    std::string name;
    std::string description;
    CommandLineValueType value_type = CommandLineValueType::Switch;
    bool list = false;
    char list_separator = ':';
    bool required = false;
    std::optional<std::string> default_value;
    CommandLineAction action = CommandLineAction::None;
};

struct CommandLinePositional {
    std::string name;
    std::string description;
    bool required = false;
    bool list = false;
};

class CommandLine {
public:
    CommandLine(std::string program_name, std::string description, std::string version);

    void add_option(CommandLineOption option);
    void add_positional(CommandLinePositional positional);
    void parse(const std::vector<std::string> &arguments);

    bool was_provided(const std::string &name) const;
    bool switch_value(const std::string &name) const;
    std::size_t count(const std::string &name) const;
    std::optional<std::string> value(const std::string &name) const;
    const std::vector<std::string> &values(const std::string &name) const;
    bool contains(const std::string &name, const std::string &item) const;
    const std::vector<std::string> &positional_values() const noexcept;
    CommandLineAction action() const noexcept;

    std::string help_text() const;
    std::string version_text() const;

private:
    const CommandLineOption *find_long(const std::string &name) const;
    const CommandLineOption *find_short(char name) const;
    void record_value(const CommandLineOption &option, const std::string &value, bool provided);
    void record_positional(const std::string &value);
    std::string display_name(const CommandLineOption &option) const;

    std::string program_name_;
    std::string description_;
    std::string version_;
    std::vector<CommandLineOption> options_;
    std::optional<CommandLinePositional> positional_;
    std::unordered_map<std::string, std::vector<std::string>> values_;
    std::unordered_map<std::string, bool> provided_;
    std::unordered_map<std::string, std::size_t> counts_;
    std::vector<std::string> positional_values_;
    CommandLineAction action_ = CommandLineAction::None;
};

}
