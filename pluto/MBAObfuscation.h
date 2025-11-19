#pragma once

#include "common.h"

namespace Pluto {

struct MbaObfuscation {
    void run(Function &F);
    static bool isRequired() { return true; }
    void substituteConstant(Instruction *I, int i);
    void substitute(BinaryOperator *BI);

    // 替换 Add 指令
    Value *substituteAdd(BinaryOperator *BI);
    // 替换 Sub 指令
    Value *substituteSub(BinaryOperator *BI);
    // 替换 And 指令
    Value *substituteAnd(BinaryOperator *BI);
    // 替换 Or 指令
    Value *substituteOr(BinaryOperator *BI);
    // 替换 Xor 指令
    Value *substituteXor(BinaryOperator *BI);
};

}; // namespace Pluto