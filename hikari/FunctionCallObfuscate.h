#ifndef _FUNCTION_CALL_OBFUSCATION_H_
#define _FUNCTION_CALL_OBFUSCATION_H_

#include "common.h"
#include <json.hpp>

struct FunctionCallObfuscate {
  int dlopen_flag; // fco_flag=-1
  string SymbolConfigPath; // fcoconfig="+-x/"
  nlohmann::json Configuration;
  bool initialized;
  bool opaquepointers;
  Triple triple;

	bool initialize(Module &M);
	bool OnlyUsedByCompilerUsed(GlobalVariable *GV);
	void HandleObjC(Function *F);
  bool runOnFunction(Function &F);
};

#endif
