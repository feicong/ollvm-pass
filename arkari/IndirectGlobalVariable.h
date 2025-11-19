#ifndef OBFUSCATION_INDIRECT_GLOBAL_VARIABLE_H
#define OBFUSCATION_INDIRECT_GLOBAL_VARIABLE_H

#include "common.h"
#include "CryptoUtils.h"

struct IndirectGlobalVariable {
    std::map<GlobalVariable *, unsigned> GVNumbering;
    std::vector<GlobalVariable *> GlobalVariables;
    CryptoUtils RandomEngine;

    void NumberGlobalVariable(Function &F);
    GlobalVariable *getIndirectGlobalVariables(Function &F, ConstantInt *EncKey);
    bool runOnFunction(Function &Fn);
};

#endif
