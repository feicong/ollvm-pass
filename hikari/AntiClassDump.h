#ifndef _ANTI_CLASSDUMP_H_
#define _ANTI_CLASSDUMP_H_

#include "common.h"

struct AntiClassDump {
	bool UseInitialize; // acd-use-initialize:true
	bool RenameMethodIMP; // acd-rename-methodimp:false

	bool appleptrauth;
	bool opaquepointers;
	Triple triple;

	bool doInitialization(Module &M);
	bool runOnModule(Module &M);
	std::unordered_map<std::string, Value *> splitclass_ro_t(ConstantStruct *class_ro, Module *M);
	void handleClass(GlobalVariable *GV, Module *M);
	void HandleMethods(ConstantStruct *class_ro, IRBuilder<> *IRB, Module *M, Value *Class, bool isMetaClass);
	GlobalVariable *readPtrauth(GlobalVariable *GV);
};

#endif
