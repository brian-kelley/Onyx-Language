#include "Subroutine.hpp"
#include "Variable.hpp"

using namespace TypeSystem;

extern bool programHasMain;
extern ModuleScope* global;

vector<Test*> Test::tests;

//Block which is body of subroutine
Block::Block(Subroutine* s)
{
  breakable = None;
  loop = None;
  subr = s;
  scope = new Scope(s->scope);
  scope->node = this;
  statementCount = 0;
}

Block::Block(Block* parent)
{
  breakable = parent->breakable;
  loop = parent->loop;
  subr = parent->subr;
  scope = new Scope(parent->scope);
  scope->node = this;
  statementCount = 0;
}

//Block which is a for loop body
//The loop body has the same index as the loop itself
Block::Block(For* f, Block* parent)
{
  subr = parent->subr;
  loop = f;
  breakable = f;
  scope = new Scope(parent->scope);
  scope->node = this;
  statementCount = 0;
}

//Block which is a while loop body
Block::Block(While* w)
{
  Block* parent = w->body->scope->parent->node.get<Block*>();
  subr = parent->subr;
  loop = w;
  breakable = w;
  scope = new Scope(parent->scope);
  scope->node = this;
  statementCount = 0;
}

void Block::resolve(bool final)
{
  for(auto& stmt : stmts)
  {
    stmt->resolve(final);
    if(!stmt->resolved)
      return;
  }
  resolved = true;
}

Assign::Assign(Block* b, Expression* lhs, Expression* rhs) : Statement(b)
{
  lvalue = lhs;
  rvalue = rhs;
}

void Assign::resolve(bool final)
{
  resolveExpr(lvalue, final);
  resolveExpr(rvalue, final);
  if(!lvalue->resolved || !rvalue->resolved)
  {
    return;
  }
  if(!lvalue->assignable())
  {
    errMsgLoc(this, "left-hand side of assignment is immutable");
  }
  if(!rvalue->type->canConvert(lvalue->type))
  {
    errMsgLoc(this, "cannot convert from " << rvalue->type->getName() << " to " << lvalue->type->getName());
  }
  if(lvalue->type != rvalue->type)
  {
    rvalue = new Converted(rvalue, lvalue->type);
    rvalue->resolve(true);
  }
  resolved = true;
}

CallStmt(Block* b, CallExpr* e) : Statement(b)
{
  eval = e;
}

void CallStmt::resolve(bool final)
{
  eval->resolve(final);
  if(!eval->resolved)
    return;
  resolved = true;
}

For::For(Block* b, Statement* i, Expression* cond, Statement* incr, Block* forBody)
  : Statement(b)
{
  //variables declared in initialization should be in body's scope
  init = i;
  condition = cond;
  increment = incr;
  body = forBody;
}

For::For(Block* b, vector<string>& tupIter, Expression* arr, Block* innerBody)
  : Statement(b)
{
  //recursively create the syntax of a nested for loop over each dim
  //body is the bodyh of innermost loop only
  if(tupIter.size() < 2)
  {
    errMsgLoc(this, "for over array needs >= 1 counter and an iteration variable");
  }
  body = new Block(this);
  setupRange(tupIter.front(), new IntLiteral(0), new ArrayLength(arr));
  //get arr[i] where i is this dimension's counter
  Expression* subArr = new Indexed(arr,
      new UnresolvedExpr(tupIter.front(), body->scope));
  if(tupIter.size() == 2)
  {
    //innermost loop
    //create the final iteration variable and assign subArr to it
    Variable* iterVar = new Variable(tupIter.back(), new ExprType(subArr), innerBody);
    body->scope->addName(iterVar);
    body->stmts.push_back(new Assign(new VarExpr(iterVar), subArr));
    //finally, add the inner body,
    //and fix its position in scope tree
    innerBody->scope->parent = body->scope;
    body->stmts.push_back(innerBody);
  }
  else
  {
    //not innermost loop,
    //only statement in body is another For (with same innerBody)
    vector<string> nextTupIter(tupIter.size() - 1);
    for(size_t i = 1; i < tupIter.size(); i++)
    {
      nextTupIter[i - 1] = tupIter[i];
    }
    body->stmts.push_back(new For(nextTupIter, subArr, innerBody));
  }
}

For::For(Block* b, string counter, Expression* begin, Expression* end, Block* innerBody)
  : Statement(b)
{
  body = new Block(this);
  setupRange(counter, begin, end);
  body->stmts.push_back(innerBody);
  innerBody->scope->parent = body->scope;
}

void For::resolve(bool final)
{
  init->resolve(final);
  if(!init->resolved)
    return;
  resolveExpr(condition, final);
  if(!condition->resolved)
    return;
  increment->resolve(final);
  if(!increment->resolved)
    return;
  body->resolve(final);
  if(!body->resolved)
    return;
  resolved = true;
}

Variable* For::setupRange(string counterName, Expression* begin, Expression* end)
{
  //get the correct type for the counter
  Variable* counterVar = new Variable(counterName, primitives[Prim::LONG], body);
  body->scope->addName(counterVar);
  Expression* counterExpr = new VarExpr(counterVar);
  counterExpr->resolve(true);
  init = new VarAssign(counterExpr, new IntLiteral(0));
  condition = new BinaryArith(counterExpr, CMPL, end);
  increment = new VarAssign(counterExpr, new BinaryArith(counterExpr, PLUS, new IntLiteral(1)));
  return counterVar;
}

While::While(Block* b, Expression* cond, Block* whileBody)
  : Statement(b)
{
  condition = cond;
  body = whileBody;
}

void While::resolve(bool final)
{
  resolveExpr(condition, final);
  if(!condition->resolved)
    return;
  if(condition->type != primitives[Prim::BOOL])
  {
    errMsgLoc(condition, "while loop condition must be bool");
  }
  body->resolve(final);
  if(!body->resolved)
    return;
  resolved = true;
}

If::If(Block* b, Expression* cond, Statement* b)
  : Statement(b)
{
  condition = cond;
  body = b;
}

If::If(Block* b, Expression* cond, Statement* tb, Statement* fb)
  : Statement(b)
{
  condition = cond;
  body = tb;
  elseBody = fb;
}

void If::resolve(bool final)
{
  resolveExpr(condition, final);
  if(!condition->resolved)
    return;
  body->resolve(final);
  if(!body->resolved)
    return;
  if(elseBody)
  {
    elseBody->resolve(final);
    if(!elseBody->resolved)
      return;
  }
  resolved = true;
}

Match::Match(Block* b, Expression* m, string varName,
    vector<TypeSystem::Type*>& t,
    vector<Block*>& caseBlocks)
  : Statement(b)
{
  matched = m;
  types = t;
  blocks = caseBlocks;
  //create blocks to enclose each case block, and
  //add the value variables to each
  if(types.size() != blocks.size())
  {
    INTERNAL_ERROR;
  }
  int n = types.size();
  caseVars.resize(n);
  for(int i = 0; i < n; i++)
  {
    Block* outerBlock = new Block(b);
    outerBlock->setLocation(blocks[i]);
    caseVars[i] = new Variable(varName, types[i], outerBlock);
    outerBlock->scope->addName(caseVars[i]);
    outerBlock->stmts.push_back(outerBlock);
    Block* innerBlock = blocks[i];
    innerBlock->scope->node = outerBlock;
    blocks[i] = outerBlock;
  }
}

void Match::resolve(bool final)
{
  resolveExpr(matched, final);
  if(!matched->resolved)
    return;
  auto ut = dynamic_cast<UnionType*>(matched->type);
  if(!ut)
  {
    errMsgLoc(matched, "matched expression must be of union type");
  }
  for(auto& t : types)
  {
    resolveType(t, final);
    if(!t->resolved)
      return;
  }
  for(auto t : types)
  {
    if(find(ut->options.begin(), ut->options.end(), t) == ut->options.end())
    {
      errMsgLoc(this, "match includes type " << t->getName() << " which is not a member of union " << ut->getName());
    }
  }
  bool allResolved = true;
  for(auto b : blocks)
  {
    b->resolve(final);
    if(!b->resolved)
      allResolved = false;
  }
  if(allResolved)
    resolved = true;
}

Switch::Switch(Block* b, Expression* s, vector<int>& inds, vector<Expression*> vals, vector<Statement*>& stmtList, int defaultPos)
  : Statement(b)
{
  switched = s;
  caseValues = vals;
  caseLabels = inds;
  defaultPosition = defaultPos;
  stmts = stmtList;
}

void Switch::resolve(bool final)
{
  resolveExpr(switched, final);
  if(!switched->resolve)
    return;
  //resolve case values and make sure they can convert to 
  bool allResolved = true;
  for(auto& caseVal : caseValues)
  {
    resolveExpr(caseVal, final);
    if(!caseVal->resolved)
    {
      allResolved = false;
    }
    else
    {
      if(!switched->type->canConvert(caseVal->type))
      {
        errMsgLoc(caseVal, "case value type incompatible with switch value type");
      }
      else if(switched->type != caseVal->type)
      {
        caseVal = new Converted(caseVal, switched->type);
      }
    }
  }
  //resolve all the statements
  for(auto& stmt : stmts)
  {
    stmt->resolve(final);
    if(!stmt->resolved)
      allResolved = false;
  }
  if(allResolved)
    resolved = true;
}

Return::Return(Block* b, Expression* e) : Statement(b)
{
  value = e;
}

Return(Block* b) : Statement(b)
{
  value = nullptr;
}

void Return::resolve(bool final)
{
  if(value)
  {
    value->resolve(final);
    if(!value->resolved)
      return;
  }
  //make sure value can be converted to enclosing subroutine's return type
  auto subrRetType = block->subr->type->retType;
  if(subrRetType == primitives[Prim::VOID])
  {
    if(value)
    {
      errMsgLoc(this, "returned a value from void subroutine");
    }
  }
  if(!subrRetType->canConvert(value->type))
  {
    errMsgLoc(this, "returned value of type " << value->type->getName() << " incompatible with subroutine return type " << subrRetType->getName());
  }
  else if(subrRetType != value->type)
  {
    value = new Converted(value, subrRetType);
  }
  resolved = true;
}

Break::Break(Block* b) : Statement(b)
{}

void Break::resolve(bool final)
{
  if(block->breakable.is<None>())
  {
    errMsgLoc(this, "break is not inside any loop or switch");
  }
  breakable = block->breakable;
}

Continue::Continue(Block* b) : Statement(b)
{}

void Continue::resolve(bool final)
{
  if(block->loop.is<None>())
  {
    errMsgLoc(this, "continue is not inside any loop");
  }
  loop = block->loop;
}

Print::Print(Block* b, vector<Expression*>& e) : Statement(b)
{
  exprs = e;
}

void Print::resolve(bool final)
{
  for(auto& e : exprs)
  {
    resolveExpr(e, final);
    if(!e->resolved)
    {
      return;
    }
  }
  resolved = true;
}

Assertion::Assertion(Block* b, Expression* a) : Statement(b)
{
  asserted = a;
}

void Assertion::resolve(bool final)
{
  resolveExpr(asserted, final);
  if(!asserted->resolved)
  {
    return;
  }
  if(asserted->type != primitives[Prim::BOOL])
  {
    errMsgLoc("asserted value has non-bool type " << asserted->type->getName());
  }
  resolved = true;
}

Subroutine::Subroutine(Scope* s, string n, TypeSystem::CallableType* ct, vector<string>& argNames, vector<TypeSystem::Type*>& argTypes, Block* bodyBlock)
{
  scope = s;
  name = n;
}

void Subroutine::resolve(bool final)
{
}

void Subroutine::check()
{
  //Need special checks for main
  //ret type can be void or int
  //args are either string[] or nothing
  auto voidType = primitives[Parser::TypeNT::VOID];
  if(name == "main")
  {
    if(type->pure)
    {
      ERR_MSG("main() must be a procedure");
    }
    if(scope->parent != global)
    {
      ERR_MSG("main() is not in global scope");
    }
    programHasMain = true;
    if(type->returnType != voidType &&
        type->returnType != primitives[Parser::TypeNT::INT])
    {
      ERR_MSG("proc main must return void or int");
    }
    bool noArgs = type->argTypes.size() == 0;
    bool takesStringArray = type->argTypes.size() == 1 &&
      type->argTypes[0] == getArrayType(primitives[Parser::TypeNT::CHAR], 2);
    if(!noArgs && !takesStringArray)
    {
      ERR_MSG("proc main must take no arguments or only an array of strings");
    }
  }
  body->check();
  //after checking body, check if it ends in a return
  //if return type is void and there is no return, add it explicitly
  //TODO: check for "missing return" when CFG is supported
  if(type->returnType == voidType &&
      (body->stmts.size() == 0 || !dynamic_cast<Return&*>(body->stmts.back())))
  {
    body->stmts.push_back(new Return(this));
  }
}

ExternalSubroutine::ExternalSubroutine(Parser::ExternSubroutineNT* es, Scope* s)
{
  TypeSystem::typeLookup->lookup(es->type, (Type*&) type);
  c = es->c;
}

Test::Test(Parser::TestDecl* td, Scope* s)
{
  tests.push_back(this);
  //Create a dummy block
  //to hold the statement
  BlockScope* bs = blockScopes[td->block];
  run = new Block(bs);
  run->addStatements(td->block);
}

