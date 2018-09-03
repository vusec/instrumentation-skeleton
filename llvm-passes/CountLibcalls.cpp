#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/CallSite.h>

#define DEBUG_TYPE "count-libcalls"

#include "builtin/Utils/ModuleFunctionPass.h"
#include "builtin/Utils/NoInstrument.h"

using namespace llvm;

namespace {

STATISTIC(NLibcalls, "Number of instrumented library calls");

struct CountLibcalls : public ModuleFunctionPass {
    static char ID;
    CountLibcalls() : ModuleFunctionPass(ID) {}

private:
    Function *CountFunc;

    bool initializeModule(Module &M) override {
        // For an LTO pass, the helper should exist
        if ((CountFunc = M.getFunction(NOINSTRUMENT_PREFIX "count_libcall")))
            return false;

        // We also support compile-time instrumentation by inserting the
        // function signature if it does not exist
        Type *VoidTy = Type::getVoidTy(M.getContext());
        Type *StringTy = Type::getIntNPtrTy(M.getContext(), 8);
        FunctionType *FnTy = FunctionType::get(VoidTy, {StringTy}, false);
        CountFunc = Function::Create(FnTy, GlobalValue::ExternalLinkage,
                                     NOINSTRUMENT_PREFIX "count_libcall", &M);
        return true;
    }

    bool runOnFunction(Function &F) override {
        SmallVector<std::pair<Instruction*, StringRef>, 16> CallSites;

        for (Instruction &I : instructions(F)) {
            CallSite CS(&I);
            if (CS) {
                Function *Target = CS.getCalledFunction();
                if (Target && Target->isDeclaration() && !Target->isIntrinsic() && !isNoInstrument(Target))
                    CallSites.push_back(std::make_pair(&I, Target->getName()));
            }
        }

        IRBuilder<> B(F.getContext());

        for (auto &it : CallSites) {
            Instruction *I = it.first;
            StringRef Name = it.second;
            B.SetInsertPoint(I);
            B.CreateCall(CountFunc, {B.CreateGlobalStringPtr(Name)});
            NLibcalls++;
        }

        return !CallSites.empty();
    }
};

}

char CountLibcalls::ID = 0;
static RegisterPass<CountLibcalls> X("count-libcalls",
        "Count library calls, printing a summary to stderr at program exit");
