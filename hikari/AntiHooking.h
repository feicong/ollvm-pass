#ifndef _ANTI_HOOKING_H_
#define _ANTI_HOOKING_H_

#include "common.h"

struct AntiHook {
	string PreCompiledIRPath; // adhexrirpath:""
	bool CheckInlineHook; // ah_inline:true
	bool CheckObjectiveCRuntimeHook; // ah_objcruntime:true
	bool AntiRebindSymbol; // ah_antirebind:false
	bool initialized;
  	bool opaquepointers;
	Triple triple;

	bool initialize(Module &M);
	bool runOnFunction(Function &F);
	void HandleInlineHookAArch64(Function *F);
	void HandleObjcRuntimeHook(Function *ObjcMethodImp, std::string classname, std::string selname, bool classmethod);
	void CreateCallbackAndJumpBack(IRBuilder<> *IRBB, BasicBlock *C);
};

#endif
