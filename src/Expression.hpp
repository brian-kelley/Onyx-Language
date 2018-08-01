#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "AST.hpp"

struct Expression : public Node
{
  Expression() : type(nullptr) {}
  virtual void resolveImpl(bool final) {}
  //Find set of read (input) or write (output) variables
  virtual set<Variable*> getReads()
  {
    return set<Variable*>();
  }
  //getWrites assume this is the LHS
  //so it's not implemented for RHS-only exprs
  virtual set<Variable*> getWrites()
  {
    return set<Variable*>();
  }
  Type* type;
  //whether this works as an lvalue
  virtual bool assignable() = 0;
  //whether this is a compile-time constant
  virtual bool constant()
  {
    return false;
  }
  //get the number of bytes required to store the constant
  //(is 0 for non-constants)
  virtual int getConstantSize()
  {
    return 0;
  }
};

//Subclasses of Expression
//Constants/literals
struct IntConstant;
struct FloatConstant;
struct StringConstant;
struct BoolConstant;
struct MapConstant;
struct CompoundLiteral;
struct UnionConstant;
//Arithmetic
struct UnaryArith;
struct BinaryArith;
//Data structure manipulations
struct Indexed;
struct NewArray;
struct ArrayLength;
struct AsExpr;
struct IsExpr;
struct CallExpr;
struct VarExpr;
struct Converted;
struct ThisExpr;
struct ErrorVal;
struct UnresolvedExpr;

//Assuming expr is a struct type, get the struct scope
//Otherwise, display relevant errors
Scope* scopeForExpr(Expression* expr);

struct UnaryArith : public Expression
{
  UnaryArith(int op, Expression* expr);
  int op;
  Expression* expr;
  bool assignable()
  {
    return false;
  }
  void resolveImpl(bool final);
  set<Variable*> getReads();
};

struct BinaryArith : public Expression
{
  BinaryArith(Expression* lhs, int op, Expression* rhs);
  int op;
  Expression* lhs;
  Expression* rhs;
  void resolveImpl(bool final);
  set<Variable*> getReads();
  bool assignable()
  {
    return false;
  }
};

struct IntConstant : public Expression
{
  IntConstant()
  {
    uval = 0;
    sval = 0;
    type = primitives[Prim::ULONG];
    resolved = true;
  }
  IntConstant(IntLit* ast)
  {
    uval = ast->val;
    type = primitives[Prim::ULONG];
    resolved = true;
  }
  IntConstant(int64_t val)
  {
    sval = val;
    type = primitives[Prim::LONG];
    resolved = true;
  }
  IntConstant(uint64_t val)
  {
    uval = val;
    type = primitives[Prim::ULONG];
    resolved = true;
  }
  //Attempt to convert to int/float/enum type
  //Make sure the conversion is valid and show error
  //if this fails
  Expression* convert(Type* t);
  //Return true if value fits in the type
  bool checkValueFits();
  IntConstant* binOp(int op, IntConstant* rhs);
  int64_t sval;
  uint64_t uval;
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
  int getConstantSize()
  {
    return ((IntegerType*) type)->size;
  }
  bool isSigned()
  {
    return ((IntegerType*) type)->isSigned;
  }
};

struct FloatConstant : public Expression
{
  FloatConstant()
  {
    fp = 0;
    dp = 0;
    type = primitives[Prim::DOUBLE];
    resolved = true;
  }
  FloatConstant(FloatLit* ast)
  {
    dp = ast->val;
    type = primitives[Prim::DOUBLE];
    resolved = true;
  }
  FloatConstant(float val)
  {
    fp = val;
    type = primitives[Prim::FLOAT];
    resolved = true;
  }
  FloatConstant(double val)
  {
    dp = val;
    type = primitives[Prim::DOUBLE];
    resolved = true;
  }
  bool isDoublePrec()
  {
    return getConstantSize() == 8;
  }
  float fp;
  double dp;
  FloatConstant* binOp(int op, FloatConstant* rhs);
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
  int getConstantSize()
  {
    return ((FloatType*) type)->size;
  }
  Expression* convert(Type* t);
};

struct StringConstant : public Expression
{
  StringConstant(StrLit* ast)
  {
    value = ast->val;
    type = getArrayType(primitives[Prim::CHAR], 1);
    resolveType(type, true);
    resolved = true;
  }
  string value;
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
  int getConstantSize()
  {
    return 16 + value.length() + 1;
  }
};

struct CharConstant : public Expression
{
  CharConstant(CharLit* ast)
  {
    value = ast->val;
    type = primitives[Prim::CHAR];
    resolved = true;
  }
  CharConstant(char c)
  {
    value = c;
    type = primitives[Prim::CHAR];
    resolved = true;
  }
  char value;
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
};

struct BoolConstant : public Expression
{
  BoolConstant(bool v)
  {
    value = v;
    type = primitives[Prim::BOOL];
    resolved = true;
  }
  bool value;
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
  int getConstantSize()
  {
    return 1;
  }
};

//Map constant: hold set of constant key-value pairs
//Relies on operator== and operator< for Expressions
struct MapConstant : public Expression
{
  map<Expression*, Expression*> values;
  bool constant()
  {
    return true;
  }
  int getConstantSize()
  {
    int total = 0;
    for(auto& kv : values)
    {
      total += kv.first->getConstantSize();
      total += kv.second->getConstantSize();
    }
    return total;
  }
  bool assignable()
  {
    return false;
  }
};

//UnionConstant only used in IR/optimization
//expr->type exactly matches exactly one of ut's options
//(which is guaranteed by semantic checking/implicit conversions)
struct UnionConstant : public Expression
{
  UnionConstant(Expression* expr, UnionType* ut)
  {
    INTERNAL_ASSERT(expr->constant());
    value = expr;
    unionType = ut;
    type = unionType;
    resolved = true;
  }
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
  UnionType* unionType;
  Expression* value;
};

//it is impossible to determine the type of a CompoundLiteral by itself
//CompoundLiteral covers both array and struct literals
struct CompoundLiteral : public Expression
{
  CompoundLiteral(vector<Expression*>& mems);
  void resolveImpl(bool final);
  bool assignable()
  {
    return lvalue;
  }
  vector<Expression*> members;
  bool lvalue;
  set<Variable*> getReads();
  set<Variable*> getWrites();
  bool constant()
  {
    for(auto m : members)
    {
      if(!m->constant())
        return false;
    }
    return true;
  }
  int getConstantSize()
  {
    int total = 0;
    for(auto mem : members)
    {
      total += sizeof(void*) + mem->getConstantSize();
    }
    return total;
  }
};

struct Indexed : public Expression
{
  Indexed(Expression* grp, Expression* ind);
  void resolveImpl(bool final);
  Expression* group; //the array or tuple being subscripted
  Expression* index;
  bool assignable()
  {
    return group->assignable();
  }
  set<Variable*> getReads();
  set<Variable*> getWrites();
};

struct CallExpr : public Expression
{
  CallExpr(Expression* callable, vector<Expression*>& args);
  void resolveImpl(bool final);
  Expression* callable;
  vector<Expression*> args;
  bool assignable()
  {
    return false;
  }
  //TODO: do evaluate calls in optimizing mode
  set<Variable*> getReads();
};

struct VarExpr : public Expression
{
  VarExpr(Variable* v, Scope* s);
  VarExpr(Variable* v);
  void resolveImpl(bool final);
  Variable* var;  //var must be looked up from current scope
  Scope* scope;
  bool assignable()
  {
    //all variables are lvalues
    return true;
  }
  set<Variable*> getReads();
  set<Variable*> getWrites();
};

//Expression to represent constant callable
//May be standalone, or may be applied to an object
struct SubroutineExpr : public Expression
{
  SubroutineExpr(Subroutine* s);
  SubroutineExpr(Expression* thisObj, Subroutine* s);
  SubroutineExpr(ExternalSubroutine* es);
  void resolveImpl(bool final);
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
  Subroutine* subr;
  ExternalSubroutine* exSubr;
  Expression* thisObject; //null for static/extern
};

struct UnresolvedExpr : public Expression
{
  UnresolvedExpr(string name, Scope* s);
  UnresolvedExpr(Member* name, Scope* s);
  UnresolvedExpr(Expression* base, Member* name, Scope* s);
  Expression* base; //null = no base
  Member* name;
  Scope* usage;
  bool assignable()
  {
    return false;
  }
  void resolveImpl(bool final)
  {
    INTERNAL_ERROR;
  }
};

struct StructMem : public Expression
{
  StructMem(Expression* base, Variable* var);
  StructMem(Expression* base, Subroutine* subr);
  void resolveImpl(bool final);
  Expression* base;           //base->type is always StructType
  variant<Variable*, Subroutine*> member;
  bool assignable()
  {
    return base->assignable() && member.is<Variable*>();
  }
  set<Variable*> getReads();
  set<Variable*> getWrites();
};

struct NewArray : public Expression
{
  NewArray(Type* elemType, vector<Expression*> dims);
  Type* elem;
  vector<Expression*> dims;
  void resolveImpl(bool final);
  bool assignable()
  {
    return false;
  }
};

struct ArrayLength : public Expression
{
  ArrayLength(Expression* arr);
  Expression* array;
  void resolveImpl(bool final);
  bool assignable()
  {
    return false;
  }
  set<Variable*> getReads();
};

struct IsExpr : public Expression
{
  IsExpr(Expression* b, Type* t)
  {
    base = b;
    ut = nullptr;
    optionIndex = -1;
    option = t;
    type = primitives[Prim::BOOL];
  }
  void resolveImpl(bool final);
  bool assignable()
  {
    return false;
  }
  set<Variable*> getReads()
  {
    return base->getReads();
  }
  Expression* base;
  UnionType* ut;
  int optionIndex;
  Type* option;
};

struct AsExpr : public Expression
{
  AsExpr(Expression* b, Type* t)
  {
    base = b;
    ut = nullptr;
    optionIndex = -1;
    option = t;
    type = t;
  }
  void resolveImpl(bool final);
  bool assignable()
  {
    return false;
  }
  set<Variable*> getReads()
  {
    return base->getReads();
  }
  Expression* base;
  UnionType* ut;
  int optionIndex;
  Type* option;
};

struct ThisExpr : public Expression
{
  ThisExpr(Scope* where);
  //structType is equal to type
  StructType* structType;
  bool assignable()
  {
    return true;
  }
};

struct Converted : public Expression
{
  Converted(Expression* val, Type* dst);
  Expression* value;
  bool assignable()
  {
    return value->assignable();
  }
  set<Variable*> getReads();
};

struct EnumExpr : public Expression
{
  EnumExpr(EnumConstant* ec);
  EnumConstant* value;
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
};

struct ErrorVal : public Expression
{
  ErrorVal();
  bool assignable()
  {
    return false;
  }
  bool constant()
  {
    return true;
  }
};

void resolveExpr(Expression*& expr, bool final);

//expr must be resolved
ostream& operator<<(ostream& os, Expression* expr);

#endif

