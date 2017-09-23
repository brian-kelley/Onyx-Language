#ifndef C_GEN_H
#define C_GEN_H

#include "Common.hpp"
#include "MiddleEnd.hpp"

namespace C
{
  //Generate C source file to outputStem.c, then run C compiler, and if !keep delete the source file
  void generate(string outputStem, bool keep);
  //add file label and basic libc includes
  void genCommon();
  //forward-declare all compound types (and arrays),
  //then actually define them as C structs
  void genTypeDecls();
  //declare all global/static data
  void genGlobals();
  //forward-declare all subroutines, then actually provide impls
  void genSubroutines();

  //Generate a unique C identifier (also won't collide with any existing C name)
  string getIdentifier();
  //given lambda f that takes a Scope*, run f on all scopes (depth-first)
  template<typename F> void walkScopeTree(F f);
  void generateCompoundType(ostream& c, string cName, TypeSystem::Type* t);
  void generateStatement(ostream& c, Block* b, Statement* stmt);
  void generateBlock(ostream& c, Block* b);
  void generateExpression(ostream& c, Expression* expr);
  void generateAssignment(ostream& c, Block* b, Expression* lhs, Expression* rhs);
  //produce a char literal (escaped as needed)
  void generateCharLiteral(ostream& c, char character);
  //utility functions
  void generateInitFuncs();
  void generateCopyFuncs();
  void generateAllocFuncs();
  void generatePrintFuncs();
  string getInitFunc(TypeSystem::Type* t);
  string getCopyFunc(TypeSystem::Type* t);
  string getAllocFunc(TypeSystem::Type* t);
  string getPrintFunc(TypeSystem::Type* t);
}

#endif
