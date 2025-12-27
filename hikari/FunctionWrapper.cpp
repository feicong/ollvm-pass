// For open-source license, please refer to
// [License](https://github.com/HikariObfuscator/Hikari/wiki/License).
//===----------------------------------------------------------------------===//

#include "FunctionWrapper.h"
#include "CryptoUtils.h"

bool FunctionWrapper::runOnFunction(Function &F) {
  vector<Instruction*> insts; // 修复崩溃
  for (Instruction &Inst : instructions(F))
    if ((isa<CallInst>(&Inst) || isa<InvokeInst>(&Inst))) {
      CallBase* CB = cast<CallBase>(&Inst);
      Function* Callee = CB->getCalledFunction();
      if (Callee == nullptr) {
        continue;
      }
      StringRef fName = Callee->getName();
      if (fName.starts_with("llvm.")) {
        continue;
      }
      if (cryptoutils->get_range(100) <= ProbRate) {
        insts.push_back(&Inst); // 修复崩溃
      }
    }
  for (Instruction* Inst : insts) { // 修复崩溃
    CallSite* CS = new CallSite(Inst);
    for (uint32_t i = 0; i < ObfTimes && CS != nullptr; i++) {
      CS = HandleCallSite(CS);
    }
  }
  return true;
} // End of runOnModule

CallSite *FunctionWrapper::HandleCallSite(CallSite *CS) {
  Value *calledFunction = CS->getCalledFunction();
  if (calledFunction == nullptr)
    calledFunction = CS->getCalledValue()->stripPointerCasts();
  // Filter out IndirectCalls that depends on the context
  // Otherwise It'll be blantantly troublesome since you can't reference an
  // Instruction outside its BB  Too much trouble for a hobby project
  // To be precise, we only keep CS that refers to a non-intrinsic function
  // either directly or through casting
  if (calledFunction == nullptr ||
      (!isa<ConstantExpr>(calledFunction) &&
        !isa<Function>(calledFunction)) ||
      CS->getIntrinsicID() != Intrinsic::not_intrinsic)
    return nullptr;
  SmallVector<unsigned int, 8> byvalArgNums;
  if (Function *tmp = dyn_cast<Function>(calledFunction)) {
#if LLVM_VERSION_MAJOR >= 18
    if (tmp->getName().starts_with("clang.")) {
#else
    if (tmp->getName().startswith("clang.")) {
#endif
      // Clang Intrinsic
      return nullptr;
    }
    for (Argument &arg : tmp->args()) {
      if (arg.hasStructRetAttr() || arg.hasSwiftSelfAttr()) {
        return nullptr;
      }
      if (arg.hasByValAttr())
        byvalArgNums.emplace_back(arg.getArgNo());
    }
  }
  // Create a new function which in turn calls the actual function
  SmallVector<Type *, 8> types;
  for (unsigned int i = 0; i < CS->getNumArgOperands(); i++)
    types.emplace_back(CS->getArgOperand(i)->getType());
  FunctionType *ft =
      FunctionType::get(CS->getType(), ArrayRef<Type *>(types), false);
  Function *func =
      Function::Create(ft, GlobalValue::LinkageTypes::InternalLinkage,
                        "HikariFunctionWrapper", CS->getParent()->getModule());
  func->setCallingConv(CS->getCallingConv());
  // Trolling was all fun and shit so old implementation forced this symbol to
  // exist in all objects
  appendToCompilerUsed(*func->getParent(), {func});
  BasicBlock *BB = BasicBlock::Create(func->getContext(), "", func);
  SmallVector<Value *, 8> params;
  if (byvalArgNums.empty())
    for (Argument &arg : func->args())
      params.emplace_back(&arg);
  else
    for (Argument &arg : func->args()) {
      if (std::find(byvalArgNums.begin(), byvalArgNums.end(),
                    arg.getArgNo()) != byvalArgNums.end()) {
        params.emplace_back(&arg);
      } else {
        AllocaInst *AI = nullptr;
        if (!BB->empty()) {
          BasicBlock::iterator InsertPoint = BB->begin();
          while (isa<AllocaInst>(InsertPoint))
            ++InsertPoint;
          AI = new AllocaInst(arg.getType(), 0, "", &*InsertPoint);
        } else
          AI = new AllocaInst(arg.getType(), 0, "", BB);
        new StoreInst(&arg, AI, BB);
        LoadInst *LI = new LoadInst(AI->getAllocatedType(), AI, "", BB);
        params.emplace_back(LI);
      }
    }
  Value *retval = CallInst::Create(
      CS->getFunctionType(),
      ConstantExpr::getBitCast(cast<Function>(calledFunction),
                                CS->getCalledValue()->getType()),
#if LLVM_VERSION_MAJOR >= 16
      ArrayRef<Value *>(params), std::nullopt, "", BB);
#else
      ArrayRef<Value *>(params), None, "", BB);
#endif
  if (ft->getReturnType()->isVoidTy()) {
    ReturnInst::Create(BB->getContext(), BB);
  } else {
    ReturnInst::Create(BB->getContext(), retval, BB);
  }
  CS->setCalledFunction(func);
  CS->mutateFunctionType(ft);
  Instruction *Inst = CS->getInstruction();
  delete CS;
  return new CallSite(Inst);
}

