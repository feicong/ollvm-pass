#pragma once

#include "common.h"

namespace Pluto {

struct GlobalEncryption {
    void insertArrayDecryption(Module &M, GlobalVariable *GV, uint64_t key, uint64_t eleNum);
    void insertIntDecryption(Module &M, GlobalVariable *GV, uint64_t key);
    void run(Module &M);
};

}; // namespace Pluto