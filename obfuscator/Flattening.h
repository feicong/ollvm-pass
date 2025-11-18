#ifndef _FLATTENING_INCLUDES_
#define _FLATTENING_INCLUDES_

#include "common.h"

struct Flattening {
  bool runOnFunction(Function &F);
  bool flatten(Function *f);
};

#endif

