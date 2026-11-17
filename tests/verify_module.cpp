/*
 * verify_module — load one example's bitcode and check one feature.
 *
 * Usage: verify_module <mode> <module.bc>
 *
 * verifyModule runs for every mode. The mode then checks the one thing
 * that example lowers. A module that verifies but lacks the feature fails.
 */

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <stdio.h>
#include <string.h>

#include <memory>
#include <string>

static int fail(const char *mode, const char *why)
{
    fprintf(stderr, "%s: %s\n", mode, why);
    return 1;
}

static llvm::Function *answer(llvm::Module &module)
{
    llvm::Function *fn = module.getFunction("answer");

    if (fn == NULL || fn->empty())
        return NULL;
    return fn;
}

static llvm::ReturnInst *only_ret(llvm::Function &fn)
{
    llvm::ReturnInst *found = NULL;

    for (llvm::BasicBlock &bb : fn) {
        auto *ret = llvm::dyn_cast<llvm::ReturnInst>(bb.getTerminator());
        if (ret == NULL)
            continue;
        if (found != NULL)
            return NULL;
        found = ret;
    }
    return found;
}

static llvm::ConstantInt *ret_int(llvm::Function &fn)
{
    llvm::ReturnInst *ret = only_ret(fn);

    if (ret == NULL || ret->getReturnValue() == NULL)
        return NULL;
    return llvm::dyn_cast<llvm::ConstantInt>(ret->getReturnValue());
}

static int expect_sig(const char *mode, llvm::Module &module,
                      llvm::Type *ret, unsigned nargs, llvm::Type *arg)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail(mode, "no function answer");
    if (fn->getReturnType() != ret)
        return fail(mode, "wrong return type");
    if (fn->arg_size() != nargs)
        return fail(mode, "wrong parameter count");
    if (nargs == 1 && fn->getArg(0)->getType() != arg)
        return fail(mode, "wrong parameter type");
    if (nargs == 2) {
        if (fn->getArg(0)->getType() != arg || fn->getArg(1)->getType() != arg)
            return fail(mode, "wrong parameter type");
    }
    return 0;
}

static int identity(const char *mode, llvm::Module &module, llvm::Type *ty)
{
    if (expect_sig(mode, module, ty, 1, ty) != 0)
        return 1;
    llvm::ReturnInst *ret = only_ret(*answer(module));
    auto *load = ret == NULL ? NULL
                             : llvm::dyn_cast<llvm::LoadInst>(ret->getReturnValue());

    if (load == NULL || load->getType() != ty)
        return fail(mode, "answer does not return its parameter");
    return 0;
}

static bool const_eq(const llvm::Value *value, uint64_t n)
{
    auto *constant = llvm::dyn_cast<llvm::ConstantInt>(value);

    return constant != NULL && constant->equalsInt(n);
}

static llvm::Instruction *ret_op(llvm::Function &fn)
{
    llvm::ReturnInst *ret = only_ret(fn);

    if (ret == NULL)
        return NULL;
    return llvm::dyn_cast<llvm::Instruction>(ret->getReturnValue());
}

static int ret_bin(const char *mode, llvm::Module &module,
                   llvm::Instruction::BinaryOps op, int which, uint64_t n)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());

    if (expect_sig(mode, module, i32, 1, i32) != 0)
        return 1;
    auto *bin = llvm::dyn_cast_or_null<llvm::BinaryOperator>(ret_op(*answer(module)));

    if (bin == NULL || bin->getOpcode() != op)
        return fail(mode, "return is not that operation");
    if (which == 0 && !const_eq(bin->getOperand(0), n))
        return fail(mode, "wrong constant operand");
    if (which == 1 && !const_eq(bin->getOperand(1), n))
        return fail(mode, "wrong constant operand");
    if (which < 0 && !const_eq(bin->getOperand(0), n) && !const_eq(bin->getOperand(1), n))
        return fail(mode, "wrong constant operand");
    return 0;
}

struct Back {
    llvm::BasicBlock *from;
    llvm::BasicBlock *to;
    bool ok;
};

static Back one_back(llvm::Function &fn)
{
    llvm::DominatorTree tree(fn);
    Back found = { NULL, NULL, false };
    int count = 0;

    for (llvm::BasicBlock &bb : fn) {
        for (llvm::BasicBlock *succ : llvm::successors(&bb)) {
            if (!tree.dominates(succ, &bb))
                continue;
            count++;
            found.from = &bb;
            found.to = succ;
        }
    }
    found.ok = false;
    if (count == 1 && llvm::succ_size(&fn.getEntryBlock()) == 1)
        found.ok = found.to == *llvm::succ_begin(&fn.getEntryBlock());
    return found;
}

struct Loop {
    bool ok;
    llvm::BasicBlock *header;
    llvm::BasicBlock *latch;
    llvm::BasicBlock *into;
    llvm::BasicBlock *exit;
};

static Loop loop_of(llvm::Function &fn)
{
    Loop loop = { false, NULL, NULL, NULL, NULL };
    Back back = one_back(fn);
    auto *header_br = back.ok
        ? llvm::dyn_cast<llvm::BranchInst>(back.to->getTerminator())
        : NULL;

    if (header_br == NULL || !header_br->isConditional())
        return loop;
    llvm::DominatorTree tree(fn);
    llvm::BasicBlock *left = header_br->getSuccessor(0);
    llvm::BasicBlock *right = header_br->getSuccessor(1);
    bool left_in = tree.dominates(left, back.from);

    loop.ok = true;
    loop.header = back.to;
    loop.latch = back.from;
    loop.into = left_in ? left : right;
    loop.exit = left_in ? right : left;
    return loop;
}

static bool direct_succ(llvm::BasicBlock *from, llvm::BasicBlock *to)
{
    for (llvm::BasicBlock *succ : llvm::successors(from)) {
        if (succ == to)
            return true;
    }
    return false;
}

static bool uncond(llvm::BasicBlock *bb)
{
    auto *br = llvm::dyn_cast<llvm::BranchInst>(bb->getTerminator());

    return br != NULL && br->isUnconditional();
}

static bool has_store(llvm::BasicBlock *bb)
{
    for (llvm::Instruction &inst : *bb) {
        if (llvm::isa<llvm::StoreInst>(inst))
            return true;
    }
    return false;
}

static int check_return(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());

    if (fn == NULL)
        return fail("return", "no function answer");
    if (fn->getReturnType() != i32 || fn->arg_size() != 0)
        return fail("return", "answer is not int(void)");
    return 0;
}

static int check_if(llvm::Module &module)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail("if", "no function answer");
    for (llvm::BasicBlock &bb : *fn) {
        auto *br = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator());
        if (br != NULL && br->isConditional())
            return 0;
    }
    return fail("if", "no conditional branch");
}

static int check_else(llvm::Module &module)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail("else", "no function answer");
    for (llvm::BasicBlock &bb : *fn) {
        auto *br = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator());
        if (br == NULL || !br->isConditional())
            continue;
        llvm::BasicBlock *left = br->getSuccessor(0);
        llvm::BasicBlock *right = br->getSuccessor(1);
        if (!has_store(left) || !has_store(right) || !uncond(left) || !uncond(right))
            continue;
        if (*llvm::succ_begin(left) == *llvm::succ_begin(right))
            return 0;
    }
    return fail("else", "the two arms do not both store and join");
}

static int check_while(llvm::Module &module)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail("while", "no function answer");
    Loop loop = loop_of(*fn);
    if (!loop.ok || loop.into != loop.latch || !uncond(loop.latch))
        return fail("while", "not a while loop");
    return 0;
}

static int check_do(llvm::Module &module)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail("do", "no function answer");
    Back back = one_back(*fn);

    if (!back.ok || !uncond(back.to))
        return fail("do", "not a do loop");
    auto *br = llvm::dyn_cast<llvm::BranchInst>(back.from->getTerminator());
    if (br == NULL || !br->isConditional())
        return fail("do", "not a do loop");
    return 0;
}

static int check_for(const char *mode, llvm::Module &module)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail(mode, "no function answer");
    Loop loop = loop_of(*fn);

    if (!loop.ok || direct_succ(loop.header, loop.latch) || !uncond(loop.latch))
        return fail(mode, "not a for loop");
    if (llvm::succ_size(loop.into) != 1 || *llvm::succ_begin(loop.into) != loop.latch)
        return fail(mode, "not a for loop");
    return 0;
}

static int check_switch(llvm::Module &module)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail("switch", "no function answer");
    for (llvm::BasicBlock &bb : *fn) {
        auto *sw = llvm::dyn_cast<llvm::SwitchInst>(bb.getTerminator());
        if (sw == NULL || sw->getNumCases() < 1)
            continue;
        bool saw_one = false;
        for (const auto &cse : sw->cases()) {
            if (cse.getCaseValue()->equalsInt(1))
                saw_one = true;
        }
        auto *def = llvm::dyn_cast<llvm::ReturnInst>(sw->getDefaultDest()->getTerminator());
        if (saw_one && def != NULL)
            return 0;
    }
    return fail("switch", "no switch with case 1 and a default return");
}

static int check_break(llvm::Module &module)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail("break", "no function answer");
    Loop loop = loop_of(*fn);

    if (!loop.ok || loop.into == loop.latch)
        return fail("break", "not a loop that leaves early");
    for (llvm::BasicBlock &bb : *fn) {
        if (&bb == loop.header)
            continue;
        for (llvm::BasicBlock *succ : llvm::successors(&bb)) {
            if (succ == loop.exit)
                return 0;
        }
    }
    return fail("break", "no branch to the loop exit");
}

static int check_continue(llvm::Module &module)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail("continue", "no function answer");
    Loop loop = loop_of(*fn);
    auto *inner = loop.ok ? llvm::dyn_cast<llvm::BranchInst>(loop.into->getTerminator()) : NULL;

    if (inner == NULL || !inner->isConditional() || direct_succ(loop.header, loop.latch))
        return fail("continue", "not a loop that skips to the step");
    for (llvm::BasicBlock *succ : llvm::successors(loop.into)) {
        if (llvm::succ_size(succ) == 1 && *llvm::succ_begin(succ) == loop.latch)
            return 0;
    }
    return fail("continue", "not a loop that skips to the step");
}

static int check_goto(llvm::Module &module)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail("goto", "no function answer");
    for (llvm::BasicBlock &bb : *fn) {
        if (llvm::pred_size(&bb) != 2)
            continue;
        bool jump = false;
        bool stored = false;
        for (llvm::BasicBlock *pred : llvm::predecessors(&bb)) {
            if (pred->size() == 1 && uncond(pred))
                jump = true;
            if (has_store(pred))
                stored = true;
        }
        if (jump && stored)
            return 0;
    }
    return fail("goto", "no label reached by a jump and by a fall-through");
}

static int check_void(llvm::Module &module)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail("void", "no function answer");
    if (!fn->getReturnType()->isVoidTy() || fn->arg_size() != 0)
        return fail("void", "answer is not void(void)");
    llvm::ReturnInst *ret = only_ret(*fn);
    if (ret == NULL || ret->getReturnValue() != NULL)
        return fail("void", "missing bare return");
    return 0;
}

static int check_empty(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());

    if (expect_sig("empty-list", module, i32, 0, NULL) != 0)
        return 1;
    llvm::ConstantInt *value = ret_int(*answer(module));
    if (value == NULL || !value->equalsInt(1))
        return fail("empty-list", "empty () does not return 1");
    return 0;
}

static int check_parameter(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());

    return identity("parameter", module, i32);
}

static int check_local(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());

    if (expect_sig("local", module, i32, 0, NULL) != 0)
        return 1;
    llvm::Function *fn = answer(module);
    llvm::ReturnInst *ret = only_ret(*fn);
    auto *load = ret == NULL ? NULL
                             : llvm::dyn_cast<llvm::LoadInst>(ret->getReturnValue());
    bool stored = false;

    if (load == NULL || !llvm::isa<llvm::AllocaInst>(load->getPointerOperand()))
        return fail("local", "return is not a load of a local");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && const_eq(store->getValueOperand(), 4))
            stored = true;
    }
    if (!stored)
        return fail("local", "local is not stored");
    return 0;
}

static llvm::GlobalVariable *global_n(llvm::Module &module)
{
    return module.getGlobalVariable("n", true);
}

static int check_global(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    llvm::GlobalVariable *global = global_n(module);

    if (expect_sig("global", module, i32, 0, NULL) != 0)
        return 1;
    if (global == NULL || global->isConstant() || global->getValueType() != i32)
        return fail("global", "no mutable int n");
    return 0;
}

static int check_const(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    llvm::GlobalVariable *global = global_n(module);

    if (expect_sig("const-var", module, i32, 0, NULL) != 0)
        return 1;
    if (global == NULL || !global->isConstant())
        return fail("const-var", "n is not a constant");
    auto *init = llvm::dyn_cast_or_null<llvm::ConstantInt>(global->getInitializer());
    if (init == NULL || !init->equalsInt(3))
        return fail("const-var", "n is not 3");
    return 0;
}

static int check_volatile(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    bool load = false;
    bool store = false;

    if (fn == NULL)
        return fail("volatile", "no function answer");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        if (auto *op = llvm::dyn_cast<llvm::LoadInst>(&inst))
            load = load || op->isVolatile();
        if (auto *op = llvm::dyn_cast<llvm::StoreInst>(&inst))
            store = store || op->isVolatile();
    }
    if (!load || !store)
        return fail("volatile", "missing a volatile load or store");
    return 0;
}

static int check_bool_const(const char *mode, llvm::Module &module, uint64_t bit)
{
    llvm::Type *i1 = llvm::Type::getInt1Ty(module.getContext());

    if (expect_sig(mode, module, i1, 0, NULL) != 0)
        return 1;
    llvm::ConstantInt *value = ret_int(*answer(module));
    if (value == NULL || !value->equalsInt(bit))
        return fail(mode, "wrong bool constant");
    return 0;
}

static int check_literal(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());

    if (expect_sig("literal", module, i32, 0, NULL) != 0)
        return 1;
    llvm::ConstantInt *value = ret_int(*answer(module));
    if (value == NULL || !value->equalsInt(42))
        return fail("literal", "does not return 42");
    return 0;
}

static int check_cast(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());

    if (expect_sig("cast", module, i32, 1, i32) != 0)
        return 1;
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *trunc = llvm::dyn_cast<llvm::TruncInst>(&inst);
        if (trunc != NULL && trunc->getType()->isIntegerTy(8))
            return 0;
    }
    return fail("cast", "no truncation to char");
}

static int check_sizeof(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());

    if (expect_sig("sizeof", module, i32, 0, NULL) != 0)
        return 1;
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *trunc = llvm::dyn_cast<llvm::TruncInst>(&inst);
        auto *wide = trunc == NULL ? NULL
                                   : llvm::dyn_cast<llvm::ConstantInt>(trunc->getOperand(0));
        if (wide != NULL && wide->getType()->isIntegerTy(64) && wide->equalsInt(4))
            return 0;
    }
    llvm::ConstantInt *value = ret_int(*fn);
    if (value != NULL && value->equalsInt(4))
        return 0;
    return fail("sizeof", "sizeof(int) is not 4");
}

static int check_comma(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());

    if (expect_sig("comma", module, i32, 0, NULL) != 0)
        return 1;
    llvm::ConstantInt *value = ret_int(*answer(module));
    if (value == NULL || !value->equalsInt(2))
        return fail("comma", "comma did not yield the right operand");
    return 0;
}

static int check_conditional(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    auto *phi = fn == NULL ? NULL : llvm::dyn_cast_or_null<llvm::PHINode>(ret_op(*fn));
    bool one = false;
    bool two = false;

    if (phi == NULL)
        return fail("conditional", "return is not a phi");
    for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
        one = one || const_eq(phi->getIncomingValue(i), 1);
        two = two || const_eq(phi->getIncomingValue(i), 2);
    }
    if (!one || !two)
        return fail("conditional", "phi is missing 1 or 2");
    return 0;
}

static int check_compare(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    auto *ext = llvm::dyn_cast_or_null<llvm::ZExtInst>(
        answer(module) == NULL ? NULL : ret_op(*answer(module)));

    if (expect_sig("compare", module, i32, 1, i32) != 0)
        return 1;
    if (ext == NULL || !llvm::isa<llvm::ICmpInst>(ext->getOperand(0)))
        return fail("compare", "return is not a comparison");
    return 0;
}

static int check_complement(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    auto *bin = llvm::dyn_cast_or_null<llvm::BinaryOperator>(
        answer(module) == NULL ? NULL : ret_op(*answer(module)));

    if (expect_sig("complement", module, i32, 1, i32) != 0)
        return 1;
    if (bin == NULL || bin->getOpcode() != llvm::Instruction::Xor)
        return fail("complement", "return is not a complement");
    auto *mask = llvm::dyn_cast<llvm::ConstantInt>(bin->getOperand(1));
    if (mask == NULL || !mask->isMinusOne())
        return fail("complement", "return is not a complement");
    return 0;
}

static int check_not(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    auto *ext = llvm::dyn_cast_or_null<llvm::ZExtInst>(
        answer(module) == NULL ? NULL : ret_op(*answer(module)));
    auto *bin = ext == NULL ? NULL
                            : llvm::dyn_cast<llvm::BinaryOperator>(ext->getOperand(0));

    if (expect_sig("not", module, i32, 1, i32) != 0)
        return 1;
    if (bin == NULL || bin->getOpcode() != llvm::Instruction::Xor
            || !bin->getType()->isIntegerTy(1))
        return fail("not", "return is not a logical not");
    return 0;
}

static int check_logic(const char *mode, llvm::Module &module, bool want_true)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    llvm::Function *fn = answer(module);
    auto *ext = fn == NULL ? NULL : llvm::dyn_cast_or_null<llvm::ZExtInst>(ret_op(*fn));
    auto *phi = ext == NULL ? NULL : llvm::dyn_cast<llvm::PHINode>(ext->getOperand(0));
    bool saw = false;
    bool branched = false;

    if (expect_sig(mode, module, i32, 2, i32) != 0)
        return 1;
    if (phi == NULL || !phi->getType()->isIntegerTy(1))
        return fail(mode, "missing a bool phi");
    for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
        if (const_eq(phi->getIncomingValue(i), want_true ? 1 : 0))
            saw = true;
    }
    for (llvm::BasicBlock &bb : *fn) {
        auto *br = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator());
        if (br != NULL && br->isConditional())
            branched = true;
    }
    if (!saw || !branched)
        return fail(mode, "not a short-circuit operator");
    return 0;
}

static int check_assign(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    llvm::Function *fn = answer(module);
    bool stored = false;

    if (expect_sig("assign", module, i32, 1, i32) != 0)
        return 1;
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && const_eq(store->getValueOperand(), 7))
            stored = true;
    }
    if (!stored)
        return fail("assign", "no store of 7");
    return 0;
}

static int check_add_assign(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    llvm::Function *fn = answer(module);

    if (expect_sig("add-assign", module, i32, 1, i32) != 0)
        return 1;
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        auto *add = store == NULL ? NULL
                                  : llvm::dyn_cast<llvm::BinaryOperator>(store->getValueOperand());
        if (add != NULL && add->getOpcode() == llvm::Instruction::Add
                && (const_eq(add->getOperand(0), 3) || const_eq(add->getOperand(1), 3)))
            return 0;
    }
    return fail("add-assign", "no store of an add by 3");
}

static int check_post(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    llvm::Function *fn = answer(module);
    auto *load = fn == NULL ? NULL : llvm::dyn_cast_or_null<llvm::LoadInst>(ret_op(*fn));
    bool added = false;

    if (expect_sig("post-inc", module, i32, 1, i32) != 0)
        return 1;
    if (load == NULL)
        return fail("post-inc", "postfix does not return the old value");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *add = llvm::dyn_cast<llvm::BinaryOperator>(&inst);
        if (add != NULL && add->getOpcode() == llvm::Instruction::Add)
            added = true;
    }
    if (!added)
        return fail("post-inc", "postfix does not update the variable");
    return 0;
}

static int check_pre(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    llvm::Function *fn = answer(module);
    auto *add = fn == NULL ? NULL : llvm::dyn_cast_or_null<llvm::BinaryOperator>(ret_op(*fn));
    bool stored = false;

    if (expect_sig("pre-inc", module, i32, 1, i32) != 0)
        return 1;
    if (add == NULL || add->getOpcode() != llvm::Instruction::Add)
        return fail("pre-inc", "prefix does not return the new value");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && store->getValueOperand() == add)
            stored = true;
    }
    if (!stored)
        return fail("pre-inc", "prefix does not store the new value");
    return 0;
}

static int check_shift_kind(const char *mode, llvm::Module &module,
                            llvm::Instruction::BinaryOps op)
{
    return ret_bin(mode, module, op, 1, 1);
}

static llvm::CallInst *direct_call(llvm::Function &fn)
{
    llvm::CallInst *found = NULL;

    for (llvm::Instruction &inst : llvm::instructions(fn)) {
        auto *call = llvm::dyn_cast<llvm::CallInst>(&inst);
        if (call == NULL || call->getCalledFunction() == NULL)
            continue;
        if (found != NULL)
            return NULL;
        found = call;
    }
    return found;
}

static int check_call(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    llvm::Function *inc = module.getFunction("inc");
    llvm::CallInst *call = fn == NULL ? NULL : direct_call(*fn);

    if (call == NULL || inc == NULL || call->getCalledFunction() != inc)
        return fail("call", "answer does not call inc");
    if (call->arg_size() != 1 || !const_eq(call->getArgOperand(0), 1))
        return fail("call", "inc is not called with 1");
    return 0;
}

static int check_overload(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    llvm::CallInst *call = fn == NULL ? NULL : direct_call(*fn);
    llvm::Function *chosen = call == NULL ? NULL : call->getCalledFunction();
    bool one = false;

    if (chosen == NULL || chosen->arg_size() != 2 || chosen->isVarArg())
        return fail("overload-call", "the call does not select the two-argument function");
    if (call->arg_size() != 2 || !const_eq(call->getArgOperand(0), 1)
        || !const_eq(call->getArgOperand(1), 2))
        return fail("overload-call", "the selected call is not sum(1, 2)");
    for (llvm::Function &other : module) {
        if (&other == fn || &other == chosen)
            continue;
        if (!other.isDeclaration() && other.arg_size() == 1)
            one = true;
    }
    if (!one)
        return fail("overload-call", "the one-argument overload is missing");
    return 0;
}

static int check_default(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    llvm::CallInst *call = fn == NULL ? NULL : direct_call(*fn);
    llvm::Function *callee = call == NULL ? NULL : call->getCalledFunction();

    if (callee == NULL || callee->arg_size() != 2 || callee->isVarArg())
        return fail("default-call", "the callee does not take two arguments");
    if (call->arg_size() != 2 || !const_eq(call->getArgOperand(0), 4)
        || !const_eq(call->getArgOperand(1), 3))
        return fail("default-call", "the default 3 was not passed at the call");
    return 0;
}

static int check_varargs(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    llvm::CallInst *call = fn == NULL ? NULL : direct_call(*fn);
    llvm::Function *callee = call == NULL ? NULL : call->getCalledFunction();

    if (callee == NULL || !callee->isVarArg())
        return fail("varargs", "the callee is not varargs");
    if (call->arg_size() != 2 || !const_eq(call->getArgOperand(0), 1))
        return fail("varargs", "the varargs call is missing its extra argument");
    return 0;
}

static int check_static(llvm::Module &module)
{
    llvm::Function *hidden = module.getFunction("hidden");
    llvm::Function *fn = answer(module);
    llvm::CallInst *call = fn == NULL ? NULL : direct_call(*fn);

    if (hidden == NULL || !hidden->hasInternalLinkage())
        return fail("static", "hidden does not have internal linkage");
    if (call == NULL || call->getCalledFunction() != hidden)
        return fail("static", "answer does not call hidden");
    return 0;
}

static int check_extern(llvm::Module &module)
{
    llvm::Function *extra = module.getFunction("extra");
    llvm::Function *fn = answer(module);
    llvm::CallInst *call = fn == NULL ? NULL : direct_call(*fn);

    if (extra == NULL || !extra->isDeclaration() || !extra->hasExternalLinkage())
        return fail("extern", "extra is not an external declaration");
    if (call == NULL || call->getCalledFunction() != extra
        || call->arg_size() != 1 || !const_eq(call->getArgOperand(0), 2))
        return fail("extern", "answer does not call extra(2)");
    return 0;
}

static int check_inline(llvm::Module &module)
{
    llvm::Function *twice = module.getFunction("twice");
    llvm::Function *fn = answer(module);
    llvm::CallInst *call = fn == NULL ? NULL : direct_call(*fn);

    if (twice == NULL || !twice->hasFnAttribute(llvm::Attribute::InlineHint))
        return fail("inline", "twice is not inline");
    if (call == NULL || call->getCalledFunction() != twice)
        return fail("inline", "answer does not call twice");
    return 0;
}

static int check_typedef(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());

    if (expect_sig("typedef", module, i32, 0, NULL) != 0)
        return 1;
    if (module.getGlobalVariable("number", true) != NULL || module.getFunction("number") != NULL)
        return fail("typedef", "the alias added a value");
    llvm::ConstantInt *value = ret_int(*answer(module));
    if (value == NULL || !value->equalsInt(6))
        return fail("typedef", "number did not lower to int");
    return 0;
}

static int check_string(llvm::Module &module)
{
    for (llvm::GlobalVariable &global : module.globals()) {
        if (!global.isConstant() || !global.hasInitializer())
            continue;
        auto *data = llvm::dyn_cast<llvm::ConstantDataArray>(global.getInitializer());
        if (data != NULL && data->isCString() && data->getAsCString() == "hi")
            return 0;
    }
    return fail("string", "no constant string hi");
}

static llvm::StructType *named_struct(llvm::Module &module, const char *name)
{
    for (llvm::StructType *ty : module.getIdentifiedStructTypes()) {
        if (ty->hasName() && ty->getName() == name)
            return ty;
    }
    return NULL;
}

static int check_struct(llvm::Module &module)
{
    llvm::StructType *point = named_struct(module, "struct.Point");
    llvm::Function *fn = answer(module);
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    bool stored = false;

    if (point == NULL || point->getNumElements() != 2)
        return fail("struct", "struct.Point is missing");
    if (point->elements()[0] != i32 || point->elements()[1] != i32)
        return fail("struct", "struct.Point fields are not i32");
    if (fn == NULL)
        return fail("struct", "no function answer");
    if (only_ret(*fn) == NULL || !llvm::isa<llvm::LoadInst>(only_ret(*fn)->getReturnValue()))
        return fail("struct", "return is not a load");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && const_eq(store->getValueOperand(), 1))
            stored = true;
    }
    if (!stored)
        return fail("struct", "no store of 1");
    return 0;
}

static int check_union(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    bool named = false;
    bool stored = false;

    for (llvm::StructType *ty : module.getIdentifiedStructTypes()) {
        if (ty->hasName() && ty->getName().starts_with("union."))
            named = true;
    }
    if (!named)
        return fail("union", "no union type");
    if (fn == NULL)
        return fail("union", "no function answer");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && const_eq(store->getValueOperand(), 3)
            && llvm::isa<llvm::AllocaInst>(store->getPointerOperand()))
            stored = true;
    }
    if (!stored)
        return fail("union", "the member store is not at the union address");
    return 0;
}

static int check_enum(llvm::Module &module)
{
    llvm::ConstantInt *value = answer(module) == NULL ? NULL : ret_int(*answer(module));

    if (value == NULL || !value->equalsInt(4))
        return fail("enum", "Green did not lower to 4");
    if (module.getGlobalVariable("Green", true) != NULL || module.getFunction("Green") != NULL)
        return fail("enum", "Green is a runtime value");
    return 0;
}

static int check_array(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    bool slot = false;
    bool stored = false;

    if (fn == NULL)
        return fail("array", "no function answer");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(&inst);
        if (alloca != NULL) {
            auto *array = llvm::dyn_cast<llvm::ArrayType>(alloca->getAllocatedType());
            if (array != NULL && array->getNumElements() == 3 && array->getElementType() == i32)
                slot = true;
        }
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && const_eq(store->getValueOperand(), 4))
            stored = true;
    }
    if (!slot || !stored)
        return fail("array", "no array of 3 int or no store of 4");
    return 0;
}

static int check_pointer(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    bool stored = false;

    if (fn == NULL)
        return fail("pointer", "no function answer");
    llvm::ReturnInst *ret = only_ret(*fn);
    auto *outer = ret == NULL ? NULL : llvm::dyn_cast<llvm::LoadInst>(ret->getReturnValue());
    auto *inner = outer == NULL ? NULL : llvm::dyn_cast<llvm::LoadInst>(outer->getPointerOperand());
    if (inner == NULL)
        return fail("pointer", "return is not a load through a pointer");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && const_eq(store->getValueOperand(), 5))
            stored = true;
    }
    if (!stored)
        return fail("pointer", "no store of 5");
    return 0;
}

static int check_array_param(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    llvm::Function *first = module.getFunction("first");
    llvm::CallInst *call = fn == NULL ? NULL : direct_call(*fn);

    if (first == NULL || first->arg_size() != 1)
        return fail("array-param", "first has no parameter");
    if (!first->getArg(0)->getType()->isPointerTy()
        || first->getFunctionType()->getParamType(0)->isArrayTy())
        return fail("array-param", "empty brackets did not lower to a pointer");
    if (call == NULL || call->getCalledFunction() != first)
        return fail("array-param", "answer does not call first");
    return 0;
}

static int check_bitfield(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    bool width = false;
    bool stored = false;

    for (llvm::StructType *ty : module.getIdentifiedStructTypes()) {
        for (llvm::Type *field : ty->elements()) {
            if (field->isIntegerTy(7))
                width = true;
        }
    }
    if (!width)
        return fail("bitfield", "no field of width 7");
    if (fn == NULL)
        return fail("bitfield", "no function answer");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && store->getValueOperand()->getType()->isIntegerTy(7))
            stored = true;
    }
    if (!stored)
        return fail("bitfield", "no store of the 7-bit field");
    return 0;
}

static bool gep_has_index(const llvm::Value *ptr, uint64_t want)
{
    auto *gep = llvm::dyn_cast<llvm::GetElementPtrInst>(ptr);

    if (gep == NULL)
        return false;
    for (const llvm::Use &index : gep->indices()) {
        auto *constant = llvm::dyn_cast<llvm::ConstantInt>(index.get());
        if (constant != NULL && constant->equalsInt(want))
            return true;
    }
    return false;
}

static int check_designated(llvm::Module &module)
{
    llvm::Function *fn = answer(module);

    if (fn == NULL)
        return fail("designated-init", "no function answer");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && const_eq(store->getValueOperand(), 2)
            && gep_has_index(store->getPointerOperand(), 1))
            return 0;
    }
    return fail("designated-init", "no store of 2 through field 1");
}

static int check_brace(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    bool one = false;
    bool two = false;

    if (fn == NULL)
        return fail("brace", "no function answer");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store == NULL)
            continue;
        if (const_eq(store->getValueOperand(), 1))
            one = true;
        if (const_eq(store->getValueOperand(), 2))
            two = true;
    }
    if (!one || !two)
        return fail("brace", "the brace list did not store 1 and 2");
    return 0;
}

static int reject_namespace_value(const char *mode, llvm::Module &module)
{
    if (module.getGlobalVariable("audio", true) != NULL || module.getFunction("audio") != NULL)
        return fail(mode, "the namespace is a runtime value");
    return 0;
}

static int pointer_this(const char *mode, llvm::Function *fn)
{
    if (fn == NULL || fn->empty() || fn->arg_size() < 1 || !fn->getArg(0)->getType()->isPointerTy())
        return fail(mode, "the method has no pointer parameter");
    return 0;
}

static int check_method(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    llvm::Function *read = module.getFunction("Point::read");
    llvm::CallInst *call = fn == NULL ? NULL : direct_call(*fn);

    if (pointer_this("method", read) != 0)
        return 1;
    if (call == NULL || call->getCalledFunction() != read)
        return fail("method", "the call is not direct");
    return 0;
}

static int check_const_method(llvm::Module &module)
{
    return pointer_this("const-method-call", module.getFunction("Box::read"));
}

static int check_ctor(llvm::Module &module)
{
    llvm::Function *ctor = module.getFunction("Point::Point");

    if (pointer_this("ctor", ctor) != 0)
        return 1;
    if (ctor->arg_size() != 2 || !ctor->getArg(1)->getType()->isIntegerTy(32))
        return fail("ctor", "the constructor parameters are wrong");
    for (llvm::Instruction &inst : llvm::instructions(*ctor)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && store->getValueOperand()->getType()->isIntegerTy(32))
            return 0;
    }
    return fail("ctor", "the constructor does not store the argument");
}

static int check_dtor(llvm::Module &module)
{
    llvm::Function *dtor = module.getFunction("Point::~Point");
    llvm::Function *fn = answer(module);

    if (pointer_this("dtor", dtor) != 0)
        return 1;
    if (fn == NULL)
        return fail("dtor", "no function answer");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        if (llvm::isa<llvm::CallInst>(inst))
            return fail("dtor", "delete calls the destructor");
    }
    return 0;
}

static bool stores_field(llvm::Function &fn, uint64_t index)
{
    for (llvm::Instruction &inst : llvm::instructions(fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store == NULL)
            continue;
        auto *gep = llvm::dyn_cast<llvm::GetElementPtrInst>(store->getPointerOperand());
        if (gep == NULL || gep->getNumIndices() < 2)
            continue;
        auto *field = llvm::dyn_cast<llvm::ConstantInt>(gep->getOperand(gep->getNumOperands() - 1));
        if (field != NULL && field->equalsInt(index))
            return true;
    }
    return false;
}

static int check_init_list(llvm::Module &module)
{
    llvm::Function *ctor = module.getFunction("Pair::Pair");

    if (pointer_this("init-list", ctor) != 0)
        return 1;
    if (!stores_field(*ctor, 0) || !stores_field(*ctor, 1))
        return fail("init-list", "the initializer list does not store both members");
    return 0;
}

static bool calls_direct(llvm::Function &fn, llvm::Function *target)
{
    if (target == NULL)
        return false;
    for (llvm::Instruction &inst : llvm::instructions(fn)) {
        auto *call = llvm::dyn_cast<llvm::CallInst>(&inst);
        if (call != NULL && call->getCalledFunction() == target)
            return true;
    }
    return false;
}

static int check_two_bases(llvm::Module &module)
{
    llvm::Function *ctor = module.getFunction("C::C");
    llvm::StructType *type = named_struct(module, "class.C");

    if (pointer_this("two-bases", ctor) != 0)
        return 1;
    if (type == NULL || type->getNumElements() != 2)
        return fail("two-bases", "the class does not contain both bases");
    if (!calls_direct(*ctor, module.getFunction("A::A"))
        || !calls_direct(*ctor, module.getFunction("B::B")))
        return fail("two-bases", "the constructor does not initialize both bases");
    return 0;
}

static int check_virtual_call(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    llvm::StructType *type = named_struct(module, "class.Point");
    bool indirect = false;

    if (fn == NULL)
        return fail("virtual-call", "no function answer");
    if (type == NULL || type->getNumElements() < 1 || !type->getElementType(0)->isPointerTy())
        return fail("virtual-call", "the object has no pointer");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *call = llvm::dyn_cast<llvm::CallInst>(&inst);
        if (call != NULL && call->getCalledFunction() == NULL)
            indirect = true;
    }
    if (!indirect)
        return fail("virtual-call", "the virtual call is not indirect");
    return 0;
}

static int check_base_call(llvm::Module &module)
{
    llvm::Function *read = module.getFunction("B::read");
    llvm::Function *value = module.getFunction("A::value");

    if (pointer_this("base-call", read) != 0)
        return 1;
    if (!calls_direct(*read, value))
        return fail("base-call", "Base::method is not a direct call");
    return 0;
}

static bool intrinsic_id(const llvm::CallInst *call, llvm::Intrinsic::ID id)
{
    if (call == NULL)
        return false;
    const llvm::Function *fn = call->getCalledFunction();
    return fn != NULL && fn->isIntrinsic() && fn->getIntrinsicID() == id;
}

static llvm::CallInst *find_call(llvm::Function &fn, llvm::Intrinsic::ID id)
{
    for (llvm::Instruction &inst : llvm::instructions(fn)) {
        auto *call = llvm::dyn_cast<llvm::CallInst>(&inst);
        if (intrinsic_id(call, id))
            return call;
    }
    return NULL;
}

static int check_alloc(const char *mode, llvm::Module &module, uint64_t count, bool constructor)
{
    llvm::Function *fn = answer(module);
    llvm::Function *alloc = module.getFunction("smallcpp_alloc");
    llvm::CallInst *state = fn == NULL ? NULL : find_call(*fn, llvm::Intrinsic::experimental_gc_statepoint);
    llvm::CallInst *result = fn == NULL ? NULL : find_call(*fn, llvm::Intrinsic::experimental_gc_result);
    llvm::CallInst *map = fn == NULL ? NULL : find_call(*fn, llvm::Intrinsic::experimental_stackmap);
    bool saw_alloc = false;
    bool saw_count = false;
    bool saw_result = false;

    if (alloc == NULL || !alloc->isDeclaration())
        return fail(mode, "the allocator is not a declaration");
    if (state == NULL)
        return fail(mode, "the allocation is not a safepoint");
    for (llvm::Value *arg : state->args()) {
        if (arg == alloc)
            saw_alloc = true;
        auto *constant = llvm::dyn_cast<llvm::ConstantInt>(arg);
        if (constant != NULL && constant->getBitWidth() == 64 && constant->equalsInt(count))
            saw_count = true;
    }
    if (!saw_alloc)
        return fail(mode, "the safepoint does not call the allocator");
    if (!saw_count)
        return fail(mode, "the allocation does not pass its size");
    if (result == NULL || map == NULL)
        return fail(mode, "the stack map is missing");
    for (llvm::Value *arg : map->args()) {
        if (arg == result)
            saw_result = true;
    }
    if (!saw_result)
        return fail(mode, "the returned pointer is not in the stack map");
    if (constructor) {
        llvm::Function *ctor = module.getFunction("Point::Point");
        if (!calls_direct(*fn, ctor))
            return fail(mode, "new does not call the constructor");
    }
    return 0;
}

static int check_new_object(llvm::Module &module)
{
    return check_alloc("new-object", module, 1, true);
}

static int check_new_array(llvm::Module &module)
{
    return check_alloc("new-array-call", module, 3, false);
}

static int check_new_pointer(llvm::Module &module)
{
    return check_alloc("new-pointer", module, 8, false);
}

static int check_delete(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    bool loaded = false;

    if (fn == NULL)
        return fail("delete-ptr", "no function answer");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        if (llvm::isa<llvm::CallInst>(inst))
            return fail("delete-ptr", "delete emits a call");
        if (llvm::isa<llvm::LoadInst>(inst))
            loaded = true;
    }
    if (!loaded)
        return fail("delete-ptr", "delete does not evaluate its operand");
    return 0;
}

static int check_nullptr(llvm::Module &module)
{
    llvm::Function *fn = answer(module);
    int nulls = 0;

    if (fn == NULL)
        return fail("nullptr", "no function answer");
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && llvm::isa<llvm::ConstantPointerNull>(store->getValueOperand()))
            nulls++;
    }
    if (nulls < 2)
        return fail("nullptr", "nullptr and NULL are not the null pointer");
    return 0;
}

static int check_namespace_fn(llvm::Module &module)
{
    llvm::Function *level = module.getFunction("audio::level");
    llvm::ConstantInt *value = level == NULL ? NULL : ret_int(*level);

    if (level == NULL || level->empty())
        return fail("namespace-fn", "no function in the namespace");
    if (value == NULL || !value->equalsInt(1))
        return fail("namespace-fn", "level did not return 1");
    return reject_namespace_value("namespace-fn", module);
}

static int check_named_call(const char *mode, llvm::Module &module, const char *callee)
{
    llvm::Function *fn = answer(module);
    llvm::Function *target = module.getFunction(callee);
    llvm::CallInst *call = fn == NULL ? NULL : direct_call(*fn);

    if (call == NULL || target == NULL || call->getCalledFunction() != target)
        return fail(mode, "the call did not resolve");
    if (call->arg_size() != 1 || !const_eq(call->getArgOperand(0), 1))
        return fail(mode, "the call argument is not 1");
    return reject_namespace_value(mode, module);
}

static int check_register(llvm::Module &module)
{
    llvm::Type *i32 = llvm::Type::getInt32Ty(module.getContext());
    llvm::Function *fn = answer(module);
    unsigned count = 0;
    bool stored = false;
    bool slot = false;

    if (expect_sig("register", module, i32, 0, NULL) != 0)
        return 1;
    for (llvm::Instruction &inst : llvm::instructions(*fn)) {
        count++;
        if (llvm::isa<llvm::CallInst>(inst))
            return fail("register", "register added a call");
        if (llvm::isa<llvm::AllocaInst>(inst))
            slot = true;
        auto *store = llvm::dyn_cast<llvm::StoreInst>(&inst);
        if (store != NULL && const_eq(store->getValueOperand(), 4))
            stored = true;
    }
    if (!slot || !stored || count != 4)
        return fail("register", "register did not lower like a local");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: verify_module <mode> <module.bc>\n");
        return 2;
    }

    const char *mode = argv[1];
    llvm::LLVMContext context;
    auto buffer = llvm::MemoryBuffer::getFile(argv[2]);

    if (!buffer) {
        fprintf(stderr, "%s: %s\n", argv[2], buffer.getError().message().c_str());
        return 1;
    }
    auto parsed = llvm::parseBitcodeFile(buffer.get()->getMemBufferRef(), context);
    if (!parsed) {
        fprintf(stderr, "%s: bitcode did not load\n", argv[2]);
        return 1;
    }
    std::unique_ptr<llvm::Module> module = std::move(parsed.get());
    std::string diagnostic;
    llvm::raw_string_ostream errors(diagnostic);
    if (llvm::verifyModule(*module, &errors)) {
        errors.flush();
        fprintf(stderr, "module failed verification: %s\n", diagnostic.c_str());
        return 1;
    }

    llvm::LLVMContext &ctx = module->getContext();
    if (strcmp(mode, "return") == 0)
        return check_return(*module);
    if (strcmp(mode, "if") == 0)
        return check_if(*module);
    if (strcmp(mode, "else") == 0)
        return check_else(*module);
    if (strcmp(mode, "while") == 0)
        return check_while(*module);
    if (strcmp(mode, "do") == 0)
        return check_do(*module);
    if (strcmp(mode, "for") == 0)
        return check_for("for", *module);
    if (strcmp(mode, "for-decl") == 0)
        return check_for("for-decl", *module);
    if (strcmp(mode, "switch") == 0)
        return check_switch(*module);
    if (strcmp(mode, "break") == 0)
        return check_break(*module);
    if (strcmp(mode, "continue") == 0)
        return check_continue(*module);
    if (strcmp(mode, "goto") == 0)
        return check_goto(*module);
    if (strcmp(mode, "void") == 0)
        return check_void(*module);
    if (strcmp(mode, "bool-type") == 0)
        return identity("bool-type", *module, llvm::Type::getInt1Ty(ctx));
    if (strcmp(mode, "char") == 0)
        return identity("char", *module, llvm::Type::getInt8Ty(ctx));
    if (strcmp(mode, "short") == 0)
        return identity("short", *module, llvm::Type::getInt16Ty(ctx));
    if (strcmp(mode, "long") == 0)
        return identity("long", *module, llvm::Type::getInt64Ty(ctx));
    if (strcmp(mode, "float") == 0)
        return identity("float", *module, llvm::Type::getFloatTy(ctx));
    if (strcmp(mode, "double") == 0)
        return identity("double", *module, llvm::Type::getDoubleTy(ctx));
    if (strcmp(mode, "signed") == 0)
        return check_shift_kind("signed", *module, llvm::Instruction::AShr);
    if (strcmp(mode, "unsigned") == 0)
        return check_shift_kind("unsigned", *module, llvm::Instruction::LShr);
    if (strcmp(mode, "empty-list") == 0)
        return check_empty(*module);
    if (strcmp(mode, "parameter") == 0)
        return check_parameter(*module);
    if (strcmp(mode, "local") == 0)
        return check_local(*module);
    if (strcmp(mode, "global") == 0)
        return check_global(*module);
    if (strcmp(mode, "const-var") == 0)
        return check_const(*module);
    if (strcmp(mode, "volatile") == 0)
        return check_volatile(*module);
    if (strcmp(mode, "true") == 0)
        return check_bool_const("true", *module, 1);
    if (strcmp(mode, "false") == 0)
        return check_bool_const("false", *module, 0);
    if (strcmp(mode, "literal") == 0)
        return check_literal(*module);
    if (strcmp(mode, "cast") == 0)
        return check_cast(*module);
    if (strcmp(mode, "sizeof") == 0)
        return check_sizeof(*module);
    if (strcmp(mode, "comma") == 0)
        return check_comma(*module);
    if (strcmp(mode, "conditional") == 0)
        return check_conditional(*module);
    if (strcmp(mode, "add") == 0)
        return ret_bin("add", *module, llvm::Instruction::Add, 1, 1);
    if (strcmp(mode, "sub") == 0)
        return ret_bin("sub", *module, llvm::Instruction::Sub, 1, 1);
    if (strcmp(mode, "mul") == 0)
        return ret_bin("mul", *module, llvm::Instruction::Mul, 1, 2);
    if (strcmp(mode, "div") == 0)
        return ret_bin("div", *module, llvm::Instruction::SDiv, 1, 2);
    if (strcmp(mode, "mod") == 0)
        return ret_bin("mod", *module, llvm::Instruction::SRem, 1, 2);
    if (strcmp(mode, "shift") == 0)
        return ret_bin("shift", *module, llvm::Instruction::Shl, 1, 1);
    if (strcmp(mode, "compare") == 0)
        return check_compare(*module);
    if (strcmp(mode, "bit-and") == 0)
        return ret_bin("bit-and", *module, llvm::Instruction::And, 1, 1);
    if (strcmp(mode, "bit-or") == 0)
        return ret_bin("bit-or", *module, llvm::Instruction::Or, 1, 1);
    if (strcmp(mode, "bit-xor") == 0)
        return ret_bin("bit-xor", *module, llvm::Instruction::Xor, 1, 1);
    if (strcmp(mode, "complement") == 0)
        return check_complement(*module);
    if (strcmp(mode, "negate") == 0)
        return ret_bin("negate", *module, llvm::Instruction::Sub, 0, 0);
    if (strcmp(mode, "not") == 0)
        return check_not(*module);
    if (strcmp(mode, "logical-and") == 0)
        return check_logic("logical-and", *module, false);
    if (strcmp(mode, "logical-or") == 0)
        return check_logic("logical-or", *module, true);
    if (strcmp(mode, "assign") == 0)
        return check_assign(*module);
    if (strcmp(mode, "add-assign") == 0)
        return check_add_assign(*module);
    if (strcmp(mode, "post-inc") == 0)
        return check_post(*module);
    if (strcmp(mode, "pre-inc") == 0)
        return check_pre(*module);
    if (strcmp(mode, "call") == 0)
        return check_call(*module);
    if (strcmp(mode, "overload-call") == 0)
        return check_overload(*module);
    if (strcmp(mode, "default-call") == 0)
        return check_default(*module);
    if (strcmp(mode, "varargs") == 0)
        return check_varargs(*module);
    if (strcmp(mode, "static") == 0)
        return check_static(*module);
    if (strcmp(mode, "extern") == 0)
        return check_extern(*module);
    if (strcmp(mode, "inline") == 0)
        return check_inline(*module);
    if (strcmp(mode, "typedef") == 0)
        return check_typedef(*module);
    if (strcmp(mode, "string") == 0)
        return check_string(*module);
    if (strcmp(mode, "register") == 0)
        return check_register(*module);
    if (strcmp(mode, "struct") == 0)
        return check_struct(*module);
    if (strcmp(mode, "union") == 0)
        return check_union(*module);
    if (strcmp(mode, "enum") == 0)
        return check_enum(*module);
    if (strcmp(mode, "array") == 0)
        return check_array(*module);
    if (strcmp(mode, "pointer") == 0)
        return check_pointer(*module);
    if (strcmp(mode, "array-param") == 0)
        return check_array_param(*module);
    if (strcmp(mode, "bitfield") == 0)
        return check_bitfield(*module);
    if (strcmp(mode, "designated-init") == 0)
        return check_designated(*module);
    if (strcmp(mode, "brace") == 0)
        return check_brace(*module);
    if (strcmp(mode, "namespace-fn") == 0)
        return check_namespace_fn(*module);
    if (strcmp(mode, "qualified-call") == 0)
        return check_named_call("qualified-call", *module, "audio::gain");
    if (strcmp(mode, "using-name") == 0)
        return check_named_call("using-name", *module, "audio::gain");
    if (strcmp(mode, "using-namespace") == 0)
        return check_named_call("using-namespace", *module, "audio::gain");
    if (strcmp(mode, "method") == 0)
        return check_method(*module);
    if (strcmp(mode, "const-method-call") == 0)
        return check_const_method(*module);
    if (strcmp(mode, "ctor") == 0)
        return check_ctor(*module);
    if (strcmp(mode, "dtor") == 0)
        return check_dtor(*module);
    if (strcmp(mode, "init-list") == 0)
        return check_init_list(*module);
    if (strcmp(mode, "two-bases") == 0)
        return check_two_bases(*module);
    if (strcmp(mode, "virtual-call") == 0)
        return check_virtual_call(*module);
    if (strcmp(mode, "base-call") == 0)
        return check_base_call(*module);
    if (strcmp(mode, "new-object") == 0)
        return check_new_object(*module);
    if (strcmp(mode, "new-array-call") == 0)
        return check_new_array(*module);
    if (strcmp(mode, "new-pointer") == 0)
        return check_new_pointer(*module);
    if (strcmp(mode, "delete-ptr") == 0)
        return check_delete(*module);
    if (strcmp(mode, "nullptr") == 0)
        return check_nullptr(*module);

    fprintf(stderr, "unknown mode %s\n", mode);
    return 2;
}
