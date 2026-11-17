/*
 * Standalone C preprocessor.
 *
 * Each pass reads the current tokens, handles directives, and expands each
 * macro once. The replacement is not expanded again until the next pass, so
 * a name formed by ## and a macro a definition depends on are expanded after
 * the macros they come from. Definitions persist across passes. A #define
 * produced by a macro is a directive on the following pass. Passes stop when
 * the text stops changing. A pass limit rejects an expansion that does not
 * finish.
 */
#include "process.hpp"

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

extern "C" char **environ;

namespace smallcpp {
namespace {
namespace fs = std::filesystem;

constexpr int kPassLimit = 128;

enum class Kind { Ident, Number, String, Char, Punct, Other, Newline };

struct Tok {
    Kind kind = Kind::Other;
    std::string text;
    bool space_before = false;
    int line = 1;
    int column = 1;
    std::string file;
};

struct Macro {
    bool function = false;
    bool from_env = false;
    std::vector<std::string> params;
    std::vector<Tok> body;
};

struct Cursor {
    const std::vector<Tok> *in = nullptr;
    std::size_t i = 0;
    int line = 1;
    bool bol = true;
    std::string file;
    std::string physical;
};

struct Cond {
    bool parent = false;
    bool any = false;
    bool now = false;
    bool saw_else = false;
};

class Engine {
public:
    explicit Engine(PreprocRequest request) : request_(std::move(request)) {}

    std::string run()
    {
        load_lines(request_.input);
        if (!request_.ignore_environment)
            import_environment();
        for (const CliMacro &item : request_.cli_macros) {
            if (item.undef)
                command_undef(item.text);
            else
                install_command_define(item.text);
        }
        std::error_code canon_error;
        const fs::path canon = fs::canonical(request_.input, canon_error);
        if (!canon_error)
            include_stack_.push_back(canon.string());
        std::vector<Tok> tokens;
        for (const std::string &file : request_.force_includes) {
            const std::string path = resolve_force_include(file);
            std::vector<Tok> part = lex_file(path);
            tokens.insert(tokens.end(), part.begin(), part.end());
        }
        std::vector<Tok> main_tokens = lex_file(request_.input);
        tokens.insert(tokens.end(), main_tokens.begin(), main_tokens.end());
        std::string previous;
        for (int pass = 0; pass < kPassLimit; ++pass) {
            std::vector<Tok> next = pass_tokens(tokens, request_.input, 1);
            std::string rendered = render(next);
            if (pass > 0 && rendered == previous)
                return rendered.empty() || rendered.back() == '\n' ? rendered : rendered + '\n';
            previous = rendered;
            tokens = std::move(next);
        }
        fail(request_.input, 1, 1, "expansion did not finish");
    }

private:
    PreprocRequest request_;
    std::unordered_map<std::string, Macro> macros_;
    std::unordered_map<std::string, std::vector<std::string>> lines_;
    std::vector<std::string> include_stack_;

    [[noreturn]] void fail(const std::string &file, int line, int column, const std::string &message) const
    {
        std::ostringstream out;
        out << file << ':' << line << ':' << column << ": " << message << '\n';
        const std::string source = source_line(file, line);
        if (!source.empty())
            out << source << '\n';
        throw std::runtime_error(out.str());
    }

    [[noreturn]] void fail(const Tok &tok, const std::string &message) const
    {
        fail(tok.file.empty() ? request_.input : tok.file, tok.line, tok.column, message);
    }

    void load_lines(const std::string &path)
    {
        if (lines_.count(path) != 0)
            return;
        std::ifstream in(path, std::ios::binary);
        if (!in)
            fail(path, 1, 1, "cannot read file");
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(line);
        }
        lines_[path] = std::move(lines);
    }

    std::string source_line(const std::string &file, int line) const
    {
        const auto found = lines_.find(file);
        if (found == lines_.end() || line < 1 || static_cast<std::size_t>(line) > found->second.size())
            return {};
        return found->second[static_cast<std::size_t>(line) - 1];
    }

    static bool is_ident_start(unsigned char c)
    {
        return std::isalpha(c) != 0 || c == '_';
    }

    static bool is_ident_char(unsigned char c)
    {
        return std::isalnum(c) != 0 || c == '_';
    }

    static const char *match_punct(const std::string &text, std::size_t i)
    {
        static const char *ops[] = {
            "...", "<<=", ">>=", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
            "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "##",
            "->", "#", "(", ")", ",", "{", "}", "[", "]", ".", "&", "*", "+",
            "-", "~", "!", "/", "%", "<", ">", "^", "|", "?", ":", ";", "=", nullptr};
        for (const char **op = ops; *op != nullptr; ++op) {
            std::size_t n = std::char_traits<char>::length(*op);
            if (i + n <= text.size() && text.compare(i, n, *op) == 0)
                return *op;
        }
        return nullptr;
    }

    std::vector<Tok> lex(const std::string &text, const std::string &file) const
    {
        std::string spliced;
        spliced.reserve(text.size());
        std::vector<int> line_of;
        std::vector<int> col_of;
        int line = 1;
        int column = 1;
        for (std::size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\\' && i + 1 < text.size()
                && (text[i + 1] == '\n' || text[i + 1] == '\r')) {
                if (text[i + 1] == '\r')
                    ++i;
                if (i + 1 < text.size() && text[i + 1] == '\n' && text[i] == '\r')
                    ++i;
                ++i;
                ++line;
                column = 1;
                continue;
            }
            spliced.push_back(text[i]);
            line_of.push_back(line);
            col_of.push_back(column);
            if (text[i] == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
        }

        std::vector<Tok> out;
        bool spaced = false;
        auto push = [&](Kind kind, std::string value, std::size_t at) {
            Tok tok;
            tok.kind = kind;
            tok.text = std::move(value);
            tok.space_before = spaced && kind != Kind::Newline;
            tok.line = at < line_of.size() ? line_of[at] : line;
            tok.column = at < col_of.size() ? col_of[at] : 1;
            tok.file = file;
            if (kind == Kind::Newline)
                tok.space_before = false;
            out.push_back(std::move(tok));
            spaced = false;
        };

        for (std::size_t i = 0; i < spliced.size();) {
            unsigned char c = static_cast<unsigned char>(spliced[i]);
            if (c == ' ' || c == '\t' || c == '\v' || c == '\f') {
                spaced = true;
                ++i;
                continue;
            }
            if (c == '\r') {
                if (i + 1 < spliced.size() && spliced[i + 1] == '\n')
                    ++i;
                push(Kind::Newline, "\n", i);
                ++i;
                continue;
            }
            if (c == '\n') {
                push(Kind::Newline, "\n", i);
                ++i;
                continue;
            }
            if (c == '/' && i + 1 < spliced.size() && spliced[i + 1] == '/') {
                i += 2;
                while (i < spliced.size() && spliced[i] != '\n' && spliced[i] != '\r')
                    ++i;
                spaced = true;
                continue;
            }
            if (c == '/' && i + 1 < spliced.size() && spliced[i + 1] == '*') {
                std::size_t start = i;
                i += 2;
                bool closed = false;
                while (i + 1 < spliced.size()) {
                    if (spliced[i] == '*' && spliced[i + 1] == '/') {
                        i += 2;
                        closed = true;
                        break;
                    }
                    if (spliced[i] == '\n')
                        push(Kind::Newline, "\n", i);
                    ++i;
                }
                if (!closed)
                    fail(file, line_of[start], col_of[start], "unterminated comment");
                spaced = true;
                continue;
            }
            if (c == '"' || c == '\'') {
                std::size_t start = i++;
                bool closed = false;
                while (i < spliced.size()) {
                    if (spliced[i] == '\\') {
                        if (i + 1 >= spliced.size())
                            break;
                        i += 2;
                        continue;
                    }
                    if (spliced[i] == '\n')
                        break;
                    if (spliced[i] == spliced[start]) {
                        ++i;
                        closed = true;
                        break;
                    }
                    ++i;
                }
                if (!closed)
                    fail(file, line_of[start], col_of[start],
                         c == '"' ? "unterminated string" : "unterminated character constant");
                push(c == '"' ? Kind::String : Kind::Char, spliced.substr(start, i - start), start);
                continue;
            }
            if (c == '.' && i + 1 < spliced.size() && std::isdigit(static_cast<unsigned char>(spliced[i + 1]))) {
                std::size_t start = i;
                std::string number = scan_number(spliced, i);
                push(Kind::Number, std::move(number), start);
                continue;
            }
            if (std::isdigit(c) != 0) {
                std::size_t start = i;
                std::string number = scan_number(spliced, i);
                push(Kind::Number, std::move(number), start);
                continue;
            }
            if (is_ident_start(c)) {
                std::size_t start = i++;
                while (i < spliced.size() && is_ident_char(static_cast<unsigned char>(spliced[i])))
                    ++i;
                push(Kind::Ident, spliced.substr(start, i - start), start);
                continue;
            }
            if (const char *op = match_punct(spliced, i)) {
                std::size_t n = std::char_traits<char>::length(op);
                push(Kind::Punct, op, i);
                i += n;
                continue;
            }
            if (c == '\\')
                fail(file, line_of[i], col_of[i], "stray backslash");
            push(Kind::Other, spliced.substr(i, 1), i);
            ++i;
        }
        return out;
    }

    static std::string scan_number(const std::string &text, std::size_t &i)
    {
        std::size_t start = i++;
        while (i < text.size()) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            if (std::isalnum(c) != 0 || c == '_' || c == '.') {
                ++i;
                continue;
            }
            if ((c == '+' || c == '-') && i > start) {
                char prev = text[i - 1];
                if (prev == 'e' || prev == 'E' || prev == 'p' || prev == 'P') {
                    ++i;
                    continue;
                }
            }
            break;
        }
        return text.substr(start, i - start);
    }

    std::vector<Tok> lex_file(const std::string &path)
    {
        load_lines(path);
        std::ifstream in(path, std::ios::binary);
        if (!in)
            fail(path, 1, 1, "cannot read file");
        std::ostringstream buffer;
        buffer << in.rdbuf();
        return lex(buffer.str(), path);
    }

    static std::string render(const std::vector<Tok> &tokens)
    {
        std::string out;
        for (const Tok &tok : tokens) {
            if (tok.kind == Kind::Newline) {
                out.push_back('\n');
                continue;
            }
            if (!out.empty() && out.back() != '\n' && tok.space_before)
                out.push_back(' ');
            out += tok.text;
        }
        return out;
    }

    static bool same_body(const Macro &left, const Macro &right)
    {
        if (left.function != right.function || left.params != right.params
            || left.body.size() != right.body.size())
            return false;
        for (std::size_t i = 0; i < left.body.size(); ++i) {
            if (left.body[i].kind != right.body[i].kind || left.body[i].text != right.body[i].text)
                return false;
            if (i > 0 && left.body[i].space_before != right.body[i].space_before)
                return false;
        }
        return true;
    }

    void define_macro(Macro macro, const Tok &site)
    {
        if (macro.params.size() > 1) {
            for (std::size_t i = 0; i < macro.params.size(); ++i) {
                if (macro.params[i].empty())
                    fail(site, "a macro parameter has no name");
                for (std::size_t j = 0; j < i; ++j) {
                    if (macro.params[i] == macro.params[j])
                        fail(site, "duplicate macro parameter: " + macro.params[i]);
                }
            }
        }
        if (!macro.body.empty()) {
            const Tok &first = macro.body.front();
            const Tok &last = macro.body.back();
            if ((first.kind == Kind::Punct && first.text == "##")
                || (last.kind == Kind::Punct && last.text == "##"))
                fail(site, "'##' is at the end of a macro");
            if (macro.function) {
                for (std::size_t i = 0; i < macro.body.size(); ++i) {
                    if (macro.body[i].kind != Kind::Punct || macro.body[i].text != "#")
                        continue;
                    std::size_t index = 0;
                    if (i + 1 >= macro.body.size() || !param_index(macro, macro.body[i + 1], index))
                        fail(site, "'#' is not followed by a parameter");
                }
            }
        }
        const std::string name = site.text;
        if (name == "defined" || name == "__LINE__" || name == "__FILE__")
            fail(site, "cannot define " + name);
        auto found = macros_.find(name);
        if (found != macros_.end()) {
            if (found->second.from_env && !macro.from_env)
                macros_.erase(found);
            else if (!same_body(found->second, macro))
                fail(site, "macro is already defined: " + name);
            else
                return;
        }
        if (!macro.body.empty())
            macro.body.front().space_before = false;
        macros_[name] = std::move(macro);
    }

    static bool param_index(const Macro &macro, const Tok &tok, std::size_t &index)
    {
        if (tok.kind != Kind::Ident)
            return false;
        for (std::size_t i = 0; i < macro.params.size(); ++i) {
            if (macro.params[i] == tok.text) {
                index = i;
                return true;
            }
        }
        return false;
    }

    void install_command_define(const std::string &text)
    {
        const auto eq = text.find('=');
        const std::string name = eq == std::string::npos ? text : text.substr(0, eq);
        const std::string body = eq == std::string::npos ? "1" : text.substr(eq + 1);
        if (!valid_ident(name))
            fail(request_.input, 1, 1, "invalid -D name");
        Tok site;
        site.kind = Kind::Ident;
        site.text = name;
        site.file = request_.input;
        site.line = 1;
        site.column = 1;
        Macro macro;
        macro.body = replacement_tokens(body);
        define_macro(std::move(macro), site);
    }

    bool valid_ident(const std::string &name) const
    {
        if (name.empty() || !is_ident_start(static_cast<unsigned char>(name[0])))
            return false;
        for (unsigned char c : name) {
            if (!is_ident_char(c))
                return false;
        }
        return true;
    }

    std::vector<Tok> replacement_tokens(const std::string &body)
    {
        if (body.empty())
            return {};
        std::vector<Tok> tokens = lex(body, request_.input);
        for (auto it = tokens.begin(); it != tokens.end();) {
            if (it->kind == Kind::Newline)
                it = tokens.erase(it);
            else
                ++it;
        }
        return tokens;
    }

    void command_undef(const std::string &name)
    {
        if (!valid_ident(name))
            fail(request_.input, 1, 1, "invalid -U name");
        if (name == "__LINE__" || name == "__FILE__")
            fail(request_.input, 1, 1, "cannot undefine " + name);
        macros_.erase(name);
    }

    void import_environment()
    {
        if (environ == nullptr)
            return;
        for (char **entry = environ; *entry != nullptr; ++entry) {
            const std::string text(*entry);
            const auto eq = text.find('=');
            if (eq == std::string::npos || eq == 0)
                continue;
            const std::string name = text.substr(0, eq);
            if (!valid_ident(name))
                continue;
            Tok site;
            site.kind = Kind::Ident;
            site.text = name;
            site.file = request_.input;
            site.line = 1;
            site.column = 1;
            Macro macro;
            macro.from_env = true;
            try {
                macro.body = replacement_tokens(text.substr(eq + 1));
            } catch (const std::runtime_error &) {
                continue;
            }
            define_macro(std::move(macro), site);
        }
    }

    Tok make_number(long long value, const Tok &site) const
    {
        Tok tok;
        tok.kind = Kind::Number;
        tok.text = std::to_string(value);
        tok.line = site.line;
        tok.column = site.column;
        tok.file = site.file;
        return tok;
    }

    Tok make_string(const std::string &value, const Tok &site) const
    {
        Tok tok;
        tok.kind = Kind::String;
        std::string escaped = "\"";
        for (char c : value) {
            if (c == '\\' || c == '"')
                escaped.push_back('\\');
            escaped.push_back(c);
        }
        escaped.push_back('"');
        tok.text = std::move(escaped);
        tok.line = site.line;
        tok.column = site.column;
        tok.file = site.file;
        return tok;
    }

    static std::string escape_spelling(const std::string &spelling)
    {
        std::string escaped;
        for (char c : spelling) {
            if (c == '\\' || c == '"')
                escaped.push_back('\\');
            escaped.push_back(c);
        }
        return "\"" + escaped + "\"";
    }

    Tok stringify(const std::vector<Tok> &arg, const Tok &site) const
    {
        std::string spelling;
        bool any = false;
        for (const Tok &tok : arg) {
            if (tok.kind == Kind::Newline)
                continue;
            if (any && tok.space_before)
                spelling.push_back(' ');
            any = true;
            spelling += tok.text;
        }
        Tok tok = site;
        tok.kind = Kind::String;
        tok.text = escape_spelling(spelling);
        tok.space_before = false;
        return tok;
    }

    Tok paste_tokens(const Tok &left, const Tok &right) const
    {
        if (left.text.empty())
            return right;
        if (right.text.empty())
            return left;
        const std::string joined = left.text + right.text;
        std::vector<Tok> parts = lex(joined, left.file.empty() ? right.file : left.file);
        std::vector<Tok> real;
        for (const Tok &tok : parts) {
            if (tok.kind != Kind::Newline)
                real.push_back(tok);
        }
        if (real.size() != 1 || real[0].text != joined)
            fail(left.text.empty() ? right : left, "'##' does not form one token");
        real[0].space_before = left.space_before;
        real[0].line = left.line != 0 ? left.line : right.line;
        real[0].column = left.column != 0 ? left.column : right.column;
        real[0].file = !left.file.empty() ? left.file : right.file;
        return real[0];
    }

    std::vector<Tok> expand_once(const std::vector<Tok> &in, int line, const std::string &file)
    {
        Cursor cursor;
        cursor.in = &in;
        cursor.line = line;
        cursor.file = file;
        cursor.bol = false;
        std::vector<Tok> out;
        while (cursor.i < in.size()) {
            if (in[cursor.i].kind == Kind::Newline) {
                out.push_back(in[cursor.i]);
                ++cursor.i;
                ++cursor.line;
                continue;
            }
            std::size_t consumed = 0;
            if (expand_name(cursor, out, consumed)) {
                cursor.i += consumed;
                continue;
            }
            out.push_back(in[cursor.i]);
            ++cursor.i;
        }
        return out;
    }

    std::vector<Tok> expand_fully(std::vector<Tok> tokens, const Tok &site)
    {
        for (int pass = 0; pass < kPassLimit; ++pass) {
            std::vector<Tok> next = expand_once(tokens, site.line, site.file);
            if (render(next) == render(tokens))
                return next;
            tokens = std::move(next);
        }
        fail(site, "expansion did not finish");
    }

    bool followed_by_paste(const std::vector<Tok> &body, std::size_t index) const
    {
        return index + 1 < body.size() && body[index + 1].kind == Kind::Punct
            && body[index + 1].text == "##";
    }

    std::vector<Tok> operand(const Macro &macro, const Tok &tok,
                             const std::vector<std::vector<Tok>> &raw, const Tok &site,
                             bool expand)
    {
        std::size_t index = 0;
        if (param_index(macro, tok, index))
            return expand ? expand_once(raw[index], site.line, site.file) : raw[index];
        if (tok.kind == Kind::Ident && tok.text == "__LINE__")
            return {make_number(site.line, site)};
        if (tok.kind == Kind::Ident && tok.text == "__FILE__")
            return {make_string(site.file, site)};
        return {tok};
    }

    static std::vector<Tok> join_paste(const std::vector<Tok> &left, const std::vector<Tok> &right,
                                       const Engine *engine)
    {
        if (left.empty())
            return right;
        if (right.empty())
            return left;
        std::vector<Tok> out = left;
        Tok pasted = engine->paste_tokens(out.back(), right.front());
        out.pop_back();
        out.push_back(pasted);
        out.insert(out.end(), right.begin() + 1, right.end());
        return out;
    }

    std::vector<Tok> substitute(const Macro &macro, const std::vector<std::vector<Tok>> &raw,
                                const Tok &site)
    {
        std::vector<Tok> out;
        bool pending = false;
        std::vector<Tok> pending_left;
        bool pending_space = false;
        auto emit = [&](std::vector<Tok> piece, bool space) {
            if (piece.empty())
                return;
            piece.front().space_before = space;
            out.insert(out.end(), piece.begin(), piece.end());
        };
        for (std::size_t i = 0; i < macro.body.size();) {
            const Tok &tok = macro.body[i];
            if (tok.kind == Kind::Punct && tok.text == "##") {
                if (!pending || i + 1 >= macro.body.size())
                    fail(site, "'##' does not form one token");
                std::vector<Tok> right = operand(macro, macro.body[i + 1], raw, site, false);
                std::vector<Tok> joined = join_paste(pending_left, right, this);
                pending = false;
                i += 2;
                if (i < macro.body.size() && macro.body[i].kind == Kind::Punct && macro.body[i].text == "##") {
                    pending = true;
                    pending_left = std::move(joined);
                    continue;
                }
                emit(std::move(joined), pending_space);
                continue;
            }
            if (followed_by_paste(macro.body, i)) {
                if (pending)
                    fail(site, "'##' does not form one token");
                pending = true;
                pending_space = tok.space_before;
                pending_left = operand(macro, tok, raw, site, false);
                ++i;
                continue;
            }
            if (macro.function && tok.kind == Kind::Punct && tok.text == "#") {
                std::size_t index = 0;
                if (i + 1 >= macro.body.size() || !param_index(macro, macro.body[i + 1], index))
                    fail(site, "'#' is not followed by a parameter");
                Tok text = stringify(raw[index], site);
                text.space_before = tok.space_before;
                out.push_back(text);
                i += 2;
                continue;
            }
            std::vector<Tok> bits = operand(macro, tok, raw, site, true);
            emit(std::move(bits), tok.space_before);
            ++i;
        }
        if (pending)
            fail(site, "'##' does not form one token");
        if (!out.empty())
            out.front().space_before = site.space_before;
        return out;
    }

    bool expand_name(Cursor &cursor, std::vector<Tok> &out, std::size_t &consumed)
    {
        const std::vector<Tok> &in = *cursor.in;
        const Tok &tok = in[cursor.i];
        if (tok.kind != Kind::Ident)
            return false;
        if (tok.text == "__LINE__") {
            Tok number = make_number(cursor.line, tok);
            number.space_before = tok.space_before;
            out.push_back(number);
            consumed = 1;
            return true;
        }
        if (tok.text == "__FILE__") {
            Tok text = make_string(cursor.file, tok);
            text.space_before = tok.space_before;
            out.push_back(text);
            consumed = 1;
            return true;
        }
        const auto found = macros_.find(tok.text);
        if (found == macros_.end())
            return false;
        const Macro &macro = found->second;
        Tok site = tok;
        site.line = cursor.line;
        site.file = cursor.file;
        if (!macro.function) {
            std::vector<std::vector<Tok>> none;
            std::vector<Tok> body = substitute(macro, none, site);
            out.insert(out.end(), body.begin(), body.end());
            consumed = 1;
            return true;
        }
        std::size_t paren = cursor.i + 1;
        int line = cursor.line;
        while (paren < in.size() && in[paren].kind == Kind::Newline) {
            ++paren;
            ++line;
        }
        if (paren >= in.size() || in[paren].kind != Kind::Punct || in[paren].text != "(")
            return false;

        std::vector<std::vector<Tok>> args;
        std::vector<Tok> current;
        bool saw_comma = false;
        bool saw_token = false;
        int depth = 0;
        std::size_t p = paren + 1;
        int scan_line = line;
        bool arg_space = false;
        bool closed = false;
        while (p < in.size()) {
            const Tok &at = in[p];
            if (at.kind == Kind::Newline) {
                ++scan_line;
                arg_space = true;
                ++p;
                continue;
            }
            if (depth == 0 && at.kind == Kind::Punct && at.text == ")") {
                closed = true;
                break;
            }
            if (depth == 0 && at.kind == Kind::Punct && at.text == ",") {
                args.push_back(current);
                current.clear();
                saw_comma = true;
                arg_space = false;
                ++p;
                continue;
            }
            if (at.kind == Kind::Punct && (at.text == "(" || at.text == "[" || at.text == "{"))
                ++depth;
            else if (at.kind == Kind::Punct && (at.text == ")" || at.text == "]" || at.text == "}")) {
                if (depth == 0)
                    fail(at, "unbalanced delimiter in a macro argument");
                --depth;
            }
            Tok copy = at;
            if (arg_space)
                copy.space_before = true;
            arg_space = false;
            current.push_back(copy);
            saw_token = true;
            ++p;
        }
        if (!closed)
            fail(tok, "unclosed macro argument list");
        if (!saw_token && !saw_comma) {
            if (macro.params.size() == 1)
                args.push_back({});
            else if (!macro.params.empty())
                fail(tok, "wrong number of macro arguments");
        } else {
            args.push_back(current);
        }
        if (args.size() != macro.params.size())
            fail(tok, "wrong number of macro arguments");
        std::vector<Tok> body = substitute(macro, args, site);
        out.insert(out.end(), body.begin(), body.end());
        cursor.line = scan_line;
        consumed = (p + 1) - cursor.i;
        return true;
    }

    static void replace_defined(std::vector<Tok> &tokens, const std::unordered_map<std::string, Macro> &macros,
                                const Tok &site, const Engine *engine)
    {
        std::vector<Tok> out;
        for (std::size_t i = 0; i < tokens.size();) {
            if (tokens[i].kind == Kind::Ident && tokens[i].text == "defined") {
                std::size_t j = i + 1;
                std::string name;
                std::size_t next = j;
                if (j < tokens.size() && tokens[j].kind == Kind::Punct && tokens[j].text == "(") {
                    if (j + 2 >= tokens.size() || tokens[j + 1].kind != Kind::Ident
                        || tokens[j + 2].kind != Kind::Punct || tokens[j + 2].text != ")")
                        engine->fail(site, "invalid defined");
                    name = tokens[j + 1].text;
                    next = j + 3;
                } else if (j < tokens.size() && tokens[j].kind == Kind::Ident) {
                    name = tokens[j].text;
                    next = j + 1;
                } else {
                    engine->fail(site, "invalid defined");
                }
                Tok number = tokens[i];
                number.kind = Kind::Number;
                number.text = macros.count(name) != 0 ? "1" : "0";
                number.space_before = tokens[i].space_before;
                out.push_back(number);
                i = next;
                continue;
            }
            out.push_back(tokens[i]);
            ++i;
        }
        tokens = std::move(out);
    }

    long long eval_expr(const std::vector<Tok> &tokens, const Tok &site) const
    {
        std::size_t i = 0;
        long long value = parse_or(tokens, i, site);
        if (i != tokens.size())
            fail(site, "extra tokens in #if");
        return value;
    }

    long long parse_or(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        long long left = parse_and(t, i, site);
        while (i < t.size() && t[i].text == "||") {
            ++i;
            long long right = parse_and(t, i, site);
            left = (left != 0 || right != 0) ? 1 : 0;
        }
        return left;
    }

    long long parse_and(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        long long left = parse_bor(t, i, site);
        while (i < t.size() && t[i].text == "&&") {
            ++i;
            long long right = parse_bor(t, i, site);
            left = (left != 0 && right != 0) ? 1 : 0;
        }
        return left;
    }

    long long parse_bor(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        long long left = parse_bxor(t, i, site);
        while (i < t.size() && t[i].text == "|") {
            ++i;
            left = checked(static_cast<__int128>(left) | parse_bxor(t, i, site), site);
        }
        return left;
    }

    long long parse_bxor(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        long long left = parse_band(t, i, site);
        while (i < t.size() && t[i].text == "^") {
            ++i;
            left = checked(static_cast<__int128>(left) ^ parse_band(t, i, site), site);
        }
        return left;
    }

    long long parse_band(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        long long left = parse_eq(t, i, site);
        while (i < t.size() && t[i].text == "&") {
            ++i;
            left = checked(static_cast<__int128>(left) & parse_eq(t, i, site), site);
        }
        return left;
    }

    long long parse_eq(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        long long left = parse_rel(t, i, site);
        while (i < t.size() && (t[i].text == "==" || t[i].text == "!=")) {
            const std::string op = t[i].text;
            ++i;
            long long right = parse_rel(t, i, site);
            left = (op == "==" ? left == right : left != right) ? 1 : 0;
        }
        return left;
    }

    long long parse_rel(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        long long left = parse_shift(t, i, site);
        while (i < t.size()
               && (t[i].text == "<" || t[i].text == ">" || t[i].text == "<=" || t[i].text == ">=")) {
            const std::string op = t[i].text;
            ++i;
            long long right = parse_shift(t, i, site);
            bool result = false;
            if (op == "<")
                result = left < right;
            else if (op == ">")
                result = left > right;
            else if (op == "<=")
                result = left <= right;
            else
                result = left >= right;
            left = result ? 1 : 0;
        }
        return left;
    }

    long long parse_shift(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        long long left = parse_add(t, i, site);
        while (i < t.size() && (t[i].text == "<<" || t[i].text == ">>")) {
            const std::string op = t[i].text;
            ++i;
            long long right = parse_add(t, i, site);
            if (right < 0 || right >= 63)
                fail(site, "invalid shift");
            if (op == "<<")
                left = checked(static_cast<__int128>(left) << right, site);
            else
                left = left >> right;
        }
        return left;
    }

    long long parse_add(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        long long left = parse_mul(t, i, site);
        while (i < t.size() && (t[i].text == "+" || t[i].text == "-")) {
            const std::string op = t[i].text;
            ++i;
            long long right = parse_mul(t, i, site);
            if (op == "+")
                left = checked(static_cast<__int128>(left) + right, site);
            else
                left = checked(static_cast<__int128>(left) - right, site);
        }
        return left;
    }

    long long parse_mul(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        long long left = parse_unary(t, i, site);
        while (i < t.size() && (t[i].text == "*" || t[i].text == "/" || t[i].text == "%")) {
            const std::string op = t[i].text;
            ++i;
            long long right = parse_unary(t, i, site);
            if (op == "*")
                left = checked(static_cast<__int128>(left) * right, site);
            else if (right == 0)
                fail(site, "division by zero");
            else if (left == INT64_MIN && right == -1)
                fail(site, "integer overflow");
            else if (op == "/")
                left = left / right;
            else
                left = left % right;
        }
        return left;
    }

    long long parse_unary(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        if (i < t.size() && (t[i].text == "+" || t[i].text == "-" || t[i].text == "!" || t[i].text == "~")) {
            const std::string op = t[i].text;
            ++i;
            long long value = parse_unary(t, i, site);
            if (op == "+")
                return value;
            if (op == "-")
                return checked(-static_cast<__int128>(value), site);
            if (op == "!")
                return value == 0 ? 1 : 0;
            return checked(~static_cast<__int128>(value), site);
        }
        return parse_primary(t, i, site);
    }

    long long parse_primary(const std::vector<Tok> &t, std::size_t &i, const Tok &site) const
    {
        if (i >= t.size())
            fail(site, "empty #if");
        if (t[i].kind == Kind::Punct && t[i].text == "(") {
            ++i;
            long long value = parse_or(t, i, site);
            if (i >= t.size() || t[i].text != ")")
                fail(site, "unclosed '(' in #if");
            ++i;
            return value;
        }
        if (t[i].kind == Kind::Ident) {
            ++i;
            return 0;
        }
        if (t[i].kind == Kind::Number || t[i].kind == Kind::Char) {
            long long value = literal_value(t[i]);
            ++i;
            return value;
        }
        fail(t[i], "not an integer constant");
    }

    long long literal_value(const Tok &tok) const
    {
        if (tok.kind == Kind::Char)
            return char_value(tok);
        const std::string &text = tok.text;
        std::size_t end = text.size();
        while (end > 0 && (text[end - 1] == 'u' || text[end - 1] == 'U' || text[end - 1] == 'l'
                           || text[end - 1] == 'L'))
            --end;
        const std::string digits = text.substr(0, end);
        if (digits.empty() || digits.find('.') != std::string::npos)
            fail(tok, "not an integer constant");
        int base = 10;
        std::size_t start = 0;
        if (digits.size() > 1 && digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X')) {
            base = 16;
            start = 2;
        } else if (digits.size() > 1 && digits[0] == '0') {
            base = 8;
            start = 1;
        }
        if (start >= digits.size())
            fail(tok, "not an integer constant");
        unsigned long long value = 0;
        for (std::size_t i = start; i < digits.size(); ++i) {
            int digit = std::tolower(static_cast<unsigned char>(digits[i]));
            int amount = -1;
            if (digit >= '0' && digit <= '9')
                amount = digit - '0';
            else if (digit >= 'a' && digit <= 'f')
                amount = digit - 'a' + 10;
            if (amount < 0 || amount >= base)
                fail(tok, "not an integer constant");
            if (value > (static_cast<unsigned long long>(INT64_MAX) - static_cast<unsigned>(amount))
                    / static_cast<unsigned>(base))
                fail(tok, "integer overflow");
            value = value * static_cast<unsigned>(base) + static_cast<unsigned>(amount);
        }
        return static_cast<long long>(value);
    }

    long long char_value(const Tok &tok) const
    {
        const std::string &text = tok.text;
        if (text.size() < 3 || text.front() != '\'' || text.back() != '\'')
            fail(tok, "not an integer constant");
        std::size_t i = 1;
        unsigned long long value = 0;
        int count = 0;
        while (i + 1 < text.size()) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            if (c == '\\') {
                if (i + 1 >= text.size() - 1)
                    fail(tok, "bad character constant");
                char e = text[++i];
                if (e == 'n')
                    c = '\n';
                else if (e == 't')
                    c = '\t';
                else if (e == 'r')
                    c = '\r';
                else if (e == '\\' || e == '\'' || e == '"')
                    c = static_cast<unsigned char>(e);
                else if (e == '0')
                    c = 0;
                else
                    fail(tok, "bad character constant");
            }
            value = (value << 8) + c;
            ++count;
            ++i;
            if (count > 1)
                fail(tok, "not an integer constant");
        }
        if (count != 1)
            fail(tok, "not an integer constant");
        return static_cast<long long>(value);
    }

    static long long checked(__int128 value, const Tok &site, const Engine *engine)
    {
        if (value < static_cast<__int128>(INT64_MIN) || value > static_cast<__int128>(INT64_MAX))
            engine->fail(site, "integer overflow");
        return static_cast<long long>(value);
    }

    long long checked(__int128 value, const Tok &site) const
    {
        return checked(value, site, this);
    }

    long long condition_value(std::vector<Tok> tokens, const Tok &site)
    {
        std::vector<Tok> kept;
        for (const Tok &tok : tokens) {
            if (tok.kind != Kind::Newline)
                kept.push_back(tok);
        }
        if (kept.empty())
            fail(site, "empty #if");
        replace_defined(kept, macros_, site, this);
        kept = expand_fully(std::move(kept), site);
        std::vector<Tok> plain;
        for (const Tok &tok : kept) {
            if (tok.kind != Kind::Newline)
                plain.push_back(tok);
        }
        return eval_expr(plain, site);
    }

    std::vector<Tok> rest_of_line(Cursor &cursor)
    {
        std::vector<Tok> tokens;
        const std::vector<Tok> &in = *cursor.in;
        while (cursor.i < in.size() && in[cursor.i].kind != Kind::Newline)
            tokens.push_back(in[cursor.i++]);
        return tokens;
    }

    void end_line(Cursor &cursor, bool numbered, int number)
    {
        if (cursor.i < cursor.in->size() && (*cursor.in)[cursor.i].kind == Kind::Newline)
            ++cursor.i;
        cursor.line = numbered ? number : cursor.line + 1;
        cursor.bol = true;
    }

    std::string header_spelling(const std::vector<Tok> &tokens, const Tok &site, bool &angle)
    {
        if (tokens.empty())
            fail(site, "missing #include file");
        if (tokens[0].kind == Kind::String) {
            angle = false;
            if (tokens.size() != 1)
                fail(site, "extra tokens in #include");
            const std::string &text = tokens[0].text;
            if (text.size() < 2 || text.front() != '"' || text.back() != '"')
                fail(site, "missing #include file");
            std::string name;
            for (std::size_t i = 1; i + 1 < text.size(); ++i) {
                if (text[i] == '\\' && i + 2 < text.size()) {
                    name.push_back(text[i + 1]);
                    ++i;
                    continue;
                }
                name.push_back(text[i]);
            }
            if (name.empty())
                fail(site, "missing #include file");
            return name;
        }
        if (tokens[0].kind == Kind::Punct && tokens[0].text == "<") {
            angle = true;
            std::string name;
            bool closed = false;
            for (std::size_t i = 1; i < tokens.size(); ++i) {
                if (tokens[i].kind == Kind::Punct && tokens[i].text == ">") {
                    if (i + 1 != tokens.size())
                        fail(site, "extra tokens in #include");
                    closed = true;
                    break;
                }
                if (tokens[i].space_before)
                    name.push_back(' ');
                name += tokens[i].text;
            }
            if (!closed || name.empty())
                fail(site, "missing #include file");
            return name;
        }
        fail(site, "#include is not a file name");
    }

    std::string find_header(const std::string &name, bool angle, const std::string &from) const
    {
        std::vector<fs::path> candidates;
        fs::path header(name);
        if (header.is_absolute()) {
            candidates.push_back(header);
        } else {
            if (!angle)
                candidates.push_back(fs::path(from).parent_path() / header);
            for (const std::string &dir : request_.includes)
                candidates.push_back(fs::path(dir) / header);
            if (!request_.nostdinc) {
                candidates.push_back(fs::path("/usr/local/include") / header);
                candidates.push_back(fs::path("/usr/include") / header);
            }
        }
        for (const fs::path &candidate : candidates) {
            std::error_code error;
            if (!fs::is_regular_file(candidate, error) || error)
                continue;
            const fs::path full = fs::canonical(candidate, error);
            if (!error)
                return full.string();
        }
        return {};
    }

    std::string resolve_force_include(const std::string &name)
    {
        std::error_code cwd_error;
        const fs::path cwd = fs::current_path(cwd_error);
        const std::string from = cwd_error ? std::string("preproc-include")
                                           : (cwd / "preproc-include").string();
        Tok site;
        site.kind = Kind::Ident;
        site.text = "include";
        site.file = request_.input;
        site.line = 1;
        site.column = 1;
        const std::string path = find_header(name, false, from);
        if (path.empty())
            fail(site, "cannot include " + name);
        return path;
    }

    std::vector<Tok> include_header(const std::string &name, bool angle, const Tok &site,
                                    const std::string &from)
    {
        const std::string path = find_header(name, angle, from);
        if (path.empty())
            fail(site, "cannot include " + name);
        for (const std::string &open : include_stack_) {
            if (open == path)
                fail(site, "include cycle: " + name);
        }
        include_stack_.push_back(path);
        std::vector<Tok> tokens = lex_file(path);
        std::vector<Tok> produced = pass_tokens(tokens, path, 1);
        include_stack_.pop_back();
        return produced;
    }

    static bool emitting(const std::vector<Cond> &stack)
    {
        return stack.empty() || (stack.back().parent && stack.back().now);
    }

    void handle_directive(Cursor &cursor, std::vector<Tok> &out, std::vector<Cond> &stack)
    {
        const Tok hash = (*cursor.in)[cursor.i++];
        std::vector<Tok> line = rest_of_line(cursor);
        std::size_t at = 0;
        if (at >= line.size()) {
            end_line(cursor, false, 0);
            return;
        }
        if (line[at].kind != Kind::Ident)
            fail(line[at], "invalid directive");
        const std::string name = line[at].text;
        const bool active = emitting(stack);
        auto rest = [&]() {
            std::vector<Tok> tokens;
            for (std::size_t i = at + 1; i < line.size(); ++i)
                tokens.push_back(line[i]);
            return tokens;
        };

        if (name == "if" || name == "ifdef" || name == "ifndef") {
            bool taken = false;
            if (active) {
                if (name == "if")
                    taken = condition_value(rest(), line[at]) != 0;
                else {
                    auto tokens = rest();
                    if (tokens.size() != 1 || tokens[0].kind != Kind::Ident)
                        fail(line[at], "invalid #" + name);
                    const bool defined = macros_.count(tokens[0].text) != 0;
                    taken = name == "ifdef" ? defined : !defined;
                }
            }
            Cond frame;
            frame.parent = active;
            frame.any = taken;
            frame.now = taken;
            stack.push_back(frame);
            end_line(cursor, false, 0);
            return;
        }
        if (name == "elif") {
            if (stack.empty())
                fail(hash, "#elif without #if");
            Cond &frame = stack.back();
            if (frame.saw_else)
                fail(hash, "#elif after #else");
            if (!frame.parent || frame.any)
                frame.now = false;
            else
                frame.now = condition_value(rest(), line[at]) != 0;
            if (frame.now)
                frame.any = true;
            end_line(cursor, false, 0);
            return;
        }
        if (name == "else") {
            if (!rest().empty())
                fail(line[at], "extra tokens in #else");
            if (stack.empty())
                fail(hash, "#else without #if");
            Cond &frame = stack.back();
            if (frame.saw_else)
                fail(hash, "#else after #else");
            frame.saw_else = true;
            frame.now = frame.parent && !frame.any;
            frame.any = true;
            end_line(cursor, false, 0);
            return;
        }
        if (name == "endif") {
            if (!rest().empty())
                fail(line[at], "extra tokens in #endif");
            if (stack.empty())
                fail(hash, "#endif without #if");
            stack.pop_back();
            end_line(cursor, false, 0);
            return;
        }
        if (!active) {
            end_line(cursor, false, 0);
            return;
        }
        if (name == "define") {
            auto tokens = rest();
            if (tokens.empty() || tokens[0].kind != Kind::Ident)
                fail(line[at], "invalid #define");
            Tok site = tokens[0];
            Macro macro;
            std::size_t body_at = 1;
            if (tokens.size() > 1 && tokens[1].kind == Kind::Punct && tokens[1].text == "("
                && !tokens[1].space_before) {
                macro.function = true;
                std::size_t p = 2;
                if (p >= tokens.size())
                    fail(site, "unclosed macro parameter list");
                if (!(tokens[p].kind == Kind::Punct && tokens[p].text == ")")) {
                    while (true) {
                        if (p >= tokens.size())
                            fail(site, "unclosed macro parameter list");
                        if (tokens[p].kind == Kind::Punct && tokens[p].text == "...")
                            fail(tokens[p], "variadic macros are rejected");
                        if (tokens[p].kind != Kind::Ident)
                            fail(tokens[p], "invalid macro parameter");
                        macro.params.push_back(tokens[p].text);
                        ++p;
                        if (p < tokens.size() && tokens[p].kind == Kind::Punct && tokens[p].text == ",") {
                            ++p;
                            continue;
                        }
                        if (p < tokens.size() && tokens[p].kind == Kind::Punct && tokens[p].text == ")")
                            break;
                        fail(site, "unclosed macro parameter list");
                    }
                }
                body_at = p + 1;
            }
            if (body_at < tokens.size()) {
                macro.body.assign(tokens.begin() + static_cast<std::ptrdiff_t>(body_at), tokens.end());
                macro.body.front().space_before = false;
            }
            define_macro(std::move(macro), site);
            end_line(cursor, false, 0);
            return;
        }
        if (name == "undef") {
            auto tokens = rest();
            if (tokens.size() != 1 || tokens[0].kind != Kind::Ident)
                fail(line[at], "invalid #undef");
            if (tokens[0].text == "__LINE__" || tokens[0].text == "__FILE__")
                fail(tokens[0], "cannot undefine " + tokens[0].text);
            macros_.erase(tokens[0].text);
            end_line(cursor, false, 0);
            return;
        }
        if (name == "include") {
            auto tokens = rest();
            if (!tokens.empty() && tokens[0].kind == Kind::Ident)
                tokens = expand_fully(tokens, line[at]);
            bool angle = false;
            const std::string header = header_spelling(tokens, line[at], angle);
            end_line(cursor, false, 0);
            std::vector<Tok> included = include_header(header, angle, line[at], cursor.file);
            out.insert(out.end(), included.begin(), included.end());
            return;
        }
        if (name == "line") {
            auto tokens = expand_fully(rest(), line[at]);
            if (tokens.empty() || tokens[0].kind != Kind::Number)
                fail(line[at], "invalid #line");
            long long number = literal_value(tokens[0]);
            if (number < 1 || number > 2147483647)
                fail(line[at], "invalid #line");
            if (tokens.size() == 2) {
                if (tokens[1].kind != Kind::String)
                    fail(line[at], "invalid #line");
                bool angle = false;
                cursor.file = header_spelling({tokens[1]}, line[at], angle);
            } else if (tokens.size() != 1) {
                fail(line[at], "invalid #line");
            }
            end_line(cursor, true, static_cast<int>(number));
            return;
        }
        if (name == "error") {
            std::string message;
            for (std::size_t i = at + 1; i < line.size(); ++i) {
                if (!message.empty() && line[i].space_before)
                    message.push_back(' ');
                message += line[i].text;
            }
            if (message.empty())
                message = "#error";
            fail(line[at], message);
        }
        if (name == "pragma")
            fail(line[at], "#pragma is rejected");
        fail(line[at], "unknown directive: " + name);
    }

    std::vector<Tok> pass_tokens(const std::vector<Tok> &in, const std::string &file, int start_line)
    {
        Cursor cursor;
        cursor.in = &in;
        cursor.file = file;
        cursor.physical = file;
        cursor.line = start_line;
        std::vector<Tok> out;
        std::vector<Cond> stack;
        while (cursor.i < in.size()) {
            const Tok &tok = in[cursor.i];
            if (!tok.file.empty() && tok.file != cursor.physical) {
                cursor.physical = tok.file;
                cursor.file = tok.file;
                cursor.line = tok.line;
            }
            if (tok.kind == Kind::Newline) {
                const bool active = emitting(stack);
                if (active)
                    out.push_back(tok);
                ++cursor.i;
                ++cursor.line;
                cursor.bol = true;
                continue;
            }
            if (cursor.bol && tok.kind == Kind::Punct && tok.text == "#") {
                handle_directive(cursor, out, stack);
                continue;
            }
            cursor.bol = false;
            const bool active = emitting(stack);
            if (!active) {
                ++cursor.i;
                continue;
            }
            std::size_t consumed = 0;
            if (expand_name(cursor, out, consumed)) {
                cursor.i += consumed;
                continue;
            }
            out.push_back(tok);
            ++cursor.i;
        }
        if (!stack.empty())
            fail(cursor.file.empty() ? file : cursor.file, cursor.line, 1, "missing #endif");
        return out;
    }
};

}

std::string preprocess(const PreprocRequest &request)
{
    if (request.input.empty())
        throw std::runtime_error("missing input file\n");
    return Engine(request).run();
}

}
