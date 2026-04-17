#include "IndirectGlobalVariable.h"
#include "Utils.h"

#define DEBUG_TYPE "indgv"

void IndirectGlobalVariable::NumberGlobalVariable(Function &F) {
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    for (User::op_iterator op = (*I).op_begin(); op != (*I).op_end(); ++op) {
      Value *val = *op;
      if (GlobalVariable *GV = dyn_cast<GlobalVariable>(val)) {
        if (!GV->isThreadLocal() && GVNumbering.count(GV) == 0 &&
            !GV->isDLLImportDependent()) {
          GVNumbering[GV] = GlobalVariables.size();
          GlobalVariables.push_back((GlobalVariable *) val);
        }
      }
    }
  }
}

GlobalVariable *IndirectGlobalVariable::getIndirectGlobalVariables(Function &F, ConstantInt *EncKey) {
  std::string GVName(F.getName().str() + "_IndirectGVars");
  GlobalVariable *GV = F.getParent()->getNamedGlobal(GVName);
  if (GV)
    return GV;

  IntegerType *PtrIntTy = IntegerType::get(F.getContext(),
      F.getParent()->getDataLayout().getPointerSizeInBits(0));

  std::vector<Constant *> Elements;
  for (auto GVar:GlobalVariables) {
    Constant *CE = ConstantExpr::getPtrToInt(GVar, PtrIntTy);
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

bool IndirectGlobalVariable::runOnFunction(Function &Fn) {
  LLVMContext &Ctx = Fn.getContext();
  IntegerType *PtrIntTy = IntegerType::get(Ctx,
      Fn.getParent()->getDataLayout().getPointerSizeInBits(0));

  GVNumbering.clear();
  GlobalVariables.clear();

  LowerConstantExpr(Fn);
  NumberGlobalVariable(Fn);

  if (GlobalVariables.empty()) {
    return false;
  }

  uint64_t V = static_cast<uint64_t>(RandomEngine.get_uint32_t() | 1U);
  IntegerType* intType = Type::getInt32Ty(Ctx);

  ConstantInt *EncKey = ConstantInt::get(PtrIntTy, V, false);

  ConstantInt *Zero = ConstantInt::get(intType, 0);
  GlobalVariable *GVars = getIndirectGlobalVariables(Fn, EncKey);

  for (inst_iterator I = inst_begin(Fn), E = inst_end(Fn); I != E; ++I) {
    Instruction *Inst = &*I;
    if (isa<LandingPadInst>(Inst) || isa<CleanupPadInst>(Inst) ||
        isa<CatchPadInst>(Inst) || isa<CatchReturnInst>(Inst) ||
        isa<CatchSwitchInst>(Inst) || isa<ResumeInst>(Inst) || 
        isa<CallInst>(Inst)) {
      continue;
    }
    if (PHINode *PHI = dyn_cast<PHINode>(Inst)) {
      for (unsigned int i = 0; i < PHI->getNumIncomingValues(); ++i) {
        Value *val = PHI->getIncomingValue(i);
        if (GlobalVariable *GV = dyn_cast<GlobalVariable>(val)) {
          if (GVNumbering.count(GV) == 0) {
            continue;
          }

          Instruction *IP = PHI->getIncomingBlock(i)->getTerminator();
          IRBuilder<> IRB(IP);

          Value *Idx = ConstantInt::get(intType, GVNumbering[GV]);
          Value *GEP = IRB.CreateGEP(
              GVars->getValueType(),
              GVars,
              {Zero, Idx});
          LoadInst *EncGVAddr = IRB.CreateLoad(
              PtrIntTy, GEP,
              GV->getName());

          Value *GVAddr = IRB.CreateSub(EncGVAddr, EncKey);
          GVAddr = IRB.CreateIntToPtr(GVAddr, GV->getType());
          GVAddr->setName("IndGV0_");
          PHI->setIncomingValue(i, GVAddr);
        }
      }
    } else {
      for (User::op_iterator op = Inst->op_begin(); op != Inst->op_end(); ++op) {
        if (GlobalVariable *GV = dyn_cast<GlobalVariable>(*op)) {
          if (GVNumbering.count(GV) == 0) {
            continue;
          }

          IRBuilder<> IRB(Inst);
          Value *Idx = ConstantInt::get(intType, GVNumbering[GV]);
          Value *GEP = IRB.CreateGEP(
              GVars->getValueType(),
              GVars,
              {Zero, Idx});
          LoadInst *EncGVAddr = IRB.CreateLoad(
              PtrIntTy,
              GEP,
              GV->getName());

          Value *GVAddr = IRB.CreateSub(EncGVAddr, EncKey);
          GVAddr = IRB.CreateIntToPtr(GVAddr, GV->getType());
          GVAddr->setName("IndGV1_");
          Inst->replaceUsesOfWith(GV, GVAddr);
        }
      }
    }
  }

    return true;
  }
