#ifndef OBFUSCATION_INDIRECTBR_H
#define OBFUSCATION_INDIRECTBR_H

#include "common.h"
#include "CryptoUtils.h"

struct IndirectBranch {
    std::map<BasicBlock *, unsigned> BBNumbering;
    std::vector<BasicBlock *> BBTargets;        //all conditional branch targets
    CryptoUtils RandomEngine;

    void NumberBasicBlock(Function &F);
    GlobalVariable *getIndirectTargets(Function &F, ConstantInt *EncKey);
    bool runOnFunction(Function &Fn);
};

#endif
