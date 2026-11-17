/*
 * Registration-based command-line parsing for smallcpp programs.
 * Adapted from the cwhip compiler parser in src/CommandLine.cpp.
 */
#include "smallcpp/CommandLine.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace smallcpp {
namespace {

std::vector<std::string> split_list(const std::string &value, char separator)
{
    std::vector<std::string> result;
    std::size_t start = 0;

    while (start <= value.size()) {
        const auto found = value.find(separator, start);
        const auto end = found == std::string::npos ? value.size() : found;
        result.push_back(value.substr(start, end - start));
        if (found == std::string::npos || found + 1 == value.size())
            break;
        start = found + 1;
    }
    return result;
}

}

CommandLine::CommandLine(std::string program_name, std::string description, std::string version)
    : program_name_(std::move(program_name)), description_(std::move(description)),
      version_(std::move(version))
{
}

std::string CommandLine::display_name(const CommandLineOption &option) const
{
    if (option.long_name.empty())
        return std::string("-") + option.short_name;
    return "--" + option.long_name;
}

void CommandLine::add_option(CommandLineOption option)
{
    if (option.name.empty() || (option.long_name.empty() && option.short_name == '\0'))
        throw std::invalid_argument("command-line options need a value name and an option name");
    const bool takes_value = option.value_type == CommandLineValueType::String
        || option.value_type == CommandLineValueType::Number;
    if (!takes_value && option.list)
        throw std::invalid_argument("switch options cannot accept list values");
    if (option.list && option.list_separator == '\0')
        throw std::invalid_argument("list options need a list separator");
    const auto duplicate_name = std::find_if(options_.begin(), options_.end(),
        [&option](const CommandLineOption &current) { return current.name == option.name; });
    if (duplicate_name != options_.end() || find_long(option.long_name) != nullptr
        || (option.short_name != '\0' && find_short(option.short_name) != nullptr))
        throw std::invalid_argument("duplicate command-line option");
    options_.push_back(std::move(option));
}

void CommandLine::add_positional(CommandLinePositional positional)
{
    if (positional.name.empty())
        throw std::invalid_argument("positional arguments need a value name");
    if (positional_)
        throw std::invalid_argument("only one positional argument is supported");
    positional_ = std::move(positional);
}

const CommandLineOption *CommandLine::find_long(const std::string &name) const
{
    const auto found = std::find_if(options_.begin(), options_.end(),
        [&name](const CommandLineOption &option) {
            return !option.long_name.empty() && option.long_name == name;
        });
    return found == options_.end() ? nullptr : &*found;
}

const CommandLineOption *CommandLine::find_short(char name) const
{
    const auto found = std::find_if(options_.begin(), options_.end(),
        [name](const CommandLineOption &option) { return option.short_name == name; });
    return found == options_.end() ? nullptr : &*found;
}

void CommandLine::record_value(const CommandLineOption &option, const std::string &value,
                               bool provided)
{
    auto &destination = values_[option.name];
    if (!option.list)
        destination.clear();
    auto items = option.list ? split_list(value, option.list_separator)
                             : std::vector<std::string>{value};
    if (option.value_type == CommandLineValueType::Number) {
        for (const auto &item : items) {
            std::int64_t number = 0;
            const auto result = std::from_chars(item.data(), item.data() + item.size(), number);
            if (item.empty() || result.ec != std::errc{} || result.ptr != item.data() + item.size())
                throw std::invalid_argument("option " + display_name(option) + " requires a number");
        }
    }
    destination.insert(destination.end(), items.begin(), items.end());
    if (provided)
        provided_[option.name] = true;
}

void CommandLine::record_positional(const std::string &value)
{
    if (!positional_)
        throw std::invalid_argument("unexpected positional argument: " + value);
    if (positional_->list)
        positional_values_.push_back(value);
    else if (positional_values_.empty())
        positional_values_.push_back(value);
    else
        throw std::invalid_argument("only one input file is supported");
}

void CommandLine::parse(const std::vector<std::string> &arguments)
{
    values_.clear();
    provided_.clear();
    counts_.clear();
    positional_values_.clear();
    events_.clear();
    action_ = CommandLineAction::None;
    for (const auto &option : options_) {
        if (option.default_value)
            record_value(option, *option.default_value, false);
    }

    const auto process = [this](const CommandLineOption &option,
                                const std::optional<std::string> &attached, std::size_t &index,
                                const std::vector<std::string> &args) {
        if (option.action != CommandLineAction::None) {
            if (attached)
                throw std::invalid_argument("option " + display_name(option)
                                            + " does not accept an argument");
            action_ = option.action;
            return true;
        }
        if (option.value_type == CommandLineValueType::Switch
            || option.value_type == CommandLineValueType::Counter) {
            if (attached)
                throw std::invalid_argument("option " + display_name(option)
                                            + " does not accept an argument");
            provided_[option.name] = true;
            if (option.value_type == CommandLineValueType::Counter)
                ++counts_[option.name];
            events_.push_back(CommandLineEvent{option.name, {}});
            return false;
        }
        std::string argument;
        if (attached) {
            argument = *attached;
        } else {
            if (index + 1 >= args.size())
                throw std::invalid_argument("option " + display_name(option) + " requires an argument");
            argument = args[++index];
        }
        if (argument.empty())
            throw std::invalid_argument("option " + display_name(option)
                                        + " requires a non-empty argument");
        record_value(option, argument, true);
        events_.push_back(CommandLineEvent{option.name, argument});
        return false;
    };

    bool positional_only = false;
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const auto &argument = arguments[index];
        if (!positional_only && argument == "--") {
            positional_only = true;
            continue;
        }
        if (!positional_only && argument.size() > 1 && argument.rfind("--", 0) == 0) {
            const auto separator = argument.find('=');
            const auto name = argument.substr(2, separator == std::string::npos
                                                      ? std::string::npos
                                                      : separator - 2);
            const auto *option = find_long(name);
            if (option == nullptr)
                throw std::invalid_argument("unknown option: --" + name);
            const std::optional<std::string> attached = separator == std::string::npos
                ? std::nullopt
                : std::optional<std::string>(argument.substr(separator + 1));
            if (process(*option, attached, index, arguments))
                return;
            continue;
        }
        if (!positional_only && argument.size() > 1 && argument[0] == '-') {
            const auto separator = argument.find('=');
            const auto word = argument.substr(1, separator == std::string::npos
                                                       ? std::string::npos
                                                       : separator - 1);
            if (const CommandLineOption *single = find_long(word);
                single != nullptr && single->single_dash) {
                const std::optional<std::string> attached = separator == std::string::npos
                    ? std::nullopt
                    : std::optional<std::string>(argument.substr(separator + 1));
                if (process(*single, attached, index, arguments))
                    return;
                continue;
            }
            for (std::size_t offset = 1; offset < argument.size(); ++offset) {
                const auto *option = find_short(argument[offset]);
                if (option == nullptr)
                    throw std::invalid_argument(std::string("unknown option: -") + argument[offset]);
                const bool flag = option->value_type == CommandLineValueType::Switch
                    || option->value_type == CommandLineValueType::Counter;
                if (flag && option->action == CommandLineAction::None) {
                    const bool attached_value = offset + 1 < argument.size()
                        && argument[offset + 1] == '=';
                    const std::optional<std::string> attached = attached_value
                        ? std::optional<std::string>(argument.substr(offset + 2))
                        : std::nullopt;
                    if (process(*option, attached, index, arguments))
                        return;
                    if (attached_value)
                        break;
                    continue;
                }
                std::optional<std::string> attached;
                if (offset + 1 < argument.size()) {
                    auto value_start = offset + 1;
                    if (argument[value_start] == '=')
                        ++value_start;
                    attached = value_start < argument.size() ? argument.substr(value_start)
                                                             : std::string();
                }
                if (process(*option, attached, index, arguments))
                    return;
                break;
            }
            continue;
        }
        record_positional(argument);
    }

    for (const auto &option : options_) {
        if (option.required && !provided_[option.name])
            throw std::invalid_argument("required option not found: " + display_name(option));
    }
    if (positional_ && positional_->required && positional_values_.empty())
        throw std::invalid_argument("required argument not found: " + positional_->name);
}

bool CommandLine::was_provided(const std::string &name) const
{
    const auto found = provided_.find(name);
    return found != provided_.end() && found->second;
}

bool CommandLine::switch_value(const std::string &name) const
{
    return was_provided(name);
}

std::size_t CommandLine::count(const std::string &name) const
{
    const auto found = counts_.find(name);
    return found == counts_.end() ? 0 : found->second;
}

std::optional<std::string> CommandLine::value(const std::string &name) const
{
    const auto found = values_.find(name);
    if (found == values_.end() || found->second.empty())
        return std::nullopt;
    return found->second.back();
}

const std::vector<std::string> &CommandLine::values(const std::string &name) const
{
    static const std::vector<std::string> empty;
    const auto found = values_.find(name);
    return found == values_.end() ? empty : found->second;
}

bool CommandLine::contains(const std::string &name, const std::string &item) const
{
    const auto &items = values(name);
    return std::find(items.begin(), items.end(), item) != items.end();
}

const std::vector<std::string> &CommandLine::positional_values() const noexcept
{
    return positional_values_;
}

const std::vector<CommandLineEvent> &CommandLine::events() const noexcept
{
    return events_;
}

CommandLineAction CommandLine::action() const noexcept
{
    return action_;
}

std::string CommandLine::help_text() const
{
    std::ostringstream output;
    output << "Usage: " << program_name_ << " [options]";
    if (positional_) {
        output << (positional_->required ? " <" : " [") << positional_->name;
        if (positional_->list)
            output << "...";
        output << (positional_->required ? '>' : ']');
    }
    output << "\n\n" << description_ << "\n\nOptions:\n";
    for (const auto &option : options_) {
        output << "  ";
        if (option.single_dash) {
            if (option.short_name != '\0')
                output << '-' << option.short_name << ", ";
            output << '-' << option.long_name;
        } else {
            if (option.short_name != '\0')
                output << '-' << option.short_name;
            else
                output << "  ";
            if (!option.long_name.empty()) {
                if (option.short_name != '\0')
                    output << ", ";
                else
                    output << "  ";
                output << "--" << option.long_name;
            }
        }
        if ((option.value_type == CommandLineValueType::String
             || option.value_type == CommandLineValueType::Number)
            && option.action == CommandLineAction::None)
            output << (option.list ? " <value>..." : " <value>");
        output << "\t" << option.description;
        if (option.value_type == CommandLineValueType::Counter)
            output << " (repeatable)";
        if (option.default_value)
            output << " (default: " << *option.default_value << ')';
        if (option.required)
            output << " (required)";
        output << '\n';
    }
    if (positional_) {
        output << "  " << positional_->name << "\t" << positional_->description;
        if (positional_->required)
            output << " (required)";
        output << '\n';
    }
    return output.str();
}

std::string CommandLine::version_text() const
{
    return program_name_ + ": v" + version_ + '\n';
}

}
