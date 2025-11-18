#ifndef _ANTI_DEBUGGING_H_
#define _ANTI_DEBUGGING_H_

#include "common.h"

struct AntiDebugging {
	string PreCompiledIRPath; // adbextirpath:""
	bool initialized;
	Triple triple;

	bool initialize(Module &M);
	bool runOnFunction(Function &F);
};

#endif
