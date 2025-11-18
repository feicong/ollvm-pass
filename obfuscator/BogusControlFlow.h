//===- BogusControlFlow.h - BogusControlFlow Obfuscation pass-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------------------===//
//
// This file contains includes and defines for the bogusControlFlow pass
//
//===--------------------------------------------------------------------------------===//

#ifndef _BOGUSCONTROLFLOW_H_
#define _BOGUSCONTROLFLOW_H_

#include "common.h"

struct BogusControlFlow {
  int ObfProbRate;
  int ObfTimes;
  int ConditionExpressionComplexity;

  bool runOnFunction(Function &F);
  void bogus(Function &F);
  void addBogusFlow(BasicBlock * basicBlock, Function &F);
  BasicBlock* createAlteredBasicBlock(BasicBlock * basicBlock, const Twine &  Name = "gen", Function * F = 0);
  bool doF(Module &M);
};

#endif

