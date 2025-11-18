#ifndef _INDIRECT_BRANCH_H_
#define _INDIRECT_BRANCH_H_

#include "common.h"

struct IndirectBranch {
	bool UseStack; // indibran-use-stack:true
	bool EncryptJumpTarget; // indibran-enc-jump-target:false
	bool initialized;
	map<BasicBlock *, unsigned long long> indexmap;
	std::unordered_map<Function *, ConstantInt *> encmap;
  	std::set<Function *> to_obf_funcs;

	bool initialize(Module &M);
	bool runOnFunction(Function &Func);
	void shuffleBasicBlocks(Function &F);
};

#endif
