extern "C" {
#include "ast.h"
}

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

static int is_specifier(const AstNode *node, const char *text)
{
    return node != NULL && node->kind == AST_SPECIFIER && node->text != NULL
        && std::strcmp(node->text, text) == 0 && node->next == NULL;
}

static int no_parameters(const AstNode *params)
{
    if (params == NULL)
        return 1;
    if (params->next != NULL || params->kind != AST_PARAMETER || params->b != NULL)
        return 0;
    return is_specifier(params->a, "void");
}

static int integer_literal(const AstNode *node, int32_t *value)
{
    char *end = NULL;
    long parsed;

    if (node == NULL || node->kind != AST_LITERAL || node->text == NULL)
        return 0;
    errno = 0;
    parsed = std::strtol(node->text, &end, 10);
    if (errno != 0 || end == node->text || *end != '\0')
        return 0;
    if (parsed < INT32_MIN || parsed > INT32_MAX)
        return 0;
    *value = static_cast<int32_t>(parsed);
    return 1;
}

static const AstNode *return_literal(const AstNode *body, int32_t *value)
{
    const AstNode *stmt;

    if (body == NULL || body->kind != AST_COMPOUND)
        return NULL;
    stmt = body->a;
    if (stmt == NULL || stmt->next != NULL || stmt->kind != AST_RETURN)
        return NULL;
    if (!integer_literal(stmt->a, value))
        return NULL;
    return stmt;
}

int compile_to_bitcode(const AstNode *root, const char *bitcode_path)
{
    const AstNode *method;
    const AstNode *decl;
    const AstNode *name;
    int32_t value = 0;
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    llvm::FunctionType *type;
    llvm::Function *function;
    llvm::BasicBlock *entry;
    llvm::IRBuilder<> builder(context);
    std::string diagnostic;
    llvm::raw_string_ostream errors(diagnostic);
    std::error_code error;

    if (root == NULL || root->kind != AST_TRANSLATION_UNIT || root->a == NULL
        || root->a->next != NULL || root->a->kind != AST_METHOD) {
        std::fprintf(stderr, "this slice compiles one function\n");
        return 1;
    }
    method = root->a;
    if (!is_specifier(method->a, "int")) {
        std::fprintf(stderr, "this slice compiles a function that returns int\n");
        return 1;
    }
    decl = method->b;
    if (decl == NULL || decl->kind != AST_FUNCTION_DECL || decl->c != NULL
        || !no_parameters(decl->b)) {
        std::fprintf(stderr, "this slice compiles a function with no parameters\n");
        return 1;
    }
    name = decl->a;
    if (name == NULL || name->kind != AST_IDENT || name->text == NULL) {
        std::fprintf(stderr, "the function has no name\n");
        return 1;
    }
    if (return_literal(method->c, &value) == NULL) {
        std::fprintf(stderr, "this slice compiles a return of an integer literal\n");
        return 1;
    }

    module = std::make_unique<llvm::Module>("smallcpp", context);
    type = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), false);
    function = llvm::Function::Create(type, llvm::Function::ExternalLinkage, name->text,
                                      module.get());
    entry = llvm::BasicBlock::Create(context, "entry", function);
    builder.SetInsertPoint(entry);
    builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                              static_cast<uint64_t>(value), true));

    if (llvm::verifyModule(*module, &errors)) {
        errors.flush();
        std::fprintf(stderr, "module failed verification: %s\n", diagnostic.c_str());
        return 1;
    }

    llvm::raw_fd_ostream out(bitcode_path, error, llvm::sys::fs::OF_None);
    if (error) {
        std::fprintf(stderr, "%s: %s\n", bitcode_path, error.message().c_str());
        return 1;
    }
    llvm::WriteBitcodeToFile(*module, out);
    out.flush();
    if (out.has_error()) {
        std::fprintf(stderr, "%s: %s\n", bitcode_path, out.error().message().c_str());
        return 1;
    }
    return 0;
}
