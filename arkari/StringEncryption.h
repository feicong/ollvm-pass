#ifndef OBFUSCATION_STRING_ENCRYPTION_H
#define OBFUSCATION_STRING_ENCRYPTION_H

#include "common.h"
#include "CryptoUtils.h"

struct StringEncryption {
    struct CSPEntry {
        CSPEntry() : ID(0), Offset(0), DecGV(nullptr), DecStatus(nullptr), DecFunc(nullptr) {}
        unsigned ID;
        unsigned Offset;
        GlobalVariable *DecGV;
        GlobalVariable *DecStatus; // is decrypted or not
        std::vector<uint8_t> Data;
        std::vector<uint8_t> EncKey;
        Function *DecFunc;
    };

    struct CSUser {
        CSUser(Type* ETy, GlobalVariable *User, GlobalVariable *NewGV)
            : Ty(ETy), GV(User), DecGV(NewGV), DecStatus(nullptr),
                InitFunc(nullptr) {}
        Type *Ty;
        GlobalVariable *GV;
        GlobalVariable *DecGV;
        GlobalVariable *DecStatus; // is decrypted or not
        Function *InitFunc; // InitFunc will use decryted string to initialize DecGV
    };

    CryptoUtils RandomEngine;
    std::vector<CSPEntry *> ConstantStringPool;
    std::map<GlobalVariable *, CSPEntry *> CSPEntryMap;
    std::map<GlobalVariable *, CSUser *> CSUserMap;
    GlobalVariable *EncryptedStringTable;
    std::set<GlobalVariable *> MaybeDeadGlobalVars;

    bool doFinalization(Module &);
    bool runOnModule(Module &M);
    void collectConstantStringUser(GlobalVariable *CString, std::set<GlobalVariable *> &Users);
    bool isValidToEncrypt(GlobalVariable *GV);
    bool processConstantStringUse(Function *F);
    void deleteUnusedGlobalVariable();
    Function *buildDecryptFunction(Module *M, const CSPEntry *Entry);
    Function *buildInitFunction(Module *M, const CSUser *User);
    void getRandomBytes(std::vector<uint8_t> &Bytes, uint32_t MinSize, uint32_t MaxSize);
    void lowerGlobalConstant(Constant *CV, IRBuilder<> &IRB, Value *Ptr, Type *Ty);
    void lowerGlobalConstantStruct(ConstantStruct *CS, IRBuilder<> &IRB, Value *Ptr, Type *Ty);
    void lowerGlobalConstantArray(ConstantArray *CA, IRBuilder<> &IRB, Value *Ptr, Type *Ty);
};

#endif
