#include "IndirectCall.h"

#define DEBUG_TYPE "icall"

void IndirectCall::NumberCallees(Function &F) {
  for (auto &BB:F) {
    for (auto &I:BB) {
      if (dyn_cast<CallInst>(&I)) {
        CallBase *CB = dyn_cast<CallBase>(&I);
        Function *Callee = CB->getCalledFunction();
        if (Callee == nullptr) {
          continue;
        }
        if (Callee->isIntrinsic()) {
          continue;
        }
        if (Callee->isDeclaration()) {
          continue;
        }
        CallSites.push_back((CallInst *) &I);
        if (CalleeNumbering.count(Callee) == 0) {
          CalleeNumbering[Callee] = Callees.size();
          Callees.push_back(Callee);
        }
      }
    }
  }
}

GlobalVariable *IndirectCall::getIndirectCallees(Function &F, ConstantInt *EncKey) {
  std::string GVName(F.getName().str() + "_IndirectCallees");
  GlobalVariable *GV = F.getParent()->getNamedGlobal(GVName);
  if (GV)
    return GV;

  IntegerType *PtrIntTy = IntegerType::get(F.getContext(),
      F.getParent()->getDataLayout().getPointerSizeInBits(0));

  // callee's address
  std::vector<Constant *> Elements;
  for (auto Callee:Callees) {
    Constant *CE = ConstantExpr::getPtrToInt(Callee, PtrIntTy);
    CE = ConstantExpr::getAdd(CE, EncKey);
    Elements.push_back(CE);
  }

  ArrayType *ATy = ArrayType::get(PtrIntTy, Elements.size());
  Constant *CA = ConstantArray::get(ATy, ArrayRef<Constant *>(Elements));
  GV = new GlobalVariable(*F.getParent(), ATy, false, GlobalValue::LinkageTypes::PrivateLinkage,
                                              CA, GVName);
  appendToCompilerUsed(*F.getParent(), {GV});
  return GV;
}

bool IndirectCall::runOnFunction(Function &Fn) {
  LLVMContext &Ctx = Fn.getContext();
  IntegerType *PtrIntTy = IntegerType::get(Ctx,
      Fn.getParent()->getDataLayout().getPointerSizeInBits(0));

  CalleeNumbering.clear();
  Callees.clear();
  CallSites.clear();

  NumberCallees(Fn);

  if (Callees.empty()) {
    return false;
  }

  uint64_t V = static_cast<uint64_t>(RandomEngine.get_uint32_t() | 1U);
  IntegerType *intType = Type::getInt32Ty(Ctx);
  ConstantInt *EncKey = ConstantInt::get(PtrIntTy, V, false);

  ConstantInt *Zero = ConstantInt::get(intType, 0);
  GlobalVariable *Targets = getIndirectCallees(Fn, EncKey);

  for (auto CI : CallSites) {
    SmallVector<Value *, 8> Args;
    SmallVector<AttributeSet, 8> ArgAttrVec;

    CallBase *CB = CI;

    Function *Callee = CB->getCalledFunction();
    FunctionType *FTy = CB->getFunctionType();
    IRBuilder<> IRB(CB);

    Args.clear();
    ArgAttrVec.clear();

    Value *Idx = ConstantInt::get(intType, CalleeNumbering[CB->getCalledFunction()]);
    Value *GEP = IRB.CreateGEP(
        Targets->getValueType(), Targets,
        {Zero, Idx});
    LoadInst *EncDestAddr = IRB.CreateLoad(
        PtrIntTy, GEP,
        CI->getName());

    const AttributeList &CallPAL = CB->getAttributes();
    auto I = CB->arg_begin();
    unsigned i = 0;

    for (unsigned e = FTy->getNumParams(); i != e; ++I, ++i) {
      Args.push_back(*I);
      AttributeSet Attrs = CallPAL.getParamAttrs(i);
      ArgAttrVec.push_back(Attrs);
    }

    for (auto E = CB->arg_end(); I != E; ++I, ++i) {
      Args.push_back(*I);
      ArgAttrVec.push_back(CallPAL.getParamAttrs(i));
    }

    Value *DestAddr = IRB.CreateSub(EncDestAddr, EncKey);
    Value *FnPtr = IRB.CreateIntToPtr(DestAddr, FTy->getPointerTo());
    FnPtr->setName("Call_" + Callee->getName());
    CB->setCalledOperand(FnPtr);
  }

  return true;
}
