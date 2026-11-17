#include "smallcpp/CommandLine.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const std::string &message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void expect_failure(const std::function<void()> &action, const std::string &expected)
{
    try {
        action();
    } catch (const std::invalid_argument &error) {
        require(std::string(error.what()).find(expected) != std::string::npos,
                "expected error containing '" + expected + "', got: " + error.what());
        return;
    }
    throw std::runtime_error("expected command-line parse failure: " + expected);
}

smallcpp::CommandLine make_command_line()
{
    smallcpp::CommandLine command_line("test", "test options", "1.2");
    smallcpp::CommandLineOption alpha;
    alpha.short_name = 'a';
    alpha.long_name = "alpha";
    alpha.name = "alpha";
    command_line.add_option(alpha);

    smallcpp::CommandLineOption beta;
    beta.short_name = 'b';
    beta.long_name = "beta";
    beta.name = "beta";
    command_line.add_option(beta);

    smallcpp::CommandLineOption path;
    path.short_name = 'p';
    path.long_name = "path";
    path.name = "path";
    path.value_type = smallcpp::CommandLineValueType::String;
    path.list = true;
    command_line.add_option(path);

    smallcpp::CommandLineOption output;
    output.short_name = 'o';
    output.long_name = "output";
    output.name = "output";
    output.value_type = smallcpp::CommandLineValueType::String;
    output.default_value = "default.out";
    command_line.add_option(output);

    smallcpp::CommandLineOption count;
    count.long_name = "count";
    count.name = "count";
    count.value_type = smallcpp::CommandLineValueType::Number;
    count.required = true;
    command_line.add_option(count);

    smallcpp::CommandLineOption help;
    help.long_name = "help";
    help.name = "help";
    help.action = smallcpp::CommandLineAction::Help;
    command_line.add_option(help);
    command_line.add_positional({"source", "source file", true, true});
    return command_line;
}

void test_groups()
{
    auto command_line = make_command_line();
    command_line.parse({"-ab", "-p", "one:two", "-pthree", "-o=first",
                        "--output=last", "--count", "-5", "first.scpp",
                        "--", "-input.scpp"});
    require(command_line.switch_value("alpha") && command_line.switch_value("beta"),
            "short option groups should set each switch");
    require(command_line.values("path") == std::vector<std::string>{"one", "two", "three"},
            "repeated list options and colon-separated values should accumulate");
    require(command_line.value("output") == std::optional<std::string>("last"),
            "the last scalar option should replace previous values");
    require(command_line.was_provided("output"),
            "overridden default values should be marked as explicitly provided");
    require(command_line.value("count") == std::optional<std::string>("-5"),
            "separate option values may begin with a dash");
    require(command_line.positional_values()
                == std::vector<std::string>{"first.scpp", "-input.scpp"},
            "list positionals should preserve each complete path");
}

void test_defaults()
{
    auto command_line = make_command_line();
    command_line.parse({"--help"});
    require(command_line.action() == smallcpp::CommandLineAction::Help,
            "help should stop parsing before required values are checked");
    require(command_line.value("output") == std::optional<std::string>("default.out"),
            "registered defaults should be available after parsing");
    require(!command_line.was_provided("output"),
            "defaults should not count as command-line occurrences");
    require(command_line.help_text().find("--count <value>") != std::string::npos,
            "help should show value-taking options");
    require(command_line.version_text() == "test: v1.2\n",
            "version text should include the registered application version");
}

void test_invalid()
{
    auto command_line = make_command_line();
    expect_failure([&] { command_line.parse({"source.scpp"}); }, "required option");
    command_line.parse({"--count", "1", "source.scpp", "other.scpp"});
    require(command_line.positional_values()
                == std::vector<std::string>{"source.scpp", "other.scpp"},
            "list positionals should accept multiple input files");
    expect_failure([&] { command_line.parse({"--unknown", "--count", "1", "x"}); },
                   "unknown option");
    expect_failure([&] { command_line.parse({"--output"}); }, "requires an argument");
    expect_failure([&] { command_line.parse({"--alpha=yes", "--count", "1", "x"}); },
                   "does not accept an argument");
    expect_failure([&] { command_line.parse({"--count", "many", "x"}); }, "requires a number");
}

void test_counters()
{
    smallcpp::CommandLine command_line("test", "test options", "1.2");
    smallcpp::CommandLineOption verbose;
    verbose.short_name = 'v';
    verbose.name = "verbose";
    verbose.value_type = smallcpp::CommandLineValueType::Counter;
    command_line.add_option(verbose);
    smallcpp::CommandLineOption quiet;
    quiet.short_name = 'q';
    quiet.name = "quiet";
    command_line.add_option(quiet);
    smallcpp::CommandLineOption trace;
    trace.short_name = 't';
    trace.long_name = "trace";
    trace.name = "trace";
    trace.value_type = smallcpp::CommandLineValueType::String;
    trace.list = true;
    trace.list_separator = ',';
    command_line.add_option(trace);

    command_line.parse({"-vv", "-qv", "-t", "a,b", "--trace=c:d", "-te"});
    require(command_line.count("verbose") == 3,
            "counted switches should count grouped and repeated occurrences");
    require(command_line.switch_value("quiet"), "counters should combine with switches");
    require(command_line.values("trace") == std::vector<std::string>{"a", "b", "c:d", "e"},
            "list options should split only on their own separator");
    require(command_line.help_text().find("(repeatable)") != std::string::npos,
            "help should mark counted switches as repeatable");
    command_line.parse({});
    require(command_line.count("verbose") == 0, "counts should reset between parses");
    expect_failure([&] { command_line.parse({"-v=2"}); }, "does not accept an argument");

    smallcpp::CommandLine invalid("test", "test options", "1.2");
    smallcpp::CommandLineOption list_counter;
    list_counter.short_name = 'c';
    list_counter.name = "count";
    list_counter.value_type = smallcpp::CommandLineValueType::Counter;
    list_counter.list = true;
    expect_failure([&] { invalid.add_option(list_counter); }, "list");
}

}

int main()
{
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"short groups, attached values, and lists", test_groups},
        {"defaults and actions", test_defaults},
        {"invalid arguments", test_invalid},
        {"counters and list separators", test_counters},
    };
    for (const auto &test : tests) {
        try {
            test.second();
        } catch (const std::exception &error) {
            std::cerr << "FAIL " << test.first << ": " << error.what() << '\n';
            return 1;
        }
    }
    std::cout << "All command-line parser tests passed\n";
    return 0;
}
