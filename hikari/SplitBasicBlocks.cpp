// For open-source license, please refer to
// [License](https://github.com/HikariObfuscator/Hikari/wiki/License).
//===----------------------------------------------------------------------===//

#include "Split.h"
#include "CryptoUtils.h"

bool SplitBasicBlock::runOnFunction(Function &F) {
  split(&F);
  return true;
}

void SplitBasicBlock::split(Function *F) {
  SmallVector<BasicBlock *, 16> origBB;
  size_t split_ctr = 0;

  // Save all basic blocks
  for (BasicBlock &BB : *F)
    origBB.emplace_back(&BB);

  for (BasicBlock *currBB : origBB) {
    if (currBB->size() < 2 || containsPHI(currBB) ||
        containsSwiftError(currBB))
      continue;

    if ((size_t)SplitNum > currBB->size() - 1)
      split_ctr = currBB->size() - 1;
    else
      split_ctr = (size_t)SplitNum;

    // Generate splits point (count number of the LLVM instructions in the
    // current BB)
    SmallVector<size_t, 32> llvm_inst_ord;
    for (size_t i = 1; i < currBB->size(); ++i)
      llvm_inst_ord.emplace_back(i);

    // Shuffle
    split_point_shuffle(llvm_inst_ord);
    std::sort(llvm_inst_ord.begin(), llvm_inst_ord.begin() + split_ctr);

    // Split
    size_t llvm_inst_prev_offset = 0;
    BasicBlock::iterator curr_bb_it = currBB->begin();
    BasicBlock *curr_bb_offset = currBB;

    for (size_t i = 0; i < split_ctr; ++i) {
      for (size_t j = 0; j < llvm_inst_ord[i] - llvm_inst_prev_offset; ++j)
        ++curr_bb_it;

      llvm_inst_prev_offset = llvm_inst_ord[i];

      // https://github.com/eshard/obfuscator-llvm/commit/fff24c7a1555aa3ae7160056b06ba1e0b3d109db
      /* TODO: find a real fix or try with the probe-stack inline-asm when its
        * ready. See https://github.com/Rust-for-Linux/linux/issues/355.
        * Sometimes moving an alloca from the entry block to the second block
        * causes a segfault when using the "probe-stack" attribute (observed
        * with with Rust programs). To avoid this issue we just split the entry
        * block after the allocas in this case.
        */
      if (F->hasFnAttribute("probe-stack") && currBB->isEntryBlock() &&
          isa<AllocaInst>(curr_bb_it))
        continue;

      curr_bb_offset = curr_bb_offset->splitBasicBlock(
          curr_bb_it, curr_bb_offset->getName() + ".split");
    }
  }
}

bool SplitBasicBlock::containsPHI(BasicBlock *BB) {
  for (Instruction &I : *BB)
    if (isa<PHINode>(&I))
      return true;
  return false;
}

bool SplitBasicBlock::containsSwiftError(BasicBlock *BB) {
  for (Instruction &I : *BB)
    if (AllocaInst *AI = dyn_cast<AllocaInst>(&I))
      if (AI->isSwiftError())
        return true;
  return false;
}

void SplitBasicBlock::split_point_shuffle(SmallVector<size_t, 32> &vec) {
  int n = vec.size();
  for (int i = n - 1; i > 0; --i)
    std::swap(vec[i], vec[cryptoutils->get_range(i + 1)]);
}

