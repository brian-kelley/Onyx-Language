#ifndef IR_H
#define IR_H

#include "Common.hpp"
#include "Subroutine.hpp"

/* Lower-level IR
 *
 * Is much higher level than 3-address code
 * (operands are Onyx expressions, not primitive values)
 *
 * Should be easy to do basic optimizations and natural to emit as C, LLVM or x86
 */

struct Subroutine;
struct Expression;


namespace IR
{
  struct StatementIR;
  struct AssignIR;
  struct CallIR;
  struct Jump;
  struct CondJump;
  struct Label;
  struct ReturnIR;
  struct PrintIR;
  struct AssertionIR;
  struct BasicBlock;
  struct SubroutineIR;

  //IR value types (expressions)
  //
  //Associative arithmetic operators on > 2 operands
  //are combined into one "operation"
  //
  //All values are given unique IDs
  //
  //Expressions can quicky be tested for symbolic equivalence for CSE
  //
  //Need a multi-pass approach to create minimal set of values in subroutine
  //
  //  1. Create an initial set of values, using them as arguments to IR instructions
  //    1a. Variables before and after any assignment to them are different Values
  //  2. For each BB with multiple incoming CFG edges, add the necessary phi nodes
  //    2a. phi nodes don't need corresponding Assign instructions, just keep a separate table of them
  //    2b. this creates valid (but not optimal, yet) SSA
  //  3. Until no more updates can be done:
  //    3a. Test if all joined values in a phi node are actually equivalent
  //        (can safely delete those nodes, and replace their usage by any of the incoming versions)
  //    3b. Find constant subexpressions (and evaluate them, except when it would
  //        create a constant bigger than an arbitrary cutoff, like 256 bytes)

  extern map<Subroutine*, SubroutineIR*> ir;
  //construct all (un-optimized) IR and CFGs from AST
  void buildIR();
  void optimizeIR();

  struct StatementIR
  {
    //data input/output (for data dependency analysis)
    virtual vector<Expression*> getInput()
    {
      return vector<Expression*>();
    }
    virtual vector<Expression*> getOutput()
    {
      return vector<Expression*>();
    }
    virtual ~StatementIR(){}
    //integer position in subroutine
    int intLabel;
  };

  struct AssignIR : public StatementIR
  {
    AssignIR(Expression* d, Expression* s) : dst(d), src(s) {}
    Expression* dst;
    Expression* src;
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, src);
    }
    vector<Expression*> getOutput()
    {
      return vector<Expression*>(1, dst);
    }
  };

  struct CallIR : public StatementIR
  {
    CallIR(CallStmt* cs) : eval(cs->eval) {}
    //currently this is only used by CallStmts
    CallExpr* eval;
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, eval);
    }
  };

  struct Jump : public StatementIR
  {
    Jump(Label* l) : dst(l) {}
    Label* dst;
  };

  struct CondJump : public StatementIR
  {
    CondJump(Expression* c, Label* dst) : cond(c), taken(dst) {}
    //the boolean condition
    //false = jump taken, true = not taken (fall through)
    Expression* cond;
    Label* taken;
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, cond);
    }
  };

  struct Label : public StatementIR {};

  struct ReturnIR : public StatementIR
  {
    ReturnIR() : expr(nullptr) {}
    ReturnIR(Expression* val) : expr(val) {}
    Expression* expr;
    vector<Expression*> getInput()
    {
      if(expr)
        return vector<Expression*>(1, expr);
      else
        return vector<Expression*>();
    }
  };

  struct PrintIR : public StatementIR
  {
    PrintIR(Expression* e) : expr(e) {}
    Expression* expr;
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, expr);
    }
  };

  struct AssertionIR : public StatementIR
  {
    AssertionIR(Assertion* a) : asserted(a->asserted) {}
    Expression* asserted;
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, asserted);
    }
  };

  struct Nop : public StatementIR {};

  struct BasicBlock
  {
    BasicBlock(int s, int e) : start(s), end(e) {}
    //add an outgoing edge, and add this as an incoming
    void link(BasicBlock* other)
    {
      out.push_back(other);
      other->in.push_back(this);
    }
    vector<BasicBlock*> in;
    vector<BasicBlock*> out;
    //statements where BB starts and ends
    int start;
    int end;
    //which BB is this in linear code sequence?
    int index;
  };

  struct SubroutineIR
  {
    SubroutineIR(Subroutine* s);
    void addStatement(Statement* s);
    void addInstruction(StatementIR* s);
    //Create a new variable that is tied to non-constant expression
    //Generate IR instructions to compute it (in proper evaluation order)
    Expression* expandExpression(Expression* e);
    //Remove no-ops and labels, and rebuild the control flow graph
    //(labels are just a convenience for translating AST to IR)
    void buildCFG();
    set<Variable*> getReads(BasicBlock* bb);
    set<Variable*> getWrites(BasicBlock* bb);
    Subroutine* subr;
    vector<StatementIR*> stmts;
    vector<BasicBlock*> blocks;
    map<int, BasicBlock*> blockStarts;
    //is one BB reachable from another?
    bool reachable(BasicBlock* root, BasicBlock* target);
    string getTempName();
    private:
    void addForC(ForC* fc);
    void addForRange(ForRange* fr);
    void addForArray(ForArray* fa);
    void addSwitch(Switch* sw);
    void addMatch(Match* ma);
    int tempCounter;
  };
}

extern IR::Nop* nop;

ostream& operator<<(ostream& os, IR::StatementIR* stmt);

#endif

