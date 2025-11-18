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
  int ObfProbRate; // bcf_prob:70
  int ObfTimes; // bcf_loop:1
  int ConditionExpressionComplexity; // bcf_cond_compl:3
  bool OnlyJunkAssembly; // bcf_onlyjunkasm:false
  bool JunkAssembly; // bcf_junkasm:false
  int MaxNumberOfJunkAssembly; // bcf_junkasm_maxnum:4
  int MinNumberOfJunkAssembly; // bcf_junkasm_minnum:2
  bool CreateFunctionForOpaquePredicate; // bcf_createfunc:false
  SmallVector<const ICmpInst *, 8> needtoedit;

  bool runOnFunction(Function &F);
  void bogus(Function &F);
  bool containsCoroBeginInst(BasicBlock *b);
  bool containsMustTailCall(BasicBlock *b);
  bool containsSwiftError(BasicBlock *b);
  void addBogusFlow(BasicBlock *basicBlock, Function &F);
  BasicBlock *createAlteredBasicBlock(BasicBlock *basicBlock, const Twine &Name = "gen", Function *F = nullptr);
  bool doF(Function &F);
};

#endif
