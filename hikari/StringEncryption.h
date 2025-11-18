#ifndef _STRING_ENCRYPTION_H_
#define _STRING_ENCRYPTION_H_

#include "common.h"

struct StringEncryption {
	uint32_t ElementEncryptProb; // strcry_prob:100
	bool appleptrauth;
	bool opaquepointers;
	std::unordered_map<Function *, GlobalVariable * /*Decryption Status*/> encstatus;
	std::unordered_map<GlobalVariable *, std::pair<Constant *, GlobalVariable *>> mgv2keys;
	std::unordered_map<Constant *, SmallVector<unsigned int, 16>> unencryptedindex;
	SmallVector<GlobalVariable *, 32> genedgv;

	bool initialize(Module &M);
	bool handleableGV(GlobalVariable *GV);
	bool runOnFunction(Function &F);
	void processConstantAggregate(GlobalVariable *strGV, ConstantAggregate *CA,
		std::set<GlobalVariable *> *rawStrings,
		SmallVector<GlobalVariable *, 32> *unhandleablegvs,
		SmallVector<GlobalVariable *, 32> *Globals,
		std::set<User *> *Users, bool *breakFor);
	void HandleUser(User *U, SmallVector<GlobalVariable *, 32> &Globals,
		std::set<User *> &Users,
		std::unordered_set<User *> &VisitedUsers);
	void HandleFunction(Function *Func);
	GlobalVariable *ObjectiveCString(GlobalVariable *GV, std::string name,
		GlobalVariable *newString,
		ConstantStruct *CS);
	void HandleDecryptionBlock(BasicBlock *B, BasicBlock *C,
    	std::unordered_map<GlobalVariable *,
        std::pair<Constant *, GlobalVariable *>> &GV2Keys);
};

#endif
