#include "C_Backend.hpp"

using namespace TypeSystem;

unordered_map<Type*, string> types;
unordered_map<Subroutine*, string> subrs;
unordered_map<Variable*, string> vars;
size_t identCount;

namespace C
{
  void generate(string outputStem, bool keep)
  {
    string cName = outputStem + ".c";
    string exeName = outputStem + ".exe";
    ofstream c(cName);
    c << "//--- " << outputStem << ".c, generated by the Onyx Compiler ---//\n\n";
    genCommon(c);
    genTypeDecls(c);
    genGlobals(c);
    genSubroutines(c);
    c << '\n';
    c.close();
    string cmd = string("gcc") + " --std=c99 -Os -o " + exeName + ' ' + cName + " &> /dev/null";
    //wait for cc to terminate
    int exitStatus = system(cmd.c_str());
    if(!keep)
    {
      remove(cName.c_str());
    }
    if(exitStatus)
    {
      ERR_MSG("C compiler encountered error.");
    }
  }

  void genCommon(ostream& c)
  {
    c << "#include \"stdio.h\"\n";
    c << "#include \"stdlib.h\"\n";
    c << "#include \"math.h\"\n";
    c << "#include \"string.h\"\n";
    c << "#include \"stdint.h\"\n";
    c << "#include \"stdbool.h\"\n";
    c << '\n';
  }

  void genTypeDecls(ostream& c)
  {
    //primitives (string is a struct, all others are C primitives)
    types[TypeSystem::primNames["void"]] = "void";
    types[TypeSystem::primNames["bool"]] = "bool";
    types[TypeSystem::primNames["char"]] = "int8_t";
    types[TypeSystem::primNames["uchar"]] = "uint8_t";
    types[TypeSystem::primNames["short"]] = "int16_t";
    types[TypeSystem::primNames["ushort"]] = "uint16_t";
    types[TypeSystem::primNames["int"]] = "int32_t";
    types[TypeSystem::primNames["uint"]] = "uint32_t";
    types[TypeSystem::primNames["long"]] = "int64_t";
    types[TypeSystem::primNames["ulong"]] = "uint64_t";
    types[TypeSystem::primNames["float"]] = "float";
    types[TypeSystem::primNames["double"]] = "double";
    types[TypeSystem::primNames["string"]] = "ostring";
    c << "typedef struct\n";
    c << "{\n";
    c << "char* data;\n";
    c << "unsigned length;\n";
    c << "} ostring;\n\n";
    int numTypes = 0;
    walkScopeTree([&] (Scope* s) -> void
      {
        for(auto t : s->types)
        {
          if(!t->isPrimitive())
          {
            //get an identifier for type t
            string ident = getIdentifier();
            types[t] = ident;
            //forward-declare the type
            c << "struct " << ident << ";\n";
            numTypes++;
          }
          for(auto arrType : t->dimTypes)
          {
            string ident = getIdentifier();
            types[t] = ident;
            c << "struct " << ident << ";\n";
          }
        }
      });
    if(numTypes)
    {
      walkScopeTree([&] (Scope* s) -> void
        {
          for(auto t : s->types)
          {
            if(!t->isPrimitive())
            {
              generateCompoundType(c, types[t], t);
            }
            for(auto arrType : t->dimTypes)
            {
              generateCompoundType(c, types[arrType], arrType);
            }
          }
        });
      c << '\n';
    }
  }

  void genGlobals(ostream& c)
  {
    int numGlobals = 0;
    walkScopeTree([&] (Scope* s) -> void
      {
        //only care about vars in module scope
        if(dynamic_cast<ModuleScope*>(s))
        {
          for(auto v : s->vars)
          {
            string ident = getIdentifier();
            vars[v] = ident;
            c << types[v->type] << " " << ident << ";\n";
            numGlobals = 0;
          }
        }
      });
    if(numGlobals)
    {
      c << '\n';
    }
  }

  void genSubroutines(ostream& c)
  {
    //forward-declare all subroutines
    walkScopeTree([&] (Scope* s) -> void
      {
        for(auto sub : s->subr)
        {
          string ident;
          //main() is the only subroutine with a special name
          if(sub->name == "main")
            ident = "main";
          else
            ident = getIdentifier();
          subrs[sub] = ident;
          c << types[sub->retType] << ' ' << ident << '(';
          for(auto arg : sub->argVars)
          {
            if(arg != sub->argVars.front())
            {
              c << ", ";
            }
            string argName = getIdentifier();
            vars[arg] = ident;
            c << types[arg->type] << ' ' << argName;
          }
          c << ");\n";
        }
      });
    //implement all subroutines
    walkScopeTree([&] (Scope* s) -> void
      {
        for(auto sub : s->subr)
        {
          c << types[sub->retType] << ' ' << subrs[sub] << '(';
          for(auto arg : sub->argVars)
          {
            if(arg != sub->argVars.front())
            {
              c << ", ";
            }
            string argName = getIdentifier();
            vars[arg] = argName;
            c << types[arg->type] << ' ' << argName;
          }
          c << ")\n";
          generateBlock(c, sub->body);
        }
      });
  }

  void generateExpression(ostream& c, Block* b, Expression* expr)
  {
    //Expressions in C mostly depend on the subclass of expr
    if(UnaryArith* unary = dynamic_cast<UnaryArith*>(expr))
    {
      switch(unary->op)
      {
        case LNOT:
          c << '!';
          break;
        case BNOT:
          c << '~';
          break;
        case SUB:
          c << '-';
          break;
        default:;
      }
      c << '(';
      generateExpression(c, b, unary->expr);
      c << ')';
    }
    else if(BinaryArith* binary = dynamic_cast<BinaryArith*>(expr))
    {
      //emit this so that it works even if onyx uses different
      //operator precedence than C
      c << "((";
      generateExpression(c, b, binary->lhs);
      c << ')';
      c << operatorTable[binary->op];
      c << '(';
      generateExpression(c, b, binary->rhs);
      c << "))";
    }
    else if(IntLiteral* intLit = dynamic_cast<IntLiteral*>(expr))
    {
      //all int literals are unsigned
      c << intLit->value << "U";
      //add "long long" suffix if necessary
      if(intLit->value >= 0x7FFFFFFF)
      {
        c << "LL";
      }
    }
    else if(FloatLiteral* floatLit = dynamic_cast<FloatLiteral*>(expr))
    {
      //all float lits have type double, so never use "f" suffix
      c << floatLit->value;
    }
    else if(StringLiteral* stringLit = dynamic_cast<StringLiteral*>(expr))
    {
      //generate an ostring struct using C struct literal
      c << "((ostring) {\"" << stringLit->value << "\", ";
      c << stringLit->value.length() << "})";
    }
    else if(CharLiteral* charLit = dynamic_cast<CharLiteral*>(expr))
    {
      c << '\'' << charLit->value << '\'';
    }
    else if(BoolLiteral* boolLit = dynamic_cast<BoolLiteral*>(expr))
    {
      if(boolLit->value)
        c << "true";
      else
        c << "false";
    }
    else if(CompoundLiteral* compLit = dynamic_cast<CompoundLiteral*>(expr))
    {
    }
    else if(TupleLiteral* tupLit = dynamic_cast<TupleLiteral*>(expr))
    {
    }
    else if(Indexed* index = dynamic_cast<Indexed*>(expr))
    {
    }
    else if(CallExpr* call = dynamic_cast<CallExpr*>(expr))
    {
    }
    else if(VarExpr* var = dynamic_cast<VarExpr*>(expr))
    {
    }
  }

  void generateBlock(ostream& c, Block* b)
  {
    c << "{\n";
    //introduce local variables
    for(auto local : b->scope->vars)
    {
      string localIdent = getIdentifier();
      vars[local] = localIdent;
      c << types[local->type] << ' ' << localIdent << ";\n";
    }
    for(auto blockStmt : b->stmts)
    {
      generateStatement(c, b, blockStmt);
    }
    c << "}\n";
  }

  void generateStatement(ostream& c, Block* b, Statement* stmt)
  {
    //get the type of statement
    if(Block* blk = dynamic_cast<Block*>(stmt))
    {
      generateBlock(c, blk);
    }
    else if(Assign* a = dynamic_cast<Assign*>(stmt))
    {
      generateAssignment(c, b, a->lvalue, a->rvalue);
    }
    else if(CallStmt* cs = dynamic_cast<CallStmt*>(stmt))
    {
      c << subrs[cs->called] << '(';
      for(size_t i = 0; i < cs->args.size(); i++)
      {
        if(i > 0)
        {
          c << ", ";
        }
        generateExpression(c, b, cs->args[i]);
      }
      c << ");\n";
    }
    else if(For* f = dynamic_cast<For*>(stmt))
    {
    }
    else if(While* w = dynamic_cast<While*>(stmt))
    {
    }
    else if(If* i = dynamic_cast<If*>(stmt))
    {
    }
    else if(IfElse* ie = dynamic_cast<IfElse*>(stmt))
    {
    }
    else if(Return* r = dynamic_cast<Return*>(stmt))
    {
      if(r->value)
      {
        c << "return ";
        generateExpression(c, b, r->value);
        c << ";\n";
      }
      else
      {
        c << "return;\n";
      }
    }
    else if(dynamic_cast<Break*>(stmt))
    {
      c << "break;\n";
    }
    else if(dynamic_cast<Continue*>(stmt))
    {
      c << "continue;\n";
    }
    else if(Print* p = dynamic_cast<Print*>(stmt))
    {
      //emit printf calls for each expression
      for(auto expr : p->exprs)
      {
        generatePrint(c, b, expr);
      }
    }
    else if(Assertion* assert = dynamic_cast<Assertion*>(stmt))
    {
    }
  }

  void generateAssignment(ostream& c, Block* b,
                          Expression* lhs, Expression* rhs)
  {
  }

  void generatePrint(ostream& c, Block* b, Expression* expr)
  {
    auto type = expr->type;
    if(IntegerType* intType = dynamic_cast<IntegerType*>(type))
    {
      //printf format code
      string fmt;
      bool isChar = false;
      switch(intType->size)
      {
        case 1:
          fmt = intType->isSigned ? "c" : "hhu";
          isChar = intType->isSigned;
          break;
        case 2:
          fmt = intType->isSigned ? "hd" : "hu";
          break;
        case 4:
          fmt = intType->isSigned ? "d" : "u";
          break;
        case 8:
          fmt = intType->isSigned ? "lld" : "llu";
          break;
        default:
          INTERNAL_ERROR;
      }
      c << "printf(\"%" << fmt << "\", ";
      auto cl = dynamic_cast<CharLiteral*>(expr);
      if(isChar && cl)
        generateCharLiteral(c, cl->value);
      else
        generateExpression(c, b, expr);
      c << ");\n";
    }
    else if(FloatType* floatType = dynamic_cast<FloatType*>(type))
    {
      //note: same printf code %f used for both float and double
      c << "printf(\"%f\", ";
      generateExpression(c, b, expr);
      c << ");\n";
    }
    else if(dynamic_cast<VoidType*>(type))
    {
      c << "printf(\"void\");\n";
    }
    else if(dynamic_cast<BoolType*>(type))
    {
      c << "if(";
      generateExpression(c, b, expr);
      c << ")\n";
      c << "printf(\"true\");\n";
      c << "else\n";
      c << "printf(\"false\");\n";
    }
    else if(dynamic_cast<StringType*>(type))
    {
      c << "printf(";
      generateExpression(c, b, expr);
      c << ".data);\n";
    }
    else if(StructType* structType = dynamic_cast<StructType*>(type))
    {
      c << "printf(\"" << structType->name << "{\");\n";
      //print each member, comma separated
      c << "putchar('}');\n";
    }
    else if(UnionType* unionType = dynamic_cast<UnionType*>(type))
    {
    }
    else if(TupleType* tupleType = dynamic_cast<TupleType*>(type))
    {
    }
    else if(ArrayType* arrayType = dynamic_cast<ArrayType*>(type))
    {
    }
  }

  string getIdentifier()
  {
    //use a base-36 encoding of identCount: 0-9 A-Z
    char buf[32];
    buf[31] = 0;
    auto val = identCount;
    int iter = 30;
    for(;; iter--)
    {
      int digit = val % 36;
      if(digit < 10)
      {
        buf[iter] = '0' + digit;
      }
      else
      {
        buf[iter] = 'A' + (digit - 10);
      }
      val /= 36;
      if(val == 0)
        break;
    }
    //now buf + iter is the string
    identCount++;
    return string("o") + (buf + iter);
  }

  template<typename F>
  void walkScopeTree(F f)
  {
    vector<Scope*> visit;
    visit.push_back(global);
    while(visit.size())
    {
      Scope* s = visit.back();
      f(s);
      visit.pop_back();
      for(auto child : s->children)
      {
        visit.push_back(child);
      }
    }
  }

  void generateCompoundType(ostream& c, string cName, Type* t)
  {
    auto at = dynamic_cast<ArrayType*>(t);
    auto st = dynamic_cast<StructType*>(t);
    auto ut = dynamic_cast<UnionType*>(t);
    auto tt = dynamic_cast<TupleType*>(t);
    c << "struct " << cName << "\n{\n";
    //C type to use for array types
    string indexType = "uint64_t";
    if(at)
    {
      //add dims
      for(int dim = 0; dim < at->dims; dim++)
      {
        c << indexType << " dim" << dim << ";\n";
      }
      //add pointer to element type
      c << types[at->elem] << "* data;\n";
    }
    else if(st)
    {
      //add all members (as pointer)
      //since there is no possible name collision among the member names, don't
      //replace them with mangled identifiers
      cout << "C name for struct " << st->name << " is " << cName << '\n';
      for(size_t i = 0; i < st->members.size(); i++)
      {
        cout << "C name for member type " << i << " is " << types[st->members[i]] << '\n';
        c << types[st->members[i]] << "* " << st->memberNames[i] << ";\n";
      }
    }
    else if(ut)
    {
      c << "void* data;\n";
      c << "int option;\n";
    }
    else if(tt)
    {
      for(size_t i = 0; i < tt->members.size(); i++)
      {
        //tuple members are anonymous so just use memN as the name
        c << types[tt->members[i]] << "* mem" << i << ";\n";
      }
    }
    c << "};\n";
  }

  void generateCharLiteral(ostream& c, char character)
  {
    c << '\'';
    switch(character)
    {
      case 0:
        c << "\\0";
        break;
      case '\n':
        c << "\\n";
        break;
      case '\t':
        c << "\\t";
        break;
      case '\r':
        c << "\\r";
        break;
      default:
        c << character;
    }
    c << '\'';
  }
}

