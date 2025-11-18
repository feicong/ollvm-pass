#ifndef _CONSTANT_ENCRYPTION_H_
#define _CONSTANT_ENCRYPTION_H_

#include "common.h"

struct ConstantEncryption {
	bool SubstituteXor; // constenc_subxor:false
    uint32_t SubstituteXorProb; // constenc_subxor_prob:40
    bool ConstToGV; // constenc_togv:false
    uint32_t ConstToGVProb; // constenc_togv_prob:50
    int ObfTimes; // constenc_times:1
    bool dispatchonce;
    std::set<GlobalVariable *> handled_gvs;
    
    bool initialize(Module &M);
    bool shouldEncryptConstant(Instruction *I);
    bool runOnFunction(Function& F);
    bool isDispatchOnceToken(GlobalVariable *GV);
    bool isAtomicLoaded(GlobalVariable *GV);
    void EncryptConstants(Function &F);
    void Constant2GlobalVariable(Function &F);
    void HandleConstantIntInitializerGV(GlobalVariable *GVPtr);
    void HandleConstantIntOperand(Instruction *I, unsigned opindex);
    std::pair<ConstantInt * /*key*/, ConstantInt * /*new*/> PairConstantInt(ConstantInt *C);
};

#endif


