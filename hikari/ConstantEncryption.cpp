/*
    LLVM ConstantEncryption Pass
    Copyright (C) 2017 Zhang(http://mayuyu.io)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ConstantEncryption.h"
#include "CryptoUtils.h"
#include "SubstituteImpl.h"
#include "Utils.h"
#include "CallSite.h"

bool ConstantEncryption::initialize(Module &M) {
  dispatchonce = M.getFunction("dispatch_once");
  return true;
}

bool ConstantEncryption::shouldEncryptConstant(Instruction *I) {
  if (isa<SwitchInst>(I) || isa<IntrinsicInst>(I) ||
      isa<GetElementPtrInst>(I) || isa<PHINode>(I) || I->isAtomic())
    return false;
  if (AllocaInst *AI = dyn_cast<AllocaInst>(I))
    if (AI->isSwiftError())
      return false;
  if (isa<CallInst>(I) || isa<InvokeInst>(I)) {
    CallSite CS(I);
    if (CS.getCalledFunction() &&
#if LLVM_VERSION_MAJOR >= 18
        CS.getCalledFunction()->getName().starts_with("hikari_")) {
#else
        CS.getCalledFunction()->getName().startswith("hikari_")) {
#endif
      return false;
    }
  }
  if (dispatchonce)
    if (AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
      if (AI->getAllocatedType()->isIntegerTy())
        for (User *U : AI->users())
          if (LoadInst *LI = dyn_cast<LoadInst>(U))
            for (User *LU : LI->users())
              if (isa<CallInst>(LU) || isa<InvokeInst>(LU)) {
                CallSite CS(LU);
                Value *calledFunction = CS.getCalledFunction();
                if (!calledFunction)
                  calledFunction = CS.getCalledValue()->stripPointerCasts();
                if (!calledFunction ||
                    (!isa<ConstantExpr>(calledFunction) &&
                      !isa<Function>(calledFunction)) ||
                    CS.getIntrinsicID() != Intrinsic::not_intrinsic)
                  continue;
                if (calledFunction->getName() == "_dispatch_once" ||
                    calledFunction->getName() == "dispatch_once")
                  return false;
              }
    }
  return true;
}

bool ConstantEncryption::runOnFunction(Function& F) {
  FixFunctionConstantExpr(&F);
  uint32_t times = ObfTimes;
  while (times) {
    EncryptConstants(F);
    if (ConstToGV) {
      Constant2GlobalVariable(F);
    }
    times--;
  }
  return true;
}

bool ConstantEncryption::isDispatchOnceToken(GlobalVariable *GV) {
  if (!dispatchonce)
    return false;
  for (User *U : GV->users()) {
    if (isa<CallInst>(U) || isa<InvokeInst>(U)) {
      CallSite CS(U);
      Value *calledFunction = CS.getCalledFunction();
      if (!calledFunction)
        calledFunction = CS.getCalledValue()->stripPointerCasts();
      if (!calledFunction ||
          (!isa<ConstantExpr>(calledFunction) &&
            !isa<Function>(calledFunction)) ||
          CS.getIntrinsicID() != Intrinsic::not_intrinsic)
        continue;
      if (calledFunction->getName() == "_dispatch_once" ||
          calledFunction->getName() == "dispatch_once") {
        Value *onceToken = U->getOperand(0);
        if (dyn_cast_or_null<GlobalVariable>(
                onceToken->stripPointerCasts()) == GV)
          return true;
      }
    }
    if (StoreInst *SI = dyn_cast<StoreInst>(U))
      for (User *SU : SI->getPointerOperand()->users())
        if (LoadInst *LI = dyn_cast<LoadInst>(SU))
          for (User *LU : LI->users())
            if (isa<CallInst>(LU) || isa<InvokeInst>(LU)) {
              CallSite CS(LU);
              Value *calledFunction = CS.getCalledFunction();
              if (!calledFunction)
                calledFunction = CS.getCalledValue()->stripPointerCasts();
              if (!calledFunction ||
                  (!isa<ConstantExpr>(calledFunction) &&
                    !isa<Function>(calledFunction)) ||
                  CS.getIntrinsicID() != Intrinsic::not_intrinsic)
                continue;
              if (calledFunction->getName() == "_dispatch_once" ||
                  calledFunction->getName() == "dispatch_once")
                return true;
            }
  }
  return false;
}

bool ConstantEncryption::isAtomicLoaded(GlobalVariable *GV) {
  for (User *U : GV->users()) {
    if (LoadInst *LI = dyn_cast<LoadInst>(U)) {
      if (LI->isAtomic())
        return true;
    }
  }
  return false;
}

void ConstantEncryption::EncryptConstants(Function &F) {
  for (Instruction &I : instructions(F)) {
    if (!shouldEncryptConstant(&I))
      continue;
    CallInst *CI = dyn_cast<CallInst>(&I);
    for (unsigned i = 0; i < I.getNumOperands(); i++) {
      if (CI && CI->isBundleOperand(i))
        continue;
      Value *Op = I.getOperand(i);
      if (isa<ConstantInt>(Op))
        HandleConstantIntOperand(&I, i);
      if (GlobalVariable *G = dyn_cast<GlobalVariable>(Op))
        if (G->hasInitializer() &&
            (G->hasPrivateLinkage() || G->hasInternalLinkage()) &&
            isa<ConstantInt>(G->getInitializer()))
          HandleConstantIntInitializerGV(G);
    }
  }
}

void ConstantEncryption::Constant2GlobalVariable(Function &F) {
  Module &M = *F.getParent();
  const DataLayout &DL = M.getDataLayout();
  for (Instruction &I : instructions(F)) {
    if (!shouldEncryptConstant(&I))
      continue;
    CallInst *CI = dyn_cast<CallInst>(&I);
    InvokeInst *II = dyn_cast<InvokeInst>(&I);
    for (unsigned int i = 0; i < I.getNumOperands(); i++) {
      if (CI && CI->isBundleOperand(i))
        continue;
      if (II && II->isBundleOperand(i))
        continue;
      if (ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand(i))) {
        if (!(cryptoutils->get_range(100) <= ConstToGVProb))
          continue;
        GlobalVariable *GV = new GlobalVariable(
            *F.getParent(), CI->getType(), false,
            GlobalValue::LinkageTypes::PrivateLinkage,
            ConstantInt::get(CI->getType(), CI->getValue()), "CToGV");
        appendToCompilerUsed(*F.getParent(), GV);
        I.setOperand(i, new LoadInst(GV->getValueType(), GV, "", &I));
      }
    }
  }
  for (Instruction &I : instructions(F)) {
    if (!shouldEncryptConstant(&I))
      continue;
    if (BinaryOperator *BO = dyn_cast<BinaryOperator>(&I)) {
      if (!BO->getType()->isIntegerTy())
        continue;
      if (!(cryptoutils->get_range(100) <= ConstToGVProb))
        continue;
      IntegerType *IT = cast<IntegerType>(BO->getType());
      uint64_t dummy;
      if (IT == Type::getInt8Ty(IT->getContext()))
        dummy = cryptoutils->get_uint8_t();
      else if (IT == Type::getInt16Ty(IT->getContext()))
        dummy = cryptoutils->get_uint16_t();
      else if (IT == Type::getInt32Ty(IT->getContext()))
        dummy = cryptoutils->get_uint32_t();
      else if (IT == Type::getInt64Ty(IT->getContext()))
        dummy = cryptoutils->get_uint64_t();
      else
        continue;
      GlobalVariable *GV = new GlobalVariable(
          M, BO->getType(), false, GlobalValue::LinkageTypes::PrivateLinkage,
          ConstantInt::get(BO->getType(), dummy), "CToGV");
      StoreInst *SI =
          new StoreInst(BO, GV, false, DL.getABITypeAlign(BO->getType()));
      SI->insertAfter(BO);
      LoadInst *LI = new LoadInst(GV->getValueType(), GV, "", false,
                                  DL.getABITypeAlign(BO->getType()));
      LI->insertAfter(SI);
      BO->replaceUsesWithIf(LI, [SI](Use &U) { return U.getUser() != SI; });
    }
  }
}

void ConstantEncryption::HandleConstantIntInitializerGV(GlobalVariable *GVPtr) {
  if (!(AreUsersInOneFunction(GVPtr)) || isDispatchOnceToken(GVPtr) ||
      isAtomicLoaded(GVPtr))
    return;
  // Prepare Types and Keys
  std::pair<ConstantInt *, ConstantInt *> keyandnew;
  ConstantInt *Old = dyn_cast<ConstantInt>(GVPtr->getInitializer());
  bool hasHandled = true;
  if (handled_gvs.find(GVPtr) == handled_gvs.end()) {
    hasHandled = false;
    keyandnew = PairConstantInt(Old);
    handled_gvs.insert(GVPtr);
  }
  ConstantInt *XORKey = keyandnew.first;
  ConstantInt *newGVInit = keyandnew.second;
  if (hasHandled || !XORKey || !newGVInit)
    return;
  GVPtr->setInitializer(newGVInit);
  bool isSigned = XORKey->getValue().isSignBitSet() ||
                  newGVInit->getValue().isSignBitSet() ||
                  Old->getValue().isSignBitSet();
  for (User *U : GVPtr->users()) {
    BinaryOperator *XORInst = nullptr;
    if (LoadInst *LI = dyn_cast<LoadInst>(U)) {
      if (LI->getType() != XORKey->getType()) {
        Instruction *IntegerCast =
            BitCastInst::CreateIntegerCast(LI, XORKey->getType(), isSigned);
        IntegerCast->insertAfter(LI);
        XORInst =
            BinaryOperator::Create(Instruction::Xor, IntegerCast, XORKey);
        XORInst->insertAfter(IntegerCast);
        Instruction *IntegerCast2 =
            BitCastInst::CreateIntegerCast(XORInst, LI->getType(), isSigned);
        IntegerCast2->insertAfter(XORInst);
        LI->replaceUsesWithIf(IntegerCast2, [IntegerCast](Use &U) {
          return U.getUser() != IntegerCast;
        });
      } else {
        XORInst = BinaryOperator::Create(Instruction::Xor, LI, XORKey);
        XORInst->insertAfter(LI);
        LI->replaceUsesWithIf(
            XORInst, [XORInst](Use &U) { return U.getUser() != XORInst; });
      }
    } else if (StoreInst *SI = dyn_cast<StoreInst>(U)) {
      XORInst = BinaryOperator::Create(Instruction::Xor, SI->getOperand(0),
                                        XORKey, "", SI);
      SI->replaceUsesOfWith(SI->getValueOperand(), XORInst);
    }
    if (XORInst && SubstituteXor &&
        cryptoutils->get_range(100) <= SubstituteXorProb)
      SubstituteImpl::substituteXor(XORInst);
  }
}

void ConstantEncryption::HandleConstantIntOperand(Instruction *I, unsigned opindex) {
  std::pair<ConstantInt * /*key*/, ConstantInt * /*new*/> keyandnew =
      PairConstantInt(cast<ConstantInt>(I->getOperand(opindex)));
  ConstantInt *Key = keyandnew.first;
  ConstantInt *New = keyandnew.second;
  if (!Key || !New)
    return;
  BinaryOperator *NewOperand =
      BinaryOperator::Create(Instruction::Xor, New, Key, "", I);

  I->setOperand(opindex, NewOperand);
  if (SubstituteXor &&
      cryptoutils->get_range(100) <= SubstituteXorProb)
    SubstituteImpl::substituteXor(NewOperand);
}

std::pair<ConstantInt * /*key*/, ConstantInt * /*new*/>
ConstantEncryption::PairConstantInt(ConstantInt *C) {
  if (!C)
    return std::make_pair(nullptr, nullptr);
  IntegerType *IT = cast<IntegerType>(C->getType());
  uint64_t K;
  if (IT == Type::getInt1Ty(IT->getContext()) ||
      IT == Type::getInt8Ty(IT->getContext()))
    K = cryptoutils->get_uint8_t();
  else if (IT == Type::getInt16Ty(IT->getContext()))
    K = cryptoutils->get_uint16_t();
  else if (IT == Type::getInt32Ty(IT->getContext()))
    K = cryptoutils->get_uint32_t();
  else if (IT == Type::getInt64Ty(IT->getContext()))
    K = cryptoutils->get_uint64_t();
  else
    return std::make_pair(nullptr, nullptr);
  ConstantInt *CI =
      cast<ConstantInt>(ConstantInt::get(IT, K ^ C->getValue()));
  return std::make_pair(ConstantInt::get(IT, K), CI);
}

