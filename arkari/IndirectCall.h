#ifndef OBFUSCATION_INDIRECT_CALL_H
#define OBFUSCATION_INDIRECT_CALL_H

#include "common.h"
#include "CryptoUtils.h"

struct IndirectCall {
    std::map<Function *, unsigned> CalleeNumbering;
    std::vector<CallInst *> CallSites;
    std::vector<Function *> Callees;
    CryptoUtils RandomEngine;

    void NumberCallees(Function &F);
    GlobalVariable *getIndirectCallees(Function &F, ConstantInt *EncKey);
    bool runOnFunction(Function &Fn);
};

#endif
