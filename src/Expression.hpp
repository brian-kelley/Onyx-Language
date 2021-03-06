#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Parser.hpp"
#include "TypeSystem.hpp"
#include "AST.hpp"

struct SubrBase;
struct Subroutine;
struct ExternalSubroutine;
struct SubroutineDecl;

struct Expression;
/**********************/
/* Parsed Expressions */
/**********************/
//Subclasses of Expression
//Constants/literals
struct IntConstant;
struct FloatConstant;
struct BoolConstant;
struct MapConstant;
struct CompoundLiteral;
struct UnionConstant;
struct SimpleConstant;
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

/*******************************/
/* Placeholders for Resolution */
/*******************************/
struct DefaultValueExpr;
struct UnresolvedExpr;

struct Expression : public Node
{
  Expression() : type(nullptr) {}
  virtual ~Expression() {}
  virtual void resolveImpl() {}
  Type* type;
  //whether this works as an lvalue
  virtual bool assignable() = 0;
  //whether this is a compile-time constant
  virtual bool constant() const
  {
    return false;
  }
  //get the number of bytes required to store the constant
  virtual int getConstantSize()
  {
    INTERNAL_ERROR;
    return 0;
  }
  //Hash this expression (for use in unordered map/set)
  //Only hard requirement: if a == b, then hash(a) == hash(b)
  virtual size_t hash() const = 0;
  virtual bool operator==(const Expression& rhs) const = 0;
  bool operator!=(const Expression& rhs) const
  {
    return !(*this == rhs);
  }
  //Precondition: constant() && rhs.constant()
  //Only to be used for folding relational binary ops.
  //Only implemented for expressions which can be constant,
  //except MapConstant
  virtual bool operator<(const Expression& rhs) const
  {
    INTERNAL_ERROR;
    return false;
  }
  bool operator>(const Expression& rhs) const
  {
    return rhs < *this;
  }
  bool operator<=(const Expression& rhs) const
  {
    return !(rhs < *this);
  }
  bool operator>=(const Expression& rhs) const
  {
    return !(*this < rhs);
  }
  //deep copy (must already be resolved)
  virtual Expression* copy() = 0;
  virtual ostream& print(ostream& os) = 0;
};

inline ostream& operator<<(ostream& os, Expression* expr)
{
  return expr->print(os);
}

struct ExprEqual
{
  bool operator()(const Expression* lhs, const Expression* rhs) const
  {
    if(lhs == rhs)
      return true;
    return *lhs == *rhs;
  }
};

struct ExprHash
{
  size_t operator()(const Expression* e) const
  {
    return e->hash();
  }
};

struct UnaryArith : public Expression
{
  UnaryArith(OperatorEnum op, Expression* expr);
  OperatorEnum op;
  Expression* expr;
  bool assignable()
  {
    return false;
  }
  void resolveImpl();
  size_t hash() const
  {
    FNV1A f;
    f.pump(op);
    f.pump(expr->hash());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  ostream& print(ostream& os);
  Expression* copy();
};

struct BinaryArith : public Expression
{
  BinaryArith(Expression* lhs, OperatorEnum op, Expression* rhs);
  OperatorEnum op;
  Expression* lhs;
  Expression* rhs;
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  bool commutative()
  {
    return isOperCommutative(op);
  }
  size_t hash() const
  {
    FNV1A f;
    size_t lhsHash = lhs->hash();
    size_t rhsHash = rhs->hash();
    //Make sure that "a op b" and "b op a" hash the same if op is commutative-
    //operator== says these are identical
    if(isOperCommutative(op) && lhsHash > rhsHash)
      std::swap(lhsHash, rhsHash);
    f.pump(op);
    f.pump(lhsHash);
    f.pump(rhsHash);
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
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
    //Prefer a signed type to represent positive integer constants
    auto intType = (IntegerType*) primitives[Prim::INT];
    auto longType = (IntegerType*) primitives[Prim::LONG];
    if(ast->val <= (uint64_t) intType->maxSignedVal())
    {
      type = intType;
      sval = ast->val;
    }
    else if(ast->val <= (uint64_t) longType->maxSignedVal())
    {
      type = longType;
      sval = ast->val;
    }
    else
    {
      type = primitives[Prim::ULONG];
      uval = ast->val;
    }
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
  IntConstant(int64_t val, Type* t)
  {
    uval = val;
    sval = val;
    type = t;
    resolved = true;
  }
  IntConstant(uint64_t val, Type* t)
  {
    uval = val;
    sval = val;
    type = t;
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
  bool constant() const
  {
    return true;
  }
  int getConstantSize()
  {
    return ((IntegerType*) type)->size;
  }
  bool isSigned() const
  {
    if(type == getCharType())
      return false;
    //All other possible types are IntegerType
    return ((IntegerType*) type)->isSigned;
  }
  size_t hash() const
  {
    //This hash ignores underlying type - exprs
    //will need to be compared exactly anyway
    //and false positives are unlikely
    if(((IntegerType*) type)->isSigned)
      return fnv1a(sval);
    return fnv1a(uval);
  }
  bool operator<(const Expression& rhs) const
  {
    const IntConstant& rhsInt = dynamic_cast<const IntConstant&>(rhs);
    if(isSigned())
      return sval < rhsInt.sval;
    else
      return uval < rhsInt.uval;
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
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
  bool isDoublePrec() const
  {
    return typesSame(primitives[Prim::DOUBLE], this->type);
  }
  Expression* convert(Type* t);
  float fp;
  double dp;
  FloatConstant* binOp(int op, FloatConstant* rhs);
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  bool operator<(const Expression& rhs) const
  {
    const FloatConstant& rhsFloat = dynamic_cast<const FloatConstant&>(rhs);
    if(isDoublePrec())
      return dp < rhsFloat.dp;
    else
      return fp < rhsFloat.fp;
  }
  int getConstantSize()
  {
    return ((FloatType*) type)->size;
  }
  size_t hash() const
  {
    if(isDoublePrec())
      return fnv1a(dp);
    return fnv1a(fp);
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
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
  bool constant() const
  {
    return true;
  }
  int getConstantSize()
  {
    return 1;
  }
  bool operator<(const Expression& rhs) const
  {
    const BoolConstant& b = dynamic_cast<const BoolConstant&>(rhs);
    return !value && b.value;
  }
  size_t hash() const
  {
    //don't use FNV-1a here since there are only 2 possible values
    if(value)
      return 0x123456789ABCDEF0ULL;
    else
      return ~0x123456789ABCDEF0ULL;
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

//Map constant: hold set of constant key-value pairs
//Relies on operator== and operator< for Expressions
struct MapConstant : public Expression
{
  MapConstant(MapType* mt);
  unordered_map<Expression*, Expression*, ExprHash, ExprEqual> values;
  bool constant() const
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
  size_t hash() const
  {
    //the order of key-value pairs in values is NOT deterministic,
    //so use XOR to combine hashes of each key-value pair
    size_t h = 0;
    for(auto& kv : values)
    {
      FNV1A f;
      f.pump(kv.first->hash());
      f.pump(kv.second->hash());
      h ^= f.get();
    }
    return h;
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

//UnionConstant only used in interpreter.
//expr->type exactly matches exactly one of ut's options
//(which is guaranteed by semantic checking/implicit conversions)
struct UnionConstant : public Expression
{
  UnionConstant(Expression* expr, UnionType* ut);
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  bool operator<(const Expression& rhs) const
  {
    const UnionConstant& u = dynamic_cast<const UnionConstant&>(rhs);
    if(option < u.option)
      return true;
    else if(option > u.option)
      return false;
    return *value < *u.value;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(option);
    f.pump(value->hash());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
  UnionType* unionType;
  Expression* value;
  int option;
};

//CompoundLiteral represents array, struct and tuple literals and constants.
//Its type (by default) is a tuple of the types of each element; this
//implicitly converts to array, struct and other tuples (elementwise).
struct CompoundLiteral : public Expression
{
  //Constructor used by parser: mems can be unresolved and type is a tuple
  CompoundLiteral(vector<Expression*>& mems);
  //Constructor used by interpreter: mems must be resolved and type is
  //any array, tuple, or struct
  CompoundLiteral(vector<Expression*>& mems, Type* t);
  void resolveImpl();
  bool assignable()
  {
    return lvalue;
  }
  vector<Expression*> members;
  //(set during resolution): is every member an lvalue?
  bool lvalue;
  bool constant() const
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
  bool operator<(const Expression& rhs) const
  {
    const CompoundLiteral& cl = dynamic_cast<const CompoundLiteral&>(rhs);
    //lex compare
    size_t n = std::min(members.size(), cl.members.size());
    for(size_t i = 0; i < n; i++)
    {
      if(*members[i] < *cl.members[i])
        return true;
      else if(*members[i] != *cl.members[i])
        return false;
    }
    return n == members.size();
  }
  size_t hash() const
  {
    FNV1A f;
    for(auto m : members)
      f.pump(31 * m->hash());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct Indexed : public Expression
{
  Indexed(Expression* grp, Expression* ind);
  void resolveImpl();
  Expression* group; //the array or tuple being subscripted
  Expression* index;
  bool assignable()
  {
    return group->assignable();
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(7 * group->hash());
    f.pump(index->hash());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct CallExpr : public Expression
{
  CallExpr(Expression* callable, vector<Expression*>& args);
  Expression* callable;
  vector<Expression*> args;
  bool assignable()
  {
    return false;
  }
  bool isProc()
  {
    return ((CallableType*) callable->type)->isProc();
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(callable->hash());
    for(auto a : args)
      f.pump(a->hash());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
  void resolveImpl();
};

struct VarExpr : public Expression
{
  VarExpr(Variable* v, Scope* s);
  VarExpr(Variable* v);
  void resolveImpl();
  Variable* var;  //var must be looked up from current scope
  Scope* scope;
  bool assignable()
  {
    //all variables are lvalues
    return true;
  }
  size_t hash() const
  {
    //variables are uniquely identifiable by pointer
    return fnv1a(var);
  }
  bool operator==(const Expression& erhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct SubrOverloadExpr : public Expression
{
  SubrOverloadExpr(SubroutineDecl* decl);
  SubrOverloadExpr(Expression* t, SubroutineDecl* decl);
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  Expression* copy()
  {
    INTERNAL_ERROR;
    return nullptr;
  }
  size_t hash() const
  {
    return fnv1a(decl);
  }
  bool operator==(const Expression& erhs) const
  {
    auto soe = dynamic_cast<const SubrOverloadExpr*>(&erhs);
    return soe && soe->decl == decl;
  }
  ostream& print(ostream& os);
  Expression* thisObject;
  SubroutineDecl* decl;
};

struct SubroutineExpr : public Expression
{
  //2 ways to select a specific SubrBase from an
  //overload family:
  // * using a specific CallableType (requires exact param type match)
  // * given a set of parameters (doesn't require exact match)
  //thisObject may be different from the SubrOverloadExpr's this, if
  //the call matches through a composed member.
  SubroutineExpr(SubrBase* s);
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump<int>(19323423);
    f.pump(subr);
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
  SubrBase* subr;
};

struct StructMem : public Expression
{
  StructMem(Expression* base, Variable* var);
  StructMem(Expression* base, Subroutine* subr);
  void resolveImpl();
  Expression* base;  //base->type must be a StructType
  variant<Variable*, Subroutine*> member;
  bool assignable()
  {
    return base->assignable() && member.is<Variable*>();
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(base->hash());
    if(member.is<Variable*>())
      f.pump(member.get<Variable*>());
    else
      f.pump(member.get<Subroutine*>());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct NewArray : public Expression
{
  NewArray(Type* elemType, vector<Expression*> dims);
  Type* elem;
  vector<Expression*> dims;
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    FNV1A f;
    for(size_t i = 0; i < dims.size(); i++)
      f.pump((i + 1) * dims[i]->hash());
    //can't hash the type
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct ArrayLength : public Expression
{
  ArrayLength(Expression* arr);
  Expression* array;
  void resolveImpl();
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(5 * array->hash());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct UnionConvBase : public Expression
{
  UnionConvBase(Expression* b, Type* t)
  {
    base = b;
    destType = t;
  }
  //Expression implementation stuff that differs between Is and As
  virtual void resolveImpl() = 0;
  virtual bool operator==(const Expression& rhs) const = 0;
  virtual Expression* copy() = 0;
  virtual ostream& print(ostream& os) = 0;
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(base->hash());
    f.pump(13 * destType->hash());
    return f.get();
  }
  //Resolve destType and base, then populate subset.
  void partialResolve();
  //The (set of) type(s) that the union is narrowed to at runtime,
  //before final conversion to destType.
  vector<Type*> subset;
  Expression* base;
  Type* destType;
};

struct IsExpr : public UnionConvBase
{
  IsExpr(Expression* b, Type* t)
    : UnionConvBase(b, t)
  {}
  size_t hash() const
  {
    return fnv1a(UnionConvBase::hash());
  }
  void resolveImpl();
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct AsExpr : public UnionConvBase
{
  AsExpr(Expression* b, Type* t)
    : UnionConvBase(b, t)
  {}
  size_t hash() const
  {
    return UnionConvBase::hash();
  }
  void resolveImpl();
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct ThisExpr : public Expression
{
  ThisExpr(Scope* where);
  void resolveImpl();
  //structType is equal to type
  StructType* structType;
  bool assignable()
  {
    return true;
  }
  //note here: ThisExpr can read/write globals if "this"
  //is a global, but that can only be done through a proc all on
  //a global object. So it's safe to assume "this" is  
  size_t hash() const
  {
    //all ThisExprs are the same
    return 0xDEADBEEF;
  }
  bool operator==(const Expression& rhs) const
  {
    //in any context, "this" always refers to the same thing
    return dynamic_cast<const ThisExpr*>(&rhs);
  }
  Scope* usage;
  Expression* copy();
  ostream& print(ostream& os);
};

struct Converted : public Expression
{
  Converted(Expression* val, Type* dst);
  Expression* value;
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    FNV1A f;
    f.pump(23 * value->hash());
    return f.get();
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct EnumExpr : public Expression
{
  EnumExpr(EnumConstant* ec); //needs no resolve() after
  EnumConstant* value;
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    //EnumConstants are names (unique, pointer-identifiable)
    return fnv1a(value);
  }
  bool constant() const
  {
    return true;
  }
  bool operator<(const Expression& rhs) const
  {
    const EnumExpr& e = dynamic_cast<const EnumExpr&>(rhs);
    return value->value < e.value->value;
  }
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

struct SimpleConstant : public Expression
{
  SimpleConstant(SimpleType* s);
  SimpleType* st;
  bool assignable()
  {
    return false;
  }
  bool constant() const
  {
    return true;
  }
  bool operator<(const Expression& rhs) const
  {
    return false;
  }
  size_t hash() const;
  bool operator==(const Expression& rhs) const;
  Expression* copy();
  ostream& print(ostream& os);
};

//DefaultValueExpr is just a placeholder
//When resolved, it's replaced by type->getDefaultValue()
struct DefaultValueExpr : public Expression
{
  DefaultValueExpr(Type* t_) : t(t_) {}
  void resolveImpl()
  {
    INTERNAL_ERROR;
  }
  bool assignable()
  {
    return false;
  }
  size_t hash() const
  {
    return 0;
  }
  Expression* copy()
  {
    return new DefaultValueExpr(t);
  }
  bool operator==(const Expression&) const
  {
    INTERNAL_ERROR;
    return false;
  }
  ostream& print(ostream& os)
  {
    INTERNAL_ERROR;
    return os;
  }
  Type* t;
};

struct UnresolvedExpr : public Expression
{
  UnresolvedExpr(string name, Scope* s);
  UnresolvedExpr(Member* name, Scope* s);
  UnresolvedExpr(Expression* base, Member* name, Scope* s);
  bool assignable()
  {
    return false;
  }
  void resolveImpl()
  {
    //Should never be called!
    //Should instead be replaced inside resolveExpr(...)
    INTERNAL_ERROR;
  }
  size_t hash() const
  {
    //UnresolvedExpr does not appear in a resolved AST
    return 0;
  }
  Expression* copy()
  {
    INTERNAL_ERROR;
    return nullptr;
  }
  ostream& print(ostream& os)
  {
    INTERNAL_ERROR;
    return os;
  }
  bool operator==(const Expression&) const
  {
    INTERNAL_ERROR;
    return false;
  }
  Expression* base; //null = no base
  Member* name;
  Scope* usage;
  //Can temporarily import enum's values into the current
  //scope for the purposes of name lookup (for
  //switch cases). Will never actually override the results
  //of a name lookup (if there is a variable with same name)
  //but there will be a warning if the enum value is overridden.
  static void setShortcutEnum(EnumType* et);
  static void clearShortcutEnum();
  static EnumType* shortcutEnum;
};

void resolveExpr(Expression*& expr);
void resolveAndCoerce(Expression*& expr, Type* reqType);

#endif

