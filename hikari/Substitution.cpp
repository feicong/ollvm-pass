// For open-source license, please refer to
// [License](https://github.com/HikariObfuscator/Hikari/wiki/License).
//===----------------------------------------------------------------------===//

#include "Substitution.h"
#include "SubstituteImpl.h"
#include "CryptoUtils.h"

#define DEBUG_TYPE "substitution"

// Stats
STATISTIC(Add, "Add substitued");
STATISTIC(Sub, "Sub substitued");
STATISTIC(Mul, "Mul substitued");
// STATISTIC(Div,  "Div substitued");
// STATISTIC(Rem,  "Rem substitued");
// STATISTIC(Shi,  "Shift substitued");
STATISTIC(And, "And substitued");
STATISTIC(Or, "Or substitued");
STATISTIC(Xor, "Xor substitued");

bool Substitution::runOnFunction(Function &F) {
  substitute(&F);
  return true;
};

bool Substitution::substitute(Function *f) {
  // Loop for the number of time we run the pass on the function
  uint32_t times = ObfTimes;
  do {
    for (Instruction &inst : instructions(f))
      if (inst.isBinaryOp() &&
          cryptoutils->get_range(100) <= ObfProbRate) {
        switch (inst.getOpcode()) {
        case BinaryOperator::Add:
          // case BinaryOperator::FAdd:
          SubstituteImpl::substituteAdd(cast<BinaryOperator>(&inst));
          ++Add;
          break;
        case BinaryOperator::Sub:
          // case BinaryOperator::FSub:
          SubstituteImpl::substituteSub(cast<BinaryOperator>(&inst));
          ++Sub;
          break;
        case BinaryOperator::Mul:
          // case BinaryOperator::FMul:
          SubstituteImpl::substituteMul(cast<BinaryOperator>(&inst));
          ++Mul;
          break;
        case BinaryOperator::UDiv:
        case BinaryOperator::SDiv:
        case BinaryOperator::FDiv:
          //++Div;
          break;
        case BinaryOperator::URem:
        case BinaryOperator::SRem:
        case BinaryOperator::FRem:
          //++Rem;
          break;
        case Instruction::Shl:
          //++Shi;
          break;
        case Instruction::LShr:
          //++Shi;
          break;
        case Instruction::AShr:
          //++Shi;
          break;
        case Instruction::And:
          SubstituteImpl::substituteAnd(cast<BinaryOperator>(&inst));
          ++And;
          break;
        case Instruction::Or:
          SubstituteImpl::substituteOr(cast<BinaryOperator>(&inst));
          ++Or;
          break;
        case Instruction::Xor:
          SubstituteImpl::substituteXor(cast<BinaryOperator>(&inst));
          ++Xor;
          break;
        default:
          break;
        } // End switch
      } // End isBinaryOp
  } while (--times); // for times
  return true;
}

