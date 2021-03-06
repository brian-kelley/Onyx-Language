#include "ConstantProp.hpp"
#include "Variable.hpp"
#include "Expression.hpp"

using namespace IR;

//vector<int> getLeading(CompoundLiteral* arr, int ndims)
//{
//  const int INIT = -1;
//  const int UNKNOWN = -1;
//  vector<int> dims(ndims, INIT);
//  //Depth-first search over the value tree, updating dims array at each element.
//  //If the dims value at current depth level is UNKNOWN, don't bother searching deeper.
//  stack<CompoundLiteral*> stk;
//  stack<int> depth;
//  stk.push(arr);
//  depth.push(0);
//  while(!stk.empty())
//  {
//    CompoundLiteral* process = stk.top();
//    stk.pop();
//    int d = depth.top();
//    depth.pop();
//    if(dims[d] == INIT)
//      dims[d] = process->members.size();
//    else if(dims[d] != process->members.size())
//      dims[d] = UNKNOWN;
//    if(dims[d] != UNKNOWN && d != dims.size() - 1)
//    {
//      for(auto m : process->members)
//      {
//        auto cl = dynamic_cast<CompoundLiteral*>(m);
//        //Non-leaf array constant elements should all be compound literals!
//        INTERNAL_ASSERT(cl);
//        stk.push(cl);
//        depth.push(d + 1);
//      }
//    }
//  }
//  //Trim down dimensions that are NOT constant
//  while(dims.size() && dims.back() == UNKNOWN)
//    dims.pop_back();
//  return dims;
//}
//
///* ******************************** */
///* Constant Propagation Value Types */
///* ******************************** */
//
//CPValue* CPValue::meet(CPValue* other)
//{
//  if(auto vk = dynamic_cast<ValueKnown*>(other))
//    return meetValue(vk);
//  else if(auto uk = dynamic_cast<UnionKindKnown*>(other))
//    return meetUnionKind(uk);
//  else if(auto mk = dynamic_cast<MemberKnown*>(other))
//    return meetMemberKnown(mk);
//  else if(auto lk = dynamic_cast<ArrayLengthKnown*>(other))
//    return meetArrayLenKnown(lk);
//  else if(dynamic_cast<UndefinedValue*>(other))
//    return this;
//  else if(auto nc = dynamic_cast<NonconstantValue*>(other))
//    return nc;
//  else
//    INTERNAL_ERROR;
//  return nullptr;
//}
//
///* ********** */
///* ValueKnown */
///* ********** */
//
//CPValue* ValueKnown::meetValue(ValueKnown* vk)
//{
//  //if the values match, can return this
//  if(*value == *vk->value)
//    return this;
//  //check for array constant (CompoundLiteral), with same dimensions
//  ArrayType* arrayType = dynamic_cast<ArrayType*>(value->type);
//  auto cl1 = dynamic_cast<CompoundLiteral*>(value);
//  auto cl2 = dynamic_cast<CompoundLiteral*>(vk->value);
//  if(at && cl1 && cl2)
//  {
//    //Make a list of the provably matching dimensions
//    //Know that a1, a2 have the same number of dimensions,
//    //since they are different values for the same var.
//    auto a1Dims = RectangularDims::getLeading(cl1, arrayType->ndims);
//    auto a2Dims = RectangularDims::getLeading(cl2, arrayType->ndims);
//    for(int i = a1Dims.size() - 1; i >= 0; i--)
//    {
//      if(a1Dims[i] == RectangularDims::UNKNOWN ||
//          a1Dims[i] != a2Dims[i])
//      {
//        a1Dims.pop_back();
//      }
//    }
//    if(a1Dims.size() > 0)
//    {
//      //can retain some information about the dimensions
//      return new ArrayLengthKnown(matching);
//    }
//    //otherwise, can't retain anything
//    return new NonconstantValue;
//  }
//  //check for union constants with same type
//  auto u1 = dynamic_cast<UnionConstant*>(value);
//  auto u2 = dynamic_cast<UnionConstant*>(vk->value);
//  if(u1 && u2)
//  {
//    auto ut = u1->unionType;
//    //Check if u1, u2 have the same underlying type
//    if(u1->option == u2->option)
//    {
//      return new UnionKindKnown(ut, u1->value->type);
//    }
//    else if(ut->options.size() > 2)
//    {
//      //still know some constraints on the type
//      UnionKindKnown* ukk = new UnionKindKnown(ut);
//      ukk->optionsPossible[u1->option] = true;
//      ukk->optionsPossible[u2->option] = true;
//      return ukk;
//    }
//    return new NonconstantValue;
//  }
//  //check for struct/tuple constants with members in common
//  if(!at && cl1 && cl2)
//  {
//    MemberKnown* mk = new MemberKnown(cl1, cl2);
//    if(mk->containsInformation())
//      return mk;
//    return new NonconstantValue;
//  }
//  //can't do anything else
//  return new NonconstantValue;
//}
//CPValue* ValueKnown::meetUnionKind(UnionKindKnown* uk)
//{
//  return uk->meetValue(this);
//}
//CPValue* ValueKnown::meetMemberKnown(MemberKnown* mk)
//{
//  return mk->meetValue(this);
//}
//CPValue* ValueKnown::meetArrayLenKnown(ArrayLengthKnown* lk)
//{
//  return lk->meetValue(this);
//}
//
///* ************** */
///* UnionKindKnown */
///* ************** */
//
//UnionKindKnown::UnionKindKnown(UnionType* u, Type* t)
//{
//  ut = u;
//  optionsPossible = vector<bool>(ut->options.size(), false);
//  optionsPossible[u->getTypeIndex(t)] = true;
//}
//UnionKindKnown::UnionKindKnown(UnionType* u)
//{
//  ut = u;
//  optionsPossible = vector<bool>(ut->options.size(), false);
//}
//bool apply(Expression*& expr)
//{
//}
//CPValue* UnionKindKnown::meetValue(ValueKnown* vk)
//{
//  if(auto UnionConstant* uc = dynamic_cast<UnionConstant*>(vk->value))
//  {
//    if(optionsPossible[uc->option])
//      return this;
//    else
//    {
//      auto ut = uc->unionType;
//      //need to add a new type to the list of possible types,
//      //but only retain the UnionKindKnown if at least one type is
//      //definitely not possible
//      UnionKindKnown* ukk = new UnionKindKnown(ut);
//      for(size_t i = 0; i < ut->options.size(); i++)
//      {
//        ukk->optionsPossible[i] = optionsPossible[i];
//      }
//      ukk->optionsPossible[uc->option] = true;
//      bool anyExcluded = false;
//      for(size_t i = 0; i < ut->options.size(); i++)
//      {
//        if(!ukk->optionsPossible[i])
//        {
//          anyExcluded = true;
//          break;
//        }
//      }
//      if(anyExcluded)
//        return ukk;
//      delete ukk;
//    }
//  }
//  return new NonconstantValue;
//}
//CPValue* UnionKindKnown::meetUnionKind(UnionKindKnown* uk)
//{
//  //Meet with another UnionKindKnown: merge possible types
//  
//}
//CPValue* UnionKindKnown::meetMemberKnown(MemberKnown* mk)
//{
//  return new NonconstantValue;
//}
//CPValue* UnionKindKnown::meetArrayLenKnown(ArrayLengthKnown* lk)
//{
//  return new NonconstantValue;
//}
//
////Max number of bytes in constant expressions
////(higher value increases compiler memory usage,
////but gives more opportunities for constant folding)
//const int maxConstantSize = 256;
//
////whether the local constant table currently contains correct,
////up-to-date constant information
////
////if this is true, foldExpression() will use local constant table to
////replace VarExprs with constants
//bool foldLocals = false;
//
//LocalConstantTable::LocalConstantTable(SubroutineIR* subr)
//{
//  for(size_t i = 0; i < subr->vars.size(); i++)
//  {
//    varTable[subr->vars[i]] = i;
//  }
//  //now know how many variables there are,
//  //so create the constant table with right size
//  for(size_t i = 0; i < subr->blocks.size(); i++)
//  {
//    constants.emplace_back(varTable.size(), CPValue(UNDEFINED_VAL));
//  }
//}
//
////The table for current subroutine
//LocalConstantTable* localConstants = nullptr;
////The current basic block being processed in constant propagation
//int currentBB;
//
//bool LocalConstantTable::update(Variable* var, CPValue replace)
//{
//  return update(varTable[var], replace);
//}
//
//bool LocalConstantTable::update(int varIndex, CPValue replace)
//{
//  CPValue& prev = constants[currentBB][varIndex];
//  if(prev != replace)
//  {
//    constants[currentBB][varIndex] = replace;
//    return true;
//  }
//  return false;
//}
//
//bool LocalConstantTable::meetUpdate(int varIndex, int destBlock, CPValue incoming)
//{
//  CPValue& prev = constants[destBlock][varIndex];
//  CPValue met = constantMeet(prev, incoming);
//  if(prev != met)
//  {
//    constants[destBlock][varIndex] = met;
//    return true;
//  }
//  return false;
//}
//
//CPValue& LocalConstantTable::getStatus(Variable* var)
//{
//  return constants[currentBB][varTable[var]];
//}
//
////Join operator for "CPValue" (used by dataflow analysis).
////associative/commutative
////
////Since undefined values are impossible, there is no need for a "top" value
//CPValue constantMeet(CPValue& lhs, CPValue& rhs)
//{
//  //meet(c, c) = c
//  //meet(c, d) = x
//  //meet(x, _) = x
//  //meet(?, _) = _
//  if(lhs.val.is<Expression*>() && rhs.val.is<Expression*>())
//  {
//    Expression* l = lhs.val.get<Expression*>();
//    Expression* r = rhs.val.get<Expression*>();
//    if(*l != *r)
//      return CPValue(NON_CONSTANT);
//    else
//      return CPValue(l);
//  }
//  else if(lhs.val.is<NonConstant>() || rhs.val.is<NonConstant>())
//  {
//    return CPValue(NON_CONSTANT);
//  }
//  else if(lhs.val.is<UndefinedVal>())
//  {
//    return rhs;
//  }
//  //otherwise, rhs is undefined, so just take whatever lhs is
//  return lhs;
//}
//
////Convert a constant expression to another type
////conv->value must already be folded and be constant
//static Expression* convertConstant(Expression* value, Type* type)
//{
//  type = canonicalize(type);
//  if(typesSame(value->type, type))
//    return value;
//  //For converting union constants, use the underlying value
//  if(auto uc = dynamic_cast<UnionConstant*>(value))
//  {
//    value = uc->value;
//  }
//  Node* loc = value;
//  INTERNAL_ASSERT(value->constant());
//  int option = -1;
//  auto structDst = dynamic_cast<StructType*>(type);
//  if(auto unionDst = dynamic_cast<UnionType*>(type))
//  {
//    for(size_t i = 0; i < unionDst->options.size(); i++)
//    {
//      if(typesSame(unionDst->options[i], value->type))
//      {
//        option = i;
//        break;
//      }
//    }
//    if(option < 0)
//    {
//      for(size_t i = 0; i < unionDst->options.size(); i++)
//      {
//        if(unionDst->options[i]->canConvert(value->type))
//        {
//          option = i;
//          value = convertConstant(value, unionDst->options[i]);
//          break;
//        }
//      }
//    }
//    INTERNAL_ASSERT(option >= 0);
//    value = new UnionConstant(value, unionDst->options[option], unionDst);
//    value->setLocation(loc);
//    return value;
//  }
//  else if(structDst && structDst->members.size() == 1 &&
//      !dynamic_cast<CompoundLiteral*>(value))
//  {
//    //Single-member struct is equivalent to the member
//    vector<Expression*> mem(1, value);
//    auto cl = new CompoundLiteral(mem);
//    cl->resolve();
//    cl->setLocation(loc);
//    return cl;
//  }
//  else if(structDst)
//  {
//    auto clRHS = dynamic_cast<CompoundLiteral*>(value);
//    INTERNAL_ASSERT(clRHS);
//    //just need to convert the elements in a
//    //CompoundLiteral to match struct member types 
//    vector<Expression*> mems;
//    for(size_t i = 0; i < clRHS->members.size(); i++)
//    {
//      mems.push_back(convertConstant(clRHS->members[i], structDst->members[i]->type));
//    }
//    clRHS->setLocation(value);
//    clRHS->resolve();
//    clRHS->setLocation(loc);
//    return clRHS;
//  }
//  else if(auto intConst = dynamic_cast<IntConstant*>(value))
//  {
//    //do the conversion which tests for overflow and enum membership
//    value = intConst->convert(type);
//    value->setLocation(loc);
//    return value;
//  }
//  else if(auto charConst = dynamic_cast<CharConstant*>(value))
//  {
//    //char is equivalent to an 8-bit unsigned for purposes of value conversion
//    value = new IntConstant((uint64_t) charConst->value);
//    value->setLocation(loc);
//    return value;
//  }
//  else if(auto floatConst = dynamic_cast<FloatConstant*>(value))
//  {
//    value = floatConst->convert(type);
//    value->setLocation(loc);
//    return value;
//  }
//  else if(auto enumConst = dynamic_cast<EnumExpr*>(value))
//  {
//    if(enumConst->value->fitsS64)
//    {
//      //make a signed temporary int constant, then convert that
//      //(this can't possible lose information)
//      IntConstant asInt(enumConst->value->sval);
//      asInt.setLocation(loc);
//      return asInt.convert(type);
//    }
//    else
//    {
//      IntConstant asInt(enumConst->value->uval);
//      asInt.setLocation(loc);
//      return asInt.convert(type);
//    }
//  }
//  //array/struct/tuple constants can be converted implicitly
//  //to each other (all use CompoundLiteral) but individual
//  //members (primitives) may need conversion
//  else if(auto compLit = dynamic_cast<CompoundLiteral*>(value))
//  {
//    //attempt to fold all elements (can't proceed unless every
//    //one is a constant)
//    bool allConstant = true;
//    for(auto& mem : compLit->members)
//    {
//      foldExpression(mem);
//      allConstant = allConstant && mem->constant();
//    }
//    if(!allConstant)
//      return compLit;
//    if(auto st = dynamic_cast<StructType*>(type))
//    {
//      for(size_t i = 0; i < compLit->members.size(); i++)
//      {
//        if(!typesSame(compLit->members[i]->type, st->members[i]->type))
//        {
//          compLit->members[i] = convertConstant(
//              compLit->members[i], st->members[i]->type);
//        }
//        //else: don't need to convert member
//      }
//      return compLit;
//    }
//    else if(auto tt = dynamic_cast<TupleType*>(type))
//    {
//      for(size_t i = 0; i < compLit->members.size(); i++)
//      {
//        if(!typesSame(compLit->members[i]->type, tt->members[i]))
//        {
//          compLit->members[i] = convertConstant(
//              compLit->members[i], tt->members[i]);
//        }
//      }
//      return compLit;
//    }
//    else if(auto at = dynamic_cast<ArrayType*>(type))
//    {
//      //Array lit is just another CompoundLiteral,
//      //but with array type instead of tuple
//      vector<Expression*> elements;
//      for(auto& elem : compLit->members)
//      {
//        elements.push_back(convertConstant(elem, at->subtype));
//      }
//      CompoundLiteral* arrayLit = new CompoundLiteral(elements);
//      arrayLit->resolved = true;
//      arrayLit->type = at;
//      return arrayLit;
//    }
//    else if(auto mt = dynamic_cast<MapType*>(type))
//    {
//      auto mc = new MapConstant(mt);
//      //add each key/value pair to the map
//      for(size_t i = 0; i < compLit->members.size(); i++)
//      {
//        auto kv = dynamic_cast<CompoundLiteral*>(compLit->members[i]);
//        INTERNAL_ASSERT(kv);
//        Expression* key = kv->members[0];
//        Expression* val = kv->members[1];
//        if(!typesSame(key->type, mt->key))
//          key = convertConstant(key, mt->key);
//        if(!typesSame(val->type, mt->value))
//          val = convertConstant(val, mt->value);
//        mc->values[key] = val;
//      }
//      return mc;
//    }
//  }
//  //????
//  cout << "Failed to convert constant expr \"" << value << "\" to type \"" << type->getName() << "\"\n";
//  cout << "Value's type is " << value->type->getName() << '\n';
//  INTERNAL_ERROR;
//  return nullptr;
//}
//
////Evaluate a numerical binary arithmetic operation.
////Check for integer overflow and invalid operations (e.g. x / 0)
//static Expression* evalBinOp(Expression*& lhs, int op, Expression*& rhs)
//{
//  //first, intercept short-circuit evaluation cases (logical AND/OR)
//  //need to fold only the LHS, then replace value immediately if it short-circuits
//  //NOTE: since constant folds never have side effects, this change never
//  //affects program semantics but may allow folding when otherwise impossible
//  foldExpression(lhs);
//  foldExpression(rhs);
//  if(!lhs->constant() || !rhs->constant())
//    return nullptr;
//  //Comparison operations easy because
//  //all constant Expressions support comparison,
//  //with semantics matching the language
//  switch(op)
//  {
//    case CMPEQ:
//      return new BoolConstant(*lhs == *rhs);
//    case CMPNEQ:
//      return new BoolConstant(!(*lhs == *rhs));
//    case CMPL:
//      return new BoolConstant(*lhs < *rhs);
//    case CMPG:
//      return new BoolConstant(*rhs < *lhs);
//    case CMPLE:
//      return new BoolConstant(!(*rhs < *lhs));
//    case CMPGE:
//      return new BoolConstant(!(*lhs < *rhs));
//    default:;
//  }
//  if(op == PLUS)
//  {
//    bool oversize =
//      lhs->getConstantSize() >= maxConstantSize ||
//      rhs->getConstantSize() >= maxConstantSize;
//    //handle array concat, prepend and append operations (not numeric + yet)
//    CompoundLiteral* compoundLHS = dynamic_cast<CompoundLiteral*>(lhs);
//    CompoundLiteral* compoundRHS = dynamic_cast<CompoundLiteral*>(rhs);
//    if((compoundLHS || compoundRHS) && oversize)
//    {
//      //+ on arrays always increases object size, so don't fold with oversized values
//      return nullptr;
//    }
//    if(compoundLHS && compoundRHS)
//    {
//      vector<Expression*> resultMembers(compoundLHS->members.size() + compoundRHS->members.size());
//      for(size_t i = 0; i < compoundLHS->members.size(); i++)
//        resultMembers[i] = compoundLHS->members[i];
//      for(size_t i = 0; i < compoundRHS->members.size(); i++)
//        resultMembers[i + compoundLHS->members.size()] = compoundRHS->members[i];
//      CompoundLiteral* result = new CompoundLiteral(resultMembers);
//      result->resolve();
//      return result;
//    }
//    else if(compoundLHS)
//    {
//      //array append
//      vector<Expression*> resultMembers = compoundLHS->members;
//      resultMembers.push_back(rhs);
//      CompoundLiteral* result = new CompoundLiteral(resultMembers);
//      result->resolve();
//      return result;
//    }
//    else if(compoundRHS)
//    {
//      //array prepend
//      vector<Expression*> resultMembers(1 + compoundRHS->members.size());
//      resultMembers[0] = lhs;
//      for(size_t i = 0; i < compoundRHS->members.size(); i++)
//      {
//        resultMembers[i + 1] = compoundRHS->members[i];
//      }
//      CompoundLiteral* result = new CompoundLiteral(resultMembers);
//      result->resolve();
//      return result;
//    }
//  }
//  else if(op == LOR)
//  {
//    auto lhsBool = (BoolConstant*) lhs;
//    auto rhsBool = (BoolConstant*) rhs;
//    return new BoolConstant(lhsBool->value || rhsBool->value);
//  }
//  else if(op == LAND)
//  {
//    auto lhsBool = (BoolConstant*) lhs;
//    auto rhsBool = (BoolConstant*) rhs;
//    return new BoolConstant(lhsBool->value && rhsBool->value);
//  }
//  //all other binary ops are numerical operations between two ints or two floats
//  FloatConstant* lhsFloat = dynamic_cast<FloatConstant*>(lhs);
//  FloatConstant* rhsFloat = dynamic_cast<FloatConstant*>(rhs);
//  IntConstant* lhsInt = dynamic_cast<IntConstant*>(lhs);
//  IntConstant* rhsInt = dynamic_cast<IntConstant*>(rhs);
//  bool useFloat = lhsFloat && rhsFloat;
//  bool useInt = lhsInt && rhsInt;
//  INTERNAL_ASSERT(useFloat ^ useInt);
//  if(useFloat)
//    return lhsFloat->binOp(op, rhsFloat);
//  return lhsInt->binOp(op, rhsInt);
//}
//
//static CompoundLiteral* createArray(uint64_t* dims, int ndims, Type* elem)
//{
//  vector<Expression*> elems;
//  for(uint64_t i = 0; i < dims[0]; i++)
//  {
//    if(ndims == 1)
//    {
//      elems.push_back(elem->getDefaultValue());
//    }
//    else
//    {
//      elems.push_back(createArray(&dims[1], ndims - 1, elem));
//    }
//  }
//  CompoundLiteral* cl = new CompoundLiteral(elems);
//  cl->type = getArrayType(elem, ndims);
//  cl->resolved = true;
//  return cl;
//}
//
////Try to fold an expression, bottom-up
////Can fold all constants in one pass
////
////After calling this, if expr->constant(),
////is guaranteed to be completely folded into a simple constant,
////or inputs were too big to fold
////
////Return true if any IR changes are made
////Set constant to true if expr is now, or was already, a constant
//bool foldExpression(Expression*& expr, bool isLHS)
//{
//  bool update = false;
//  //Save the lexical location of previous expr,
//  //since folded expr should have the same position
//  Node* oldLoc = expr;
//  if(expr->constant())
//  {
//    //only thing needed here is convert each string constant to char arrays
//    if(auto str = dynamic_cast<StringConstant*>(expr))
//    {
//      vector<Expression*> chars;
//      for(size_t i = 0; i < str->value.size(); i++)
//      {
//        chars.push_back(new CharConstant(str->value[i]));
//      }
//      CompoundLiteral* cl = new CompoundLiteral(chars);
//      cl->type = str->type;
//      cl->resolved = true;
//      expr = cl;
//      update = true;
//    }
//  }
//  else if(auto ve = dynamic_cast<VarExpr*>(expr))
//  {
//    //Can't fold variable that gets modified by an assignment
//    if(isLHS)
//    {
//      return false;
//    }
//    if(ve->var->isLocal() && foldLocals)
//    {
//      //look up the variable in local constant table
//      auto& status = localConstants->getStatus(ve->var);
//      if(status.val.is<Expression*>())
//      {
//        expr = status.val.get<Expression*>();
//        update = true;
//      }
//      else if(status.val.is<UndefinedVal>())
//        errMsgLoc(ve, "use of variable " << ve->var->name << " before initialization");
//      //else: non-constant, can't do anything
//    }
//  }
//  else if(auto conv = dynamic_cast<Converted*>(expr))
//  {
//    update = foldExpression(conv->value) || update;
//    if(conv->value->constant())
//    {
//      expr = convertConstant(conv->value, conv->type);
//      update = true;
//    }
//  }
//  else if(auto binArith = dynamic_cast<BinaryArith*>(expr))
//  {
//    //evalBinOp returns null if expr not constant or is too big to evaluate
//    Expression* result = evalBinOp(binArith->lhs, binArith->op, binArith->rhs);
//    if(result)
//    {
//      expr = result;
//      update = true;
//    }
//  }
//  else if(auto unaryArith = dynamic_cast<UnaryArith*>(expr))
//  {
//    update = foldExpression(unaryArith->expr) || update;
//    if(unaryArith->expr->constant())
//    {
//      if(unaryArith->op == LNOT)
//      {
//        //operand must be a bool constant
//        expr = new BoolConstant(!((BoolConstant*) unaryArith->expr)->value);
//        expr->setLocation(oldLoc);
//      }
//      else if(unaryArith->op == BNOT)
//      {
//        //operand must be an integer
//        IntConstant* input = (IntConstant*) unaryArith->expr;
//        if(input->isSigned())
//          expr = new IntConstant(~(input->sval));
//        else
//          expr = new IntConstant(~(input->uval));
//        expr->setLocation(oldLoc);
//      }
//      else if(unaryArith->op == SUB)
//      {
//        //unary sub can be applied to numbers only
//        if(auto ic = dynamic_cast<IntConstant*>(unaryArith->expr))
//        {
//          if(ic->isSigned())
//          {
//            //conversion is always OK, unless the value is the minimum for the type
//            //(because that value in 2s complement has no negation)
//            //e.g. there is no +128 value for byte (but there is -128)
//            auto intType = (IntegerType*) ic->type;
//            if((intType->size == 1 && ic->sval == numeric_limits<int8_t>::min()) ||
//               (intType->size == 2 && ic->sval == numeric_limits<int16_t>::min()) ||
//               (intType->size == 4 && ic->sval == numeric_limits<int32_t>::min()) ||
//               (intType->size == 8 && ic->sval == numeric_limits<int64_t>::min()))
//            {
//              errMsgLoc(unaryArith, "negating value overflows signed integer");
//            }
//            //otherwise, always fine to do the conversion
//            IntConstant* neg = new IntConstant(-ic->sval);
//            neg->type = intType;
//            expr = neg;
//          }
//          else
//          {
//            //unsigned values are always considered nonnegative,
//            //so negating one is illegal
//            errMsgLoc(unaryArith, "can't negate an unsigned value");
//          }
//        }
//        else if(auto fc = dynamic_cast<FloatConstant*>(unaryArith->expr))
//        {
//          FloatConstant* neg = new FloatConstant;
//          neg->fp = -fc->fp;
//          neg->dp = -fc->dp;
//          neg->type = fc->type;
//          expr = neg;
//        }
//        else
//        {
//          INTERNAL_ERROR;
//        }
//      }
//      update = true;
//    }
//  }
//  else if(auto indexed = dynamic_cast<Indexed*>(expr))
//  {
//    Expression*& grp = indexed->group;
//    Expression*& ind = indexed->index;
//    update = foldExpression(grp, isLHS) || update;
//    update = foldExpression(ind) || update;
//    if(grp->constant() && ind->constant())
//    {
//      if(dynamic_cast<ArrayType*>(grp->type))
//      {
//        CompoundLiteral* arrValues = (CompoundLiteral*) grp;
//        IntConstant* intIndex = (IntConstant*) ind;
//        if(intIndex->isSigned())
//        {
//          if(intIndex->sval < 0 || intIndex->sval >= arrValues->members.size())
//          {
//            errMsgLoc(indexed, "array index " << intIndex->sval <<
//                " out of bounds [0, " << arrValues->members.size() - 1 << ")");
//          }
//          expr = arrValues->members[intIndex->sval];
//        }
//        else
//        {
//          if(intIndex->uval >= arrValues->members.size())
//          {
//            errMsgLoc(indexed, "array index " << intIndex->sval <<
//                " out of bounds [0, " << arrValues->members.size() - 1 << ")");
//          }
//          expr = arrValues->members[intIndex->uval];
//        }
//      }
//      else if(dynamic_cast<MapType*>(grp->type))
//      {
//        //map lookup returns (T | Error)
//        auto mapValue = (MapConstant*) grp;
//        auto mapIt = mapValue->values.find(ind);
//        if(mapIt == mapValue->values.end())
//        {
//          auto voidType = (SimpleType*) primitives[Prim::VOID];
//          auto voidConstant = voidType->val;
//          expr = new UnionConstant(voidConstant, voidType, (UnionType*) indexed->type);
//        }
//        else
//          expr = new UnionConstant(mapIt->second, mapIt->second->type, (UnionType*) indexed->type);
//      }
//      else
//      {
//        INTERNAL_ERROR;
//      }
//      update = true;
//    }
//  }
//  else if(auto newArray = dynamic_cast<NewArray*>(expr))
//  {
//    ArrayType* arrType = (ArrayType*) expr->type;
//    //create one default instance of an element to find its size in bytes
//    auto elemSize = arrType->elem->getDefaultValue()->getConstantSize();
//    uint64_t totalElems = 1;
//    bool allConstant = true;
//    vector<uint64_t> dimVals;
//    for(auto& dim : newArray->dims)
//    {
//      update = foldExpression(dim) || update;
//      if(!dim->constant())
//      {
//        allConstant = false;
//        break;
//      }
//      auto dimVal = (IntConstant*) dim;
//      if(dimVal->isSigned())
//      {
//        totalElems *= dimVal->sval;
//        dimVals.push_back(dimVal->sval);
//      }
//      else
//      {
//        totalElems *= dimVal->uval;
//        dimVals.push_back(dimVal->uval);
//      }
//    }
//    if(elemSize * totalElems <= maxConstantSize)
//    {
//      //can create the array
//      expr = createArray(dimVals.data(), newArray->dims.size(), arrType->elem);
//      update = true;
//    }
//  }
//  else if(auto structMem = dynamic_cast<StructMem*>(expr))
//  {
//    update = foldExpression(structMem->base, isLHS) || update;
//    if(structMem->base->constant() && structMem->member.is<Variable*>())
//    {
//      Variable* var = structMem->member.get<Variable*>();
//      //Need to find which member var is, to extract it from compound literal
//      auto st = (StructType*) structMem->base->type;
//      for(size_t memIndex = 0; memIndex < st->members.size(); memIndex++)
//      {
//        if(st->members[memIndex] == var)
//        {
//          expr = ((CompoundLiteral*) structMem->base)->members[memIndex];
//          update = true;
//          break;
//        }
//      }
//      INTERNAL_ERROR;
//    }
//  }
//  else if(auto arrayLen = dynamic_cast<ArrayLength*>(expr))
//  {
//    update = foldExpression(arrayLen->array) || update;
//    if(arrayLen->array->constant())
//    {
//      int64_t len = ((CompoundLiteral*) arrayLen->array)->members.size();
//      expr = new IntConstant(len);
//      update = true;
//    }
//  }
//  else if(auto call = dynamic_cast<CallExpr*>(expr))
//  {
//    //can try to fold both the callable and each argument
//    update = foldExpression(call->callable) || update;
//    for(auto& arg : call->args)
//    {
//      update = foldExpression(arg) || update;
//    }
//  }
//  else if(auto isExpr = dynamic_cast<IsExpr*>(expr))
//  {
//    update = foldExpression(isExpr->base) || update;
//    if(auto uc = dynamic_cast<UnionConstant*>(isExpr->base))
//    {
//      expr = new BoolConstant(uc->value->type == isExpr->option);
//      update = true;
//    }
//  }
//  else if(auto asExpr = dynamic_cast<AsExpr*>(expr))
//  {
//    update = foldExpression(asExpr->base) || update;
//    if(auto uc = dynamic_cast<UnionConstant*>(asExpr->base))
//    {
//      if(!typesSame(uc->value->type, asExpr->option))
//      {
//        errMsgLoc(asExpr, "known at compile time that union value is not a " << asExpr->option->getName());
//      }
//      expr = uc->value;
//      update = true;
//    }
//  }
//  else if(!dynamic_cast<ThisExpr*>(expr))
//  {
//    INTERNAL_ERROR;
//  }
//  expr->setLocation(oldLoc);
//  return update;
//}
//
//void constantFold(SubroutineIR* subr)
//{
//  foldLocals = false;
//  //every expression (including parts of an assignment LHS)
//  //may be folded (i.e. myArray[5 + 3] = 8 % 3)
//  //
//  //recursivly attempt to fold all expressions (input and output) bottom-up
//  for(auto& stmt : subr->stmts)
//  {
//    if(auto assign = dynamic_cast<AssignIR*>(stmt))
//    {
//      foldExpression(assign->dst, true);
//      foldExpression(assign->src);
//    }
//    else if(auto ci = dynamic_cast<CallIR*>(stmt))
//    {
//      if(ci->thisObject)
//      {
//        foldExpression(ci->thisObject);
//      }
//      if(ci->callable.is<Expression*>())
//      {
//        foldExpression(ci->callable.get<Expression*>());
//      }
//      for(auto& a : ci->args)
//      {
//        foldExpression(a);
//      }
//    }
//    else if(auto condJump = dynamic_cast<CondJump*>(stmt))
//    {
//      foldExpression(condJump->cond);
//    }
//    else if(auto ret = dynamic_cast<ReturnIR*>(stmt))
//    {
//      if(ret->expr)
//        foldExpression(ret->expr);
//    }
//    else if(auto print = dynamic_cast<PrintIR*>(stmt))
//    {
//      foldExpression(print->expr);
//    }
//    else if(auto assertion = dynamic_cast<AssertionIR*>(stmt))
//    {
//      foldExpression(assertion->asserted);
//    }
//  }
//}
//
////Update local constant table for assignment of rhs to lhs
////Each kind of lvalue (var, indexed, structmem, compoundLit) handled separately
////"this" is assignable but is assumed to be NonConstant throughout all subroutines
////rhs may or may not be constant
////Pass "nullptr" for rhs to mean "some non-constant expression"
//bool bindValue(Expression* lhs, Expression* rhs)
//{
//  if(auto ve = dynamic_cast<VarExpr*>(lhs))
//  {
//    //don't care about parameters/globals being modified
//    if(ve->var->isLocal())
//    {
//      int varIndex = localConstants->varTable[ve->var];
//      if(rhs && rhs->constant())
//      {
//        return localConstants->update(varIndex, CPValue(rhs));
//      }
//      else
//        return localConstants->update(varIndex, CPValue(NON_CONSTANT));
//    }
//  }
//  else if(auto in = dynamic_cast<Indexed*>(lhs))
//  {
//    //assigning to an element poisons the whole LHS as non-const
//    return bindValue(in->group, nullptr);
//  }
//  else if(auto sm = dynamic_cast<StructMem*>(lhs))
//  {
//    return bindValue(sm->base, nullptr);
//  }
//  else if(auto clLHS = dynamic_cast<CompoundLiteral*>(lhs))
//  {
//    //bind value for individual elements if rhs is also a compound literal
//    //otherwise, every element of LHS is now non-const
//    bool update = false;
//    if(auto clRHS = dynamic_cast<CompoundLiteral*>(rhs))
//    {
//      for(size_t i = 0; i < clRHS->members.size(); i++)
//        update = bindValue(clLHS->members[i], clRHS->members[i]) || update;
//    }
//    else
//    {
//      for(auto mem : clLHS->members)
//        update = bindValue(mem, nullptr) || update;
//    }
//    return update;
//  }
//  else
//  {
//    INTERNAL_ERROR;
//  }
//  return false;
//}
//
//bool cpApplyStatement(StatementIR* stmt)
//{
//  bool update = false;
//  //Process all expressions in the statement in order of evaluation
//  //If it's an assign:
//  //  -whole RHS is processed before whole LHS (important for compound assignment)
//  //  -after processing side effects of LHS, update root variable's constant status
//  if(auto ai = dynamic_cast<AssignIR*>(stmt))
//  {
//    //first, process side effects of the whole RHS
//    update = cpProcessExpression(ai->src) || update;
//    //then process all side effects of LHS
//    update = cpProcessExpression(ai->dst, true) || update;
//    //finally, update the values of assigned variable(s)
//    //if in constant-folding mode, don't want to track updates to constant statuses
//    //(only care about modified Expressions in the IR)
//    if(foldLocals)
//      bindValue(ai->dst, ai->src);
//    else
//      update = bindValue(ai->dst, ai->src) || update;
//  }
//  else if(auto ci = dynamic_cast<CallIR*>(stmt))
//  {
//    //The output variable is marked non-constant,
//    //and if the call is a procedure then all
//    //constant globals must be marked non-constant
//
//    return cpProcessExpression(ev->eval);
//  }
//  else if(auto cj = dynamic_cast<CondJump*>(stmt))
//  {
//    return cpProcessExpression(cj->cond);
//  }
//  else if(auto ret = dynamic_cast<ReturnIR*>(stmt))
//  {
//    if(ret->expr)
//      return cpProcessExpression(ret->expr);
//    return false;
//  }
//  else if(auto pi = dynamic_cast<PrintIR*>(stmt))
//  {
//    return cpProcessExpression(pi->expr);
//  }
//  else if(auto asi = dynamic_cast<AssertionIR*>(stmt))
//  {
//    return cpProcessExpression(asi->asserted);
//  }
//  return update;
//}
//
////cpProcessExpression 
//bool cpProcessExpression(Expression*& expr, bool isLHS)
//{
//  bool update = false;
//  //Fold this expression each child expression using
//  //current (incoming) constant set and then process it to apply side effects
//  //Note that the ONLY type of expression that can have side effects on local
//  //variables is a member procedure call with a local variable as the base
//  //
//  //Also need to process children of all compound expressions
//  if(auto call = dynamic_cast<CallExpr*>(expr))
//  {
//    update = cpProcessExpression(call->callable) || update;
//    for(auto& arg : call->args)
//    {
//      update = cpProcessExpression(arg) || update;
//    }
//    if(auto sm = dynamic_cast<StructMem*>(call->callable))
//    {
//      if(!((CallableType*) sm->type)->pure)
//      {
//        Variable* root = sm->getRootVariable();
//        if(root->isLocal())
//        {
//          if(localConstants->update(root, CPValue(NON_CONSTANT)))
//            update = true;
//        }
//      }
//    }
//  }
//  else if(auto cl = dynamic_cast<CompoundLiteral*>(expr))
//  {
//    for(auto& mem : cl->members)
//    {
//      update = cpProcessExpression(mem, isLHS) || update;
//    }
//  }
//  else if(auto ua = dynamic_cast<UnaryArith*>(expr))
//  {
//    update = cpProcessExpression(ua->expr);
//  }
//  else if(auto ba = dynamic_cast<BinaryArith*>(expr))
//  {
//    update = cpProcessExpression(ba->lhs) || update;
//    update = cpProcessExpression(ba->rhs) || update;
//  }
//  else if(auto ind = dynamic_cast<Indexed*>(expr))
//  {
//    update = cpProcessExpression(ind->group, isLHS) || update;
//    update = cpProcessExpression(ind->index) || update;
//  }
//  else if(auto arrLen = dynamic_cast<ArrayLength*>(expr))
//  {
//    update = cpProcessExpression(arrLen->array);
//  }
//  else if(auto as = dynamic_cast<AsExpr*>(expr))
//  {
//    update = cpProcessExpression(as->base);
//  }
//  else if(auto is = dynamic_cast<IsExpr*>(expr))
//  {
//    update = cpProcessExpression(is->base);
//  }
//  else if(auto conv = dynamic_cast<Converted*>(expr))
//  {
//    update = cpProcessExpression(conv->value);
//  }
//  //else: no child expressions so no side effects possible
//  //but, now that side effects of children have been taken into account,
//  //is now the right time to fold the overall expression
//  if(foldLocals)
//    update = false;
//  Expression* prevExpr = expr;
//  foldExpression(expr, isLHS);
//  if(*prevExpr != *expr)
//    update = true;
//  return update;
//}
//
//bool constantPropagation(SubroutineIR* subrIR)
//{
//  bool anyUpdate = false;
//  auto subr = subrIR->subr;
//  if(subrIR->blocks.size() == 0)
//    return false;
//  localConstants = new LocalConstantTable(subrIR);
//  queue<int> processQueue;
//  //all blocks will be processed at least once
//  processQueue.push(0);
//  //blocks should be in queue at most once, so keep track with this
//  vector<bool> blocksInQueue(subrIR->blocks.size(), false);
//  blocksInQueue[0] = true;
//  //Keep track of which blocks have been processed at all.
//  //Blocks which are determined to be unreachable won't be processed at all,
//  //so their variable statuses won't be valid.
//  vector<bool> processedBlocks(subrIR->blocks.size(), false);
//  while(!processQueue.empty())
//  {
//    int process = processQueue.front();
//    processQueue.pop();
//    processedBlocks[process] = true;
//    // To process a block:
//    //  -Have some variable states at entry to block.
//    //   These have already been met with incoming blocks when they were processed.
//    //   Save a copy of this so original can be modified temporarily.
//    //  -Go through statements in BB, updating var statuses
//    //  -Meet new outgoing var statuses with those of all outgoing blocks
//    //  -If any var status changes in an outgoing block, put it in queue
//    vector<CPValue> savedConstants = localConstants->constants[process];
//    bool update = false;
//    blocksInQueue[process] = false;
//    currentBB = process;
//    auto processBlock = subrIR->blocks[process];
//    foldLocals = false;
//    //apply effects of each statement (and its expressions) to constant statuses
//    for(int i = processBlock->start; i < processBlock->end; i++)
//    {
//      cpApplyStatement(subrIR->stmts[i]);
//    }
//    //meet the statuses with all outgoing blocks that are reachable
//    //given current set of constants
//    //
//    //This is always correct since in all cases where the condition turns out non-constant,
//    //currentBB will have to be reached and processed again by another block
//    //
//    //Note that a CondJump is the only instruction with multiple outgoing blocks
//    vector<BasicBlock*> reachableOut;
//    bool prunedOutgoing = false;
//    if(auto cj = dynamic_cast<CondJump*>(subrIR->stmts[processBlock->end - 1]))
//    {
//      //Since a CondJump can only have local var side effects through a call,
//      //don't need to worry about folding the condition before vs. after applying side effects
//      //
//      //Also, don't overwrite the condition in the CondJump, since this folding may not actually
//      //be correct in the final version
//      Expression* cond = cj->cond->copy();
//      foldLocals = true;
//      foldExpression(cond);
//      foldLocals = false;
//      if(auto constCond = dynamic_cast<BoolConstant*>(cond))
//      {
//        //assume branch always falls through or is always taken,
//        //depending on constCoord
//        BasicBlock* constDest = nullptr;
//        if(constCond->value)
//          constDest = subrIR->blocks[process + 1];
//        else
//          constDest = subrIR->blockStarts[cj->taken->intLabel];
//        //it is only safe to treat this jump as const if the
//        //condjump is never reachable again from the non-taken branch
//        if(!subrIR->reachable(constDest, processBlock))
//        {
//          reachableOut.push_back(constDest);
//          prunedOutgoing = true;
//        }
//      }
//    }
//    if(!prunedOutgoing)
//    {
//      //could not prune any paths: will process all ougoing paths
//      reachableOut = processBlock->out;
//    }
//    for(auto out : reachableOut)
//    {
//      bool outUpdated = false;
//      for(size_t i = 0; i < localConstants->locals.size(); i++)
//      {
//        if(localConstants->meetUpdate(i, out->index, localConstants->constants[currentBB][i]))
//          outUpdated = true;
//      }
//      if(outUpdated && !blocksInQueue[out->index])
//      {
//        blocksInQueue[out->index] = true;
//        processQueue.push(out->index);
//      }
//    }
//    //restore saved statuses from start of current BB
//    localConstants->constants[currentBB] = savedConstants;
//    if(update)
//      anyUpdate = true;
//  }
//  // Now that BB-level var statuses have stabilized:
//  //  -go through each BB one more time, folding expressions (and local VarExprs) and updating statuses with statements
//  //  -don't need to make a copy of statuses since they won't be used again
//  foldLocals = true;
//  for(size_t i = 0; i < subrIR->blocks.size(); i++)
//  {
//    if(!processedBlocks[i])
//      continue;
//    auto bb = subrIR->blocks[i];
//    currentBB = bb->index;
//    for(int j = bb->start; j < bb->end; j++)
//    {
//      if(cpApplyStatement(subrIR->stmts[j]))
//        anyUpdate = true;
//    }
//  }
//  delete localConstants;
//  localConstants = nullptr;
//  return anyUpdate;
//}
//
//bool operator==(const CPValue& lhs, const CPValue& rhs)
//{
//  if(lhs.val.which() != rhs.val.which())
//    return false;
//  if(lhs.val.is<Expression*>())
//  {
//    return *lhs.val.get<Expression*>() == *rhs.val.get<Expression*>();
//  }
//  return true;
//}
//
//ostream& operator<<(ostream& os, const CPValue& cv)
//{
//  if(cv.val.is<UndefinedVal>())
//    os << "<UNDEFINED>";
//  else if(cv.val.is<NonConstant>())
//    os << "<NON-CONSTANT>";
//  else
//    os << cv.val.get<Expression*>();
//  return os;
//}
//
