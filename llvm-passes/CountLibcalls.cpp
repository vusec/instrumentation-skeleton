#define DEBUG_TYPE "count-libcalls"

#include "utils/Common.h"
#include "utils/CustomFunctionPass.h"
#include "include/noinstrument.h"

using namespace llvm;

namespace {

STATISTIC(NLibcalls, "Number of instrumented library calls");

struct CountLibcalls : public CustomFunctionPass {
    static char ID;
    CountLibcalls() : CustomFunctionPass(ID) {}

private:
    Function *CountFunc;

    bool initializeModule(Module &M) override {
        Type *VoidTy = Type::getVoidTy(M.getContext());
        Type *StringTy = Type::getIntNPtrTy(M.getContext(), 8);
        FunctionType *FnTy = FunctionType::get(VoidTy, {StringTy}, false);
        CountFunc = cast<Function>(M.getOrInsertFunction(NOINSTRUMENT_PREFIX "count_libcall", FnTy));
        return false;
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
