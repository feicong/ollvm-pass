//===- FlatteningIncludes.h - Flattening Obfuscation pass------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains includes and defines for the split basicblock pass
//
//===----------------------------------------------------------------------===//

#ifndef _SPLIT_INCLUDES_
#define _SPLIT_INCLUDES_

#include "common.h"

struct SplitBasicBlock {
  int SplitNum; // split_num:2

  bool runOnFunction(Function &F);
  void split(Function *f);
  bool containsPHI(BasicBlock *b);
  bool containsSwiftError(BasicBlock *BB);
  void split_point_shuffle(SmallVector<size_t, 32> &vec);
};

#endif
