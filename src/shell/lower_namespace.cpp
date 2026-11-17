/*
 * Namespaces, qualified names, and using.
 */
#include "lower.hpp"

std::string Codegen::join_ns(int count) const
{
    std::string key;
    int limit = count;
    if (limit > static_cast<int>(ns_stack.size()))
        limit = static_cast<int>(ns_stack.size());
    for (int i = 0; i < limit; i++) {
        if (!key.empty())
            key += "::";
        key += ns_stack[static_cast<std::size_t>(i)];
    }
    return key;
}

std::string Codegen::current_path() const
{
    return join_ns(static_cast<int>(ns_stack.size()));
}

std::string Codegen::qualify(const std::string &name) const
{
    std::string path = current_path();
    if (path.empty())
        return name;
    return path + "::" + name;
}

bool Codegen::name_known(const std::string &key) const
{
    if (key.empty())
        return false;
    if (scopes.empty())
        return false;
    return scopes[0].count(key) != 0 || overloads.count(key) != 0 || aliases.count(key) != 0
        || enum_consts.count(key) != 0 || namespaces.count(key) != 0;
}

std::vector<std::string> Codegen::dedup_keys(std::vector<std::string> keys) const
{
    std::vector<std::string> out;
    for (const std::string &key : keys) {
        bool seen = false;
        for (const std::string &earlier : out) {
            if (earlier == key)
                seen = true;
        }
        if (!seen)
            out.push_back(key);
    }
    return out;
}

std::vector<std::string> Codegen::use_keys(const std::vector<std::pair<std::string, std::string>> &names,
                                  const std::vector<std::string> &directives, const std::string &name) const
{
    std::vector<std::string> hits;
    for (const auto &use : names) {
        if (use.first == name && name_known(use.second))
            hits.push_back(use.second);
    }
    if (!hits.empty())
        return dedup_keys(hits);
    for (const std::string &dir : directives) {
        std::string key = dir + "::" + name;
        if (name_known(key))
            hits.push_back(key);
    }
    return dedup_keys(hits);
}

std::vector<std::string> Codegen::scope_keys(const LookupScope &scope, const std::string &name) const
{
    std::string key = scope.ns.empty() ? name : scope.ns + "::" + name;
    if (name_known(key))
        return {key};
    std::vector<std::string> declared = use_keys(scope.names, {}, name);
    if (!declared.empty())
        return declared;
    return use_keys({}, scope.directives, name);
}

std::vector<LookupScope> Codegen::live_chain() const
{
    std::vector<LookupScope> chain;
    for (int i = static_cast<int>(ns_stack.size()); i >= 0; --i) {
        std::string key = join_ns(i);
        LookupScope scope;
        auto found = namespaces.find(key);
        if (found != namespaces.end())
            scope = found->second;
        scope.ns = key;
        chain.push_back(scope);
    }
    return chain;
}

std::vector<std::string> Codegen::visible_keys(const std::string &name) const
{
    for (auto block = blocks.rbegin(); block != blocks.rend(); ++block) {
        std::vector<std::string> declared = use_keys(block->names, {}, name);
        if (!declared.empty())
            return declared;
        std::vector<std::string> directed = use_keys({}, block->directives, name);
        if (!directed.empty())
            return directed;
    }
    std::vector<LookupScope> live;
    const std::vector<LookupScope> *chain = active_lookup;
    if (chain == NULL) {
        live = live_chain();
        chain = &live;
    }
    for (const LookupScope &scope : *chain) {
        std::vector<std::string> hits = scope_keys(scope, name);
        if (!hits.empty())
            return hits;
    }
    return {};
}

void Codegen::flatten_name(const AstNode *node, std::vector<const AstNode *> &parts) const
{
    if (node == NULL)
        return;
    if (node->kind == AST_QUALIFIED) {
        flatten_name(node->a, parts);
        flatten_name(node->b, parts);
        return;
    }
    parts.push_back(node);
}

std::string Codegen::find_ns_component(const std::string &component)
{
    for (int i = static_cast<int>(ns_stack.size()); i >= 0; --i) {
        std::string key = join_ns(i);
        if (!key.empty())
            key += "::";
        key += component;
        if (namespaces.count(key) != 0)
            return key;
    }
    error_name("unknown namespace", component.c_str());
    return "";
}

std::string Codegen::namespace_key(const AstNode *node)
{
    std::vector<const AstNode *> parts;
    flatten_name(node, parts);
    if (parts.empty() || parts[0] == NULL || parts[0]->text == NULL) {
        error("unknown namespace");
        return "";
    }
    std::string path = find_ns_component(parts[0]->text);
    if (failed)
        return "";
    for (std::size_t i = 1; i < parts.size(); i++) {
        if (parts[i] == NULL || parts[i]->text == NULL) {
            error("unknown namespace");
            return "";
        }
        path += "::";
        path += parts[i]->text;
        if (namespaces.count(path) == 0) {
            error_name("unknown namespace", path.c_str());
            return "";
        }
    }
    return path;
}

std::string Codegen::entity_key(const AstNode *node)
{
    std::vector<const AstNode *> parts;
    flatten_name(node, parts);
    if (parts.empty() || parts.back() == NULL || parts.back()->text == NULL) {
        error("the name is missing");
        return "";
    }
    if (parts.size() == 1) {
        std::vector<std::string> keys = visible_keys(parts[0]->text);
        if (keys.size() != 1) {
            if (keys.empty())
                error_name("unknown name", parts[0]->text);
            else
                error_name("ambiguous name", parts[0]->text);
            return "";
        }
        return keys[0];
    }
    std::string path = find_ns_component(parts[0]->text);
    if (failed)
        return "";
    for (std::size_t i = 1; i + 1 < parts.size(); i++) {
        path += "::";
        path += parts[i]->text;
        if (namespaces.count(path) == 0) {
            error_name("unknown namespace", path.c_str());
            return "";
        }
    }
    std::string key = path + "::" + parts.back()->text;
    if (!name_known(key)) {
        error_name("unknown name", key.c_str());
        return "";
    }
    return key;
}

bool Codegen::namespace_only_name(const char *name) const
{
    if (name == NULL)
        return false;
    std::vector<std::string> keys = visible_keys(name);
    return keys.size() == 1 && namespace_only(keys[0]);
}

bool Codegen::namespace_only(const std::string &key) const
{
    return !key.empty() && namespaces.count(key) != 0 && scopes[0].count(key) == 0
        && overloads.count(key) == 0 && aliases.count(key) == 0 && enum_consts.count(key) == 0;
}

std::optional<int64_t> Codegen::lookup_enum(const char *name)
{
    if (name == NULL)
        return std::nullopt;
    for (const std::string &key : visible_keys(name)) {
        auto found = enum_consts.find(key);
        if (found != enum_consts.end())
            return found->second;
    }
    return std::nullopt;
}

std::optional<Ty> Codegen::lookup_alias(const char *name)
{
    if (name == NULL)
        return std::nullopt;
    for (const std::string &key : visible_keys(name)) {
        auto found = aliases.find(key);
        if (found != aliases.end())
            return found->second;
    }
    return std::nullopt;
}

void Codegen::declare_item(const AstNode *item)
{
    if (item == NULL || failed)
        return;
    if (item->kind == AST_METHOD)
        declare_function(item->a, item->b, item->c);
    else if (item->kind == AST_DECLARATION)
        emit_declaration(item, true);
    else if (item->kind == AST_NAMESPACE)
        declare_namespace(item);
    else if (item->kind == AST_USING)
        add_using(item);
    else if (item->kind == AST_CONSTRUCTOR || item->kind == AST_DESTRUCTOR)
        declare_special(item);
    else
        reject_node(item);
}

void Codegen::declare_namespace(const AstNode *node)
{
    if (node->text == NULL) {
        error("namespace has no name");
        return;
    }
    std::string key = qualify(node->text);
    if (scopes[0].count(key) != 0 || overloads.count(key) != 0 || aliases.count(key) != 0
        || enum_consts.count(key) != 0) {
        error_name("name is already defined", node->text);
        return;
    }
    if (namespaces.count(key) == 0)
        namespaces[key] = LookupScope{};
    ns_stack.push_back(node->text);
    for (const AstNode *item = node->a; item != NULL && !failed; item = item->next)
        declare_item(item);
    ns_stack.pop_back();
}

void Codegen::add_using(const AstNode *node)
{
    bool directive = node->text != NULL && std::strcmp(node->text, "namespace") == 0;
    if (directive) {
        std::string target = namespace_key(node->a);
        if (failed)
            return;
        if (!blocks.empty())
            blocks.back().directives.push_back(target);
        else
            namespaces[current_path()].directives.push_back(target);
        return;
    }
    if (node->a == NULL || node->a->kind != AST_QUALIFIED) {
        error("using needs a qualified name");
        return;
    }
    std::string target = entity_key(node->a);
    if (failed)
        return;
    if (namespace_only(target)) {
        error("namespace is not a value");
        return;
    }
    std::string introduced = node->a->b != NULL && node->a->b->text != NULL ? node->a->b->text : "";
    if (introduced.empty()) {
        error("using needs a qualified name");
        return;
    }
    if (!blocks.empty())
        blocks.back().names.push_back({introduced, target});
    else
        namespaces[current_path()].names.push_back({introduced, target});
}

std::optional<Slot> Codegen::lookup(const char *name)
{
    if (name == NULL)
        return std::nullopt;
    for (std::size_t i = scopes.size(); i-- > 1;) {
        auto found = scopes[i].find(name);
        if (found != scopes[i].end())
            return found->second;
    }
    const Slot *hit = NULL;
    for (const std::string &key : visible_keys(name)) {
        auto found = scopes[0].find(key);
        if (found == scopes[0].end())
            continue;
        if (hit != NULL && hit->addr != found->second.addr) {
            error_name("ambiguous name", name);
            return std::nullopt;
        }
        hit = &found->second;
    }
    if (hit == NULL)
        return std::nullopt;
    return *hit;
}
