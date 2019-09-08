#include "AstInterpreter.hpp"
#include "Variable.hpp"

Interpreter::Interpreter(Subroutine* subr, vector<Expression*> args)
{
  returning = false;
  callSubr(subr, args);
}

Expression* Interpreter::callSubr(Subroutine* subr, vector<Expression*> args, Expression* thisExpr)
{
  returning = false;
  //push stack frame
  frames.emplace();
  StackFrame& current = frames.top();
  current.thisExpr = thisExpr;
  //Execute statements in linear sequence.
  //If a return is encountered, execute() returns and
  //the return value will be placed in the frame rv
  for(auto s : subr->body->stmts)
  {
    execute(s);
    if(returning)
    {
      frames.pop();
      return rv;
    }
  }
  frames.pop();
  //implicit return, so there is no return value.
  //Check that the subroutine doesn't return anything,
  //since the resolved AST can't check for missing returns
  if(!subr->type->returnType->isSimple())
  {
    errMsgLoc(subr, "interpreter reached end of subroutine without a return value");
  }
  return nullptr;
}

Expression* Interpreter::callExtern(ExternalSubroutine* exSubr, vector<Expression*> args)
{
  //TODO: lazily load correct dynamic library, put arguments in correct binary format, and call
  return nullptr;
}

void Interpreter::execute(Statement* stmt)
{
  if(breaking || continuing || returning)
    return;
  if(auto assign = dynamic_cast<Assign*>(stmt))
  {
    Expression* rvalue = evaluate(assign->rvalue);
    if(auto compoundAssign = dynamic_cast<CompoundLiteral*>(assign->lvalue))
    {
      //rvalue (fully evaluated to a constant) should also be a CompoundLiteral.
      //Do the assignment one element at a time
      auto compoundRHS = dynamic_cast<CompoundLiteral*>(rvalue);
      INTERNAL_ASSERT(compoundRHS);
      size_t n = compoundAssign->members.size();
      INTERNAL_ASSERT(n == compoundRHS->members.size());
      for(size_t i = 0; i < n; i++)
      {
        //evaluate symbolic lvalue, and directly assign rvalue
        evaluateLValue(compoundAssign->members[i]) = compoundRHS->members[i];
      }
    }
    else
    {
      evaluateLValue(assign->lvalue) = rvalue;
    }
  }
  else if(auto block = dynamic_cast<Block*>(stmt))
  {
    for(auto bstmt : block->stmts)
    {
      execute(bstmt);
      if(breaking || continuing || returning)
        return;
    }
  }
  else if(auto call = dynamic_cast<CallStmt*>(stmt))
  {
    evaluate(call->eval);
  }
  else if(auto fc = dynamic_cast<ForC*>(stmt))
  {
    //Initialize the loop
    if(fc->init)
      execute(fc->init);
    while(true)
    {
      //Evaluate the condition
      BoolConstant* cond = dynamic_cast<BoolConstant*>(evaluate(fc->condition));
      INTERNAL_ASSERT(cond);
      if(!cond->value)
        break;
      execute(fc->inner);
      if(breaking)
      {
        breaking = false;
        break;
      }
      else if(continuing)
        continuing = false;
      else if(returning)
        break;
      //"continue" is implicit: body execution breaks immediately and the loop continues
      if(fc->increment)
        execute(fc->increment);
    }
  }
  else if(auto fr = dynamic_cast<ForRange*>(stmt))
  {
    //Create a statement to initialize counter, and another to increment
    VarExpr* ve = new VarExpr(fr->counter);
    Statement* init = new Assign(fr->block, ve, ASSIGN, fr->begin);
    Statement* incr = new Assign(fr->block, ve, INC);
    Expression* cond = new BinaryArith(ve, CMPL, fr->end);
    init->resolve();
    incr->resolve();
    cond->resolve();
    while(true)
    {
      BoolConstant* condVal = dynamic_cast<BoolConstant*>(evaluate(cond));
      INTERNAL_ASSERT(condVal);
      if(!condVal->value)
        break;
      execute(fc->inner);
      if(breaking)
      {
        breaking = false;
        break;
      }
      else if(continuing)
        continuing = false;
      else if(returning)
        break;
      execute(incr);
    }
  }
  else if(auto fa = dynamic_cast<ForArray*>(stmt))
  {
    Expression* zero = new IntConstant((int64_t) 0, primitives[Prim::LONG]);
    zero->resolve();
    int dims = fa->counters.size();
    vector<Expression*> counterExprs;
    for(auto counter : fa->counters)
      counterExprs.push_back(new VarExpr(counter));
    //A ragged array is easily iterated using DFS over a tree.
    //Use a stack to store the nodes which must still be visited.
    //zero out first counter
    vector<Assign*> counterZero;
    vector<int> indices(dims, 0);
    stack<pair<Expression*, int>> toVisit;
    toVisit.emplace(evaluate(fa->arr), 0);
    while(!toVisit.empty())
    {
      Expression* visit;
      int depth;
      visit = toVisit.top().first;
      depth = toVisit.top().second;
      toVisit.pop();
      if(depth == dims - 1)
      {
        //innermost dimension, assign iter's value
        assignVar(fa->iter, visit);
      }
      else
      {
        //push elements of visit in reverse order
        if(auto cl = dynamic_cast<CompoundLiteral*>(visit))
          for(int i = cl->members.size() - 1; i >= 0; i--)
            toVisit.emplace(cl->members[i], depth + 1);
        else
        {
          StringConstant* sc = dynamic_cast<StringConstant*>(visit);
          INTERNAL_ASSERT(sc);
          string val = sc->value;
          for(int i = val.length() - 1; i >= 0; i--)
            toVisit.emplace(new CharConstant(val[i]), depth + 1);
        }
        //zero out next dim's counter,
        assignVar(fa->counters[depth + 1], zero);
      }
      //execute the body
      execute(fa->inner);
      if(breaking)
      {
        breaking = false;
        break;
      }
      else if(continuing)
        continuing = false;
      else if(returning)
        break;
      //finally, increment the counter for the current dimension
      IntConstant* countVal = dynamic_cast<IntConstant*>(readVar(fa->counters[depth]));
      INTERNAL_ASSERT(countVal);
      //"long" is the type used for counters
      INTERNAL_ASSERT(countVal->isSigned());
      countVal->sval++;
    }
  }
  else if(auto w = dynamic_cast<While*>(stmt))
  {
    while(true)
    {
      BoolConstant* cond = dynamic_cast<BoolConstant*>(evaluate(w->condition));
      INTERNAL_ASSERT(cond);
      if(!cond->value)
        break;
      execute(w->body);
      if(breaking)
      {
        breaking = false;
        break;
      }
      else if(continuing)
        continuing = false;
      else if(returning)
        break;
    }
  }
  else if(auto ifStmt = dynamic_cast<If*>(stmt))
  {
    BoolConstant* cond = dynamic_cast<BoolConstant*>(evaluate(ifStmt->condition));
    INTERNAL_ASSERT(cond);
    if(cond->value)
      execute(ifStmt->body);
    else if(ifStmt->elseBody)
      execute(ifStmt->elseBody);
  }
  else if(auto ret = dynamic_cast<Return*>(stmt))
  {
    if(ret->value)
      rv = evaluate(ret->value);
    returning = true;
  }
  else if(dynamic_cast<Break*>(stmt))
  {
    breaking = true;
  }
  else if(dynamic_cast<Continue*>(stmt))
  {
    continuing = true;
  }
  else if(auto print = dynamic_cast<Print*>(stmt))
  {
    for(auto e : print->exprs)
      cout << evaluate(e);
  }
  else if(auto assertion = dynamic_cast<Assertion*>(stmt))
  {
    BoolConstant* bc = dynamic_cast<BoolConstant*>(evaluate(assertion->asserted));
    assert(bc);
    if(!bc->value)
    {
      errMsgLoc(assertion, "Assertion failed: " << assertion->asserted);
    }
  }
  else if(auto sw = dynamic_cast<Switch*>(stmt))
  {
    Expression* switched = evaluate(sw->switched);
    //Run down list of cases, comparing value
    int label = sw->defaultPosition;
    for(size_t i = 0; i < sw->caseValues.size(); i++)
    {
      if(*switched == *sw->caseValues[i])
      {
        label = sw->caseLabels[i];
        break;
      }
    }
    //begin executing body at the proper position
    for(size_t i = label; i < sw->block->stmts.size(); i++)
    {
      execute(sw->block->stmts[i]);
      if(breaking)
      {
        breaking = false;
        return;
      }
      else if(continuing || returning)
        return;
    }
  }
  else if(auto ma = dynamic_cast<Match*>(stmt))
  {
    UnionConstant* uc = dynamic_cast<UnionConstant*>(evaluate(ma->matched));
    INTERNAL_ASSERT(uc);
    for(size_t i = 0; i < ma->types.size(); i++)
    {
      if(typesSame(uc->value->type, ma->types[i]))
      {
        assignVar(ma->caseVars[i], uc->value);
        execute(ma->cases[i]);
        if(breaking)
          breaking = false;
        break;
      }
    }
  }
  else
  {
    INTERNAL_ERROR;
  }
}

CompoundLiteral* Interpreter::createArray(uint64_t* dims, int ndims, Type* elem, Expression* fillVal)
{
  vector<Expression*> elems;
  for(uint64_t i = 0; i < dims[0]; i++)
  {
    if(ndims == 1)
    {
      elems.push_back(fillVal);
    }
    else
    {
      elems.push_back(createArray(&dims[1], ndims - 1, elem, fillVal));
    }
  }
  CompoundLiteral* cl = new CompoundLiteral(elems);
  cl->type = getArrayType(elem, ndims);
  cl->resolved = true;
  return cl;
}

Expression* Interpreter::convertConstant(Expression* value, Type* type)
{
  type = canonicalize(type);
  if(typesSame(value->type, type))
    return value;
  //For converting union constants, use the underlying value
  if(auto uc = dynamic_cast<UnionConstant*>(value))
  {
    value = uc->value;
  }
  Node* loc = value;
  INTERNAL_ASSERT(value->constant());
  int option = -1;
  auto structDst = dynamic_cast<StructType*>(type);
  if(auto unionDst = dynamic_cast<UnionType*>(type))
  {
    for(size_t i = 0; i < unionDst->options.size(); i++)
    {
      if(typesSame(unionDst->options[i], value->type))
      {
        option = i;
        break;
      }
    }
    if(option < 0)
    {
      for(size_t i = 0; i < unionDst->options.size(); i++)
      {
        if(unionDst->options[i]->canConvert(value->type))
        {
          option = i;
          value = convertConstant(value, unionDst->options[i]);
          break;
        }
      }
    }
    INTERNAL_ASSERT(option >= 0);
    value = new UnionConstant(value, unionDst->options[option], unionDst);
    value->setLocation(loc);
    return value;
  }
  else if(structDst && structDst->members.size() == 1 &&
      !dynamic_cast<CompoundLiteral*>(value))
  {
    //Single-member struct is equivalent to the member
    vector<Expression*> mem(1, value);
    auto cl = new CompoundLiteral(mem);
    cl->resolve();
    cl->setLocation(loc);
    cl->type = type;
    return cl;
  }
  else if(structDst)
  {
    auto clRHS = dynamic_cast<CompoundLiteral*>(value);
    INTERNAL_ASSERT(clRHS);
    //just need to convert the elements in a
    //CompoundLiteral to match struct member types 
    vector<Expression*> mems;
    for(size_t i = 0; i < clRHS->members.size(); i++)
    {
      mems.push_back(convertConstant(clRHS->members[i], structDst->members[i]->type));
    }
    clRHS->setLocation(value);
    clRHS->resolve();
    clRHS->setLocation(loc);
    clRHS->type = type;
    return clRHS;
  }
  else if(auto intConst = dynamic_cast<IntConstant*>(value))
  {
    //do the conversion which tests for overflow and enum membership
    value = intConst->convert(type);
    value->setLocation(loc);
    return value;
  }
  else if(auto charConst = dynamic_cast<CharConstant*>(value))
  {
    //char is equivalent to an 8-bit unsigned for purposes of value conversion
    value = new IntConstant((uint64_t) charConst->value);
    value->setLocation(loc);
    return value;
  }
  else if(auto floatConst = dynamic_cast<FloatConstant*>(value))
  {
    value = floatConst->convert(type);
    value->setLocation(loc);
    return value;
  }
  else if(auto enumConst = dynamic_cast<EnumExpr*>(value))
  {
    if(enumConst->value->fitsS64)
    {
      //make a signed temporary int constant, then convert that
      //(this can't possible lose information)
      IntConstant asInt(enumConst->value->sval);
      asInt.setLocation(loc);
      return asInt.convert(type);
    }
    else
    {
      IntConstant asInt(enumConst->value->uval);
      asInt.setLocation(loc);
      return asInt.convert(type);
    }
  }
  //array/struct/tuple constants can be converted implicitly
  //to each other (all use CompoundLiteral) but individual
  //members (primitives) may need conversion
  else if(auto compLit = dynamic_cast<CompoundLiteral*>(value))
  {
    //attempt to fold all elements (can't proceed unless every
    //one is a constant)
    if(auto st = dynamic_cast<StructType*>(type))
    {
      for(size_t i = 0; i < compLit->members.size(); i++)
      {
        if(!typesSame(compLit->members[i]->type, st->members[i]->type))
        {
          compLit->members[i] = convertConstant(
              compLit->members[i], st->members[i]->type);
        }
        //else: don't need to convert member
      }
      compLit->type = type;
      return compLit;
    }
    else if(auto tt = dynamic_cast<TupleType*>(type))
    {
      for(size_t i = 0; i < compLit->members.size(); i++)
      {
        if(!typesSame(compLit->members[i]->type, tt->members[i]))
        {
          compLit->members[i] = convertConstant(
              compLit->members[i], tt->members[i]);
        }
      }
      compLit->type = type;
      return compLit;
    }
    else if(auto at = dynamic_cast<ArrayType*>(type))
    {
      //Array lit is just another CompoundLiteral,
      //but with array type instead of tuple
      vector<Expression*> elements;
      for(auto& elem : compLit->members)
      {
        elements.push_back(convertConstant(elem, at->subtype));
      }
      CompoundLiteral* arrayLit = new CompoundLiteral(elements);
      arrayLit->resolved = true;
      arrayLit->type = at;
      arrayLit->type = type;
      return arrayLit;
    }
    else if(auto mt = dynamic_cast<MapType*>(type))
    {
      auto mc = new MapConstant(mt);
      //add each key/value pair to the map
      for(size_t i = 0; i < compLit->members.size(); i++)
      {
        auto kv = dynamic_cast<CompoundLiteral*>(compLit->members[i]);
        INTERNAL_ASSERT(kv);
        Expression* key = kv->members[0];
        Expression* val = kv->members[1];
        if(!typesSame(key->type, mt->key))
          key = convertConstant(key, mt->key);
        if(!typesSame(val->type, mt->value))
          val = convertConstant(val, mt->value);
        mc->values[key] = val;
      }
      mc->type = type;
      return mc;
    }
  }
  //????
  cout << "Failed to convert constant expr \"" << value << "\" to type \"" << type->getName() << "\"\n";
  cout << "Value's type is " << value->type->getName() << '\n';
  INTERNAL_ERROR;
  return nullptr;
}

Expression* Interpreter::evaluate(Expression* e)
{
  if(e->constant())
  {
    //nothing to do, except convert StringConstant
    //to char[] (this is required because StringConstants are not
    //mutable, but variables of string type need to be)
    if(auto sc = dynamic_cast<StringConstant*>(e))
    {
      vector<Expression*> chars;
      for(size_t i = 0; i < sc->value.length(); i++)
        chars.push_back(new CharConstant(sc->value[i]));
      return new CompoundLiteral(chars);
    }
    return e;
  }
  if(e->assignable())
  {
    //Don't want modifications to apply to the original.
    return evaluateLValue(e)->copy();
  }
  if(auto ua = dynamic_cast<UnaryArith*>(e))
  {
    Expression* operand = evaluate(ua->expr);
    switch(ua->op)
    {
      case LNOT:
        {
          //Logical NOT
          BoolConstant* bc = dynamic_cast<BoolConstant*>(operand);
          INTERNAL_ASSERT(bc);
          return new BoolConstant(!bc->value);
        }
      case BNOT:
        {
          //Bitwise NOT: only applies to integers
          IntConstant* ic = dynamic_cast<IntConstant*>(operand->copy());
          INTERNAL_ASSERT(ic);
          if(ic->isSigned())
            ic->sval = ~ic->sval;
          else
            ic->uval = ~ic->uval;
          return ic;
        }
      case SUB:
        {
          //Negation: applies to integers and floats
          Expression* copy = operand->copy();
          if(auto ic = dynamic_cast<IntConstant*>(copy))
          {
            if(ic->isSigned())
              ic->sval = -ic->sval;
            else
              ic->uval = -ic->uval;
          }
          else if(auto fc = dynamic_cast<FloatConstant*>(copy))
          {
            if(fc->isDoublePrec())
              fc->dp = -fc->dp;
            else
              fc->fp = -fc->fp;
          }
          return copy;
        }
      default:
        INTERNAL_ERROR;
    }
    return nullptr;
  }
  else if(auto ba = dynamic_cast<BinaryArith*>(e))
  {
    //first, intercept short-circuit evaluation cases (logical AND/OR)
    //need to fold only the LHS, then replace value immediately if it short-circuits
    //NOTE: since constant folds never have side effects, this change never
    //affects program semantics but may allow folding when otherwise impossible
    Expression* lhs = evaluate(ba->lhs);
    Expression* rhs = evaluate(ba->rhs);
    int op = ba->op;
    //Comparison operations easy because
    //all constant Expressions support comparison,
    //with semantics matching the language
    switch(op)
    {
      //note: ordering operators are not implemented for all Expression subclasses,
      //but they are implemented for all constant types where they're allowed in syntax
      case CMPEQ:
        return new BoolConstant(*lhs == *rhs);
      case CMPNEQ:
        return new BoolConstant(!(*lhs == *rhs));
      case CMPL:
        return new BoolConstant(*lhs < *rhs);
      case CMPG:
        return new BoolConstant(*rhs < *lhs);
      case CMPLE:
        return new BoolConstant(!(*rhs < *lhs));
      case CMPGE:
        return new BoolConstant(!(*lhs < *rhs));
      default:;
    }
    if(op == PLUS)
    {
      //handle array concat, prepend and append operations (not numeric + yet)
      CompoundLiteral* compoundLHS = dynamic_cast<CompoundLiteral*>(lhs);
      CompoundLiteral* compoundRHS = dynamic_cast<CompoundLiteral*>(rhs);
      if(compoundLHS && compoundRHS)
      {
        vector<Expression*> resultMembers(compoundLHS->members.size() + compoundRHS->members.size());
        for(size_t i = 0; i < compoundLHS->members.size(); i++)
          resultMembers[i] = compoundLHS->members[i];
        for(size_t i = 0; i < compoundRHS->members.size(); i++)
          resultMembers[i + compoundLHS->members.size()] = compoundRHS->members[i];
        CompoundLiteral* result = new CompoundLiteral(resultMembers);
        result->resolve();
        return result;
      }
      else if(compoundLHS)
      {
        //array append
        vector<Expression*> resultMembers = compoundLHS->members;
        resultMembers.push_back(rhs);
        CompoundLiteral* result = new CompoundLiteral(resultMembers);
        result->resolve();
        return result;
      }
      else if(compoundRHS)
      {
        //array prepend
        vector<Expression*> resultMembers(1 + compoundRHS->members.size());
        resultMembers[0] = lhs;
        for(size_t i = 0; i < compoundRHS->members.size(); i++)
        {
          resultMembers[i + 1] = compoundRHS->members[i];
        }
        CompoundLiteral* result = new CompoundLiteral(resultMembers);
        result->resolve();
        return result;
      }
    }
    else if(op == LOR)
    {
      auto lhsBool = (BoolConstant*) lhs;
      auto rhsBool = (BoolConstant*) rhs;
      return new BoolConstant(lhsBool->value || rhsBool->value);
    }
    else if(op == LAND)
    {
      auto lhsBool = (BoolConstant*) lhs;
      auto rhsBool = (BoolConstant*) rhs;
      return new BoolConstant(lhsBool->value && rhsBool->value);
    }
    //all other binary ops are numerical operations between two ints or two floats
    FloatConstant* lhsFloat = dynamic_cast<FloatConstant*>(lhs);
    FloatConstant* rhsFloat = dynamic_cast<FloatConstant*>(rhs);
    IntConstant* lhsInt = dynamic_cast<IntConstant*>(lhs);
    IntConstant* rhsInt = dynamic_cast<IntConstant*>(rhs);
    bool useFloat = lhsFloat && rhsFloat;
    bool useInt = lhsInt && rhsInt;
    if(useFloat)
      return lhsFloat->binOp(op, rhsFloat);
    INTERNAL_ASSERT(useInt);
    return lhsInt->binOp(op, rhsInt);
  }
  else if(auto cl = dynamic_cast<CompoundLiteral*>(e))
  {
    vector<Expression*> elems;
    for(auto mem : cl->members)
      elems.push_back(evaluate(mem));
    return new CompoundLiteral(elems);
  }
  else if(auto ind = dynamic_cast<Indexed*>(e))
  {
    Expression* group = evaluate(ind->group);
    Expression* index = evaluate(ind->index);
    //arrays are represented as either CompoundLiteral or StringConstant
    auto compLit = dynamic_cast<CompoundLiteral*>(group);
    auto stringConstant = dynamic_cast<StringConstant*>(group);
    if(compLit || stringConstant)
    {
      //an array, so index should be an integer
      IntConstant* ic = dynamic_cast<IntConstant*>(index);
      INTERNAL_ASSERT(ic);
      unsigned ord = 0;
      if(ic->isSigned())
      {
        if(ic->sval < 0)
          errMsgLoc(ind, "negative array index");
        ord = ic->sval;
      }
      else
        ord = ic->uval;
      if(compLit)
      {
        if(ord >= compLit->members.size())
          errMsgLoc(ind, "array index " << ord << " out of bound " << compLit->members.size());
        return compLit->members[ord];
      }
      else
      {
        if(ord >= stringConstant->value.length())
          errMsgLoc(ind, "string index out of bounds");
        return new CharConstant(stringConstant->value[ord]);
      }
    }
    else if(auto mc = dynamic_cast<MapConstant*>(group))
    {
      MapType* mapType = (MapType*) mc->type;
      //if key (index) is not already in the map, insert it and default-initialize the value
      if(mc->values.find(group) == mc->values.end())
        mc->values[group] = mapType->value->getDefaultValue();
      return mc->values[group];
    }
    INTERNAL_ERROR;
  }
  else if(auto call = dynamic_cast<CallExpr*>(e))
  {
    //evaluate callable, then args in order
    auto callable = evaluate(call->callable);
    vector<Expression*> args;
    for(auto a : call->args)
      args.push_back(evaluate(a));
    //All "constant" callables must be SubroutineExpr
    auto subExpr = dynamic_cast<SubroutineExpr*>(callable);
    INTERNAL_ASSERT(subExpr);
    if(subExpr->subr)
      callSubr(subExpr->subr, args, subExpr->thisObject);
    else
      callExtern(subExpr->exSubr, args);
  }
  else if(auto sm = dynamic_cast<StructMem*>(e))
  {
    //All structs are represented as CompoundLiteral
    CompoundLiteral* base = dynamic_cast<CompoundLiteral*>(evaluate(sm->base));
    INTERNAL_ASSERT(base);
    StructType* structType = dynamic_cast<StructType*>(base->type);
    INTERNAL_ASSERT(structType);
    auto& dataMems = structType->members;
    if(sm->member.is<Variable*>())
    {
      for(size_t i = 0; i < dataMems.size(); i++)
      {
        if(dataMems[i] == sm->member.get<Variable*>())
          return base->members[i];
      }
      INTERNAL_ERROR;
    }
    else
    {
      //Member subroutine - return a SubroutineExpr
      return new SubroutineExpr(base, sm->member.get<Subroutine*>());
    }
    INTERNAL_ERROR;
    return nullptr;
  }
  else if(auto na = dynamic_cast<NewArray*>(e))
  {
    vector<uint64_t> dims;
    for(auto d : na->dims)
    {
      IntConstant* ic = dynamic_cast<IntConstant*>(d);
      INTERNAL_ASSERT(ic);
      if(ic->isSigned())
        dims.push_back(ic->sval);
      else
        dims.push_back(ic->uval);
    }
    Type* elem = ((ArrayType*) na->type)->elem;
    return createArray(dims.data(), na->dims.size(), elem, elem->getDefaultValue());
  }
  else if(auto al = dynamic_cast<ArrayLength*>(e))
  {
    //note: the type of this expression is always "long"
    Expression* arr = evaluate(al->array);
    auto compLit = dynamic_cast<CompoundLiteral*>(arr);
    INTERNAL_ASSERT(compLit);
    return new IntConstant((int64_t) compLit->members.size());
  }
  else if(auto ie = dynamic_cast<IsExpr*>(e))
  {
    UnionConstant* uc = dynamic_cast<UnionConstant*>(evaluate(ie->base));
    INTERNAL_ASSERT(uc);
    return new BoolConstant(uc->option == ie->optionIndex);
  }
  else if(auto ae = dynamic_cast<AsExpr*>(e))
  {
    UnionConstant* uc = dynamic_cast<UnionConstant*>(evaluate(ae->base));
    INTERNAL_ASSERT(uc);
    if(uc->option != ae->optionIndex)
      errMsgLoc(ae, "union value does not have the type expected by \"as\"");
    return ae->base;
  }
  else if(dynamic_cast<ThisExpr*>(e))
  {
    return frames.top().thisExpr;
  }
  else if(auto conv = dynamic_cast<Converted*>(e))
  {
    return convertConstant(evaluate(conv->value), conv->type);
  }
  INTERNAL_ERROR;
  return nullptr;
}

Expression*& Interpreter::evaluateLValue(Expression* e)
{
  if(auto v = dynamic_cast<VarExpr*>(e))
  {
    return readVar(v->var);
  }
  else if(auto sm = dynamic_cast<StructMem*>(e))
  {
    //the sm's base must be an lvalue, and from that
    //the member can be accessed
    CompoundLiteral* base = dynamic_cast<CompoundLiteral*>(evaluateLValue(sm->base));
    INTERNAL_ASSERT(base);
    StructType* structType = dynamic_cast<StructType*>(sm->base->type);
    INTERNAL_ASSERT(structType);
    auto& dataMems = structType->members;
    //Only variable members are mutable!
    //Subroutine members are immutable parts of a struct type's interface.
    INTERNAL_ASSERT(sm->member.is<Variable*>());
    for(size_t i = 0; i < dataMems.size(); i++)
    {
      if(dataMems[i] == sm->member.get<Variable*>())
        return base->members[i];
    }
    INTERNAL_ERROR;
  }
  else if(auto ind = dynamic_cast<Indexed*>(e))
  {
    Expression*& group = evaluateLValue(ind->group);
    Expression* index = evaluate(ind->index);
    //arrays are represented as either CompoundLiteral or StringConstant
    auto cl = dynamic_cast<CompoundLiteral*>(group);
    if(cl)
    {
      //an array, so index should be an integer
      IntConstant* ic = dynamic_cast<IntConstant*>(index);
      INTERNAL_ASSERT(ic);
      unsigned ord = 0;
      if(ic->isSigned())
      {
        if(ic->sval < 0)
          errMsgLoc(ind, "negative array index");
        ord = ic->sval;
      }
      else
        ord = ic->uval;
      if(ord >= cl->members.size())
        errMsgLoc(ind, "array index out of bounds");
      return cl->members[ord];
    }
    else if(auto mc = dynamic_cast<MapConstant*>(group))
    {
      MapType* mapType = (MapType*) mc->type;
      //if key (index) is not already in the map, insert it and default-initialize the value
      if(mc->values.find(group) == mc->values.end())
        mc->values[group] = mapType->value->getDefaultValue();
      return mc->values[group];
    }
    INTERNAL_ERROR;
  }
  else if(dynamic_cast<ThisExpr*>(e))
  {
    if(auto thisVar = dynamic_cast<VarExpr*>(frames.top().thisExpr))
    {
      //return a reference to the var referred to be "this"
      return evaluateLValue(thisVar);
    }
    else
    {
      //return a direct reference to thisExpr
      return frames.top().thisExpr;
    }
  }
  INTERNAL_ERROR;
}

void Interpreter::assignVar(Variable* v, Expression* e)
{
  if(!e->constant())
    e = evaluate(e);
  //Only have to search in top stack frame, and globals
  if(globals.find(v) != globals.end())
  {
    globals[v] = e;
  }
  else
  {
    //a reference to local must be in the current frame
    frames.top().locals[v] = e;
  }
}

Expression*& Interpreter::readVar(Variable* v)
{
  if(globals.find(v) != globals.end())
    return globals[v];
  return frames.top().locals[v];
}

