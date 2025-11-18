#ifndef _SPLIT_INCLUDES_
#define _SPLIT_INCLUDES_

#include "common.h"

struct SplitBasicBlock {
  int SplitNum;

  bool runOnFunction(Function &F);
  void split(Function *f);
  bool containsPHI(BasicBlock *b);
  void shuffle(std::vector<int> &vec);
};

#endif

