#ifndef _FUNCTION_WRAPPER_H_
#define _FUNCTION_WRAPPER_H_

#include "common.h"
#include "CallSite.h"

struct FunctionWrapper {
	uint32_t ProbRate; // fw_prob=30
  uint32_t ObfTimes; // fw_times=2
  SmallVector<CallSite *, 16> callsites;
  
  bool runOnFunction(Function &F);
  CallSite *HandleCallSite(CallSite *CS);
};

#endif
