#ifndef _SUBSTITUTIONS_H_
#define _SUBSTITUTIONS_H_

#include "common.h"

#define NUMBER_ADD_SUBST 4
#define NUMBER_SUB_SUBST 3
#define NUMBER_AND_SUBST 2
#define NUMBER_OR_SUBST 2
#define NUMBER_XOR_SUBST 2

struct Substitution {
  int ObfTimes;
  Substitution();

  void (Substitution::*funcAdd[NUMBER_ADD_SUBST])(BinaryOperator *bo);
  void (Substitution::*funcSub[NUMBER_SUB_SUBST])(BinaryOperator *bo);
  void (Substitution::*funcAnd[NUMBER_AND_SUBST])(BinaryOperator *bo);
  void (Substitution::*funcOr[NUMBER_OR_SUBST])(BinaryOperator *bo);
  void (Substitution::*funcXor[NUMBER_XOR_SUBST])(BinaryOperator *bo);

  bool runOnFunction(Function &F);
  bool substitute(Function *f);

  void addNeg(BinaryOperator *bo);
  void addDoubleNeg(BinaryOperator *bo);
  void addRand(BinaryOperator *bo);
  void addRand2(BinaryOperator *bo);

  void subNeg(BinaryOperator *bo);
  void subRand(BinaryOperator *bo);
  void subRand2(BinaryOperator *bo);

  void andSubstitution(BinaryOperator *bo);
  void andSubstitutionRand(BinaryOperator *bo);

  void orSubstitution(BinaryOperator *bo);
  void orSubstitutionRand(BinaryOperator *bo);

  void xorSubstitution(BinaryOperator *bo);
  void xorSubstitutionRand(BinaryOperator *bo);
};

#endif

