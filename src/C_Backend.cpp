#include "C_Backend.hpp"

using namespace TypeSystem;

unordered_map<Type*, string> types;
unordered_map<Type*, bool> typesImplemented;
unordered_map<Subroutine*, string> subrs;
unordered_map<Variable*, string> vars;
unordered_map<Type*, string> printFuncs;
size_t identCount;
ofstream c;
Oss typeDecls;
Oss varDecls;
Oss printFuncDecls;
Oss printFuncDefs;
Oss funcDecls;
Oss funcDefs;

namespace C
{
  void generate(string outputStem, bool keep)
  {
    string cName = outputStem + ".c";
    string exeName = outputStem + ".exe";
    typeDecls = Oss();
    varDecls = Oss();
    printFuncDecls = Oss();
    printFuncDefs = Oss();
    funcDecls = Oss();
    funcDefs = Oss();
    genTypeDecls();
    genGlobals();
    genSubroutines();
    c = ofstream(cName);
    c << "//--- " << outputStem << ".c, generated by the Onyx Compiler ---//\n\n";
    //genCommon writes directly to the c stream
    genCommon();
    //write types, vars, func decls, func defs in the ostringstreams
    c.write(typeDecls.str().c_str(), typeDecls.tellp());
    c.write(varDecls.str().c_str(), varDecls.tellp());
    c.write(printFuncDecls.str().c_str(), printFuncDecls.tellp());
    c.write(printFuncDefs.str().c_str(), printFuncDefs.tellp());
    c.write(funcDecls.str().c_str(), funcDecls.tellp());
    c.write(funcDefs.str().c_str(), funcDefs.tellp());
    c << '\n';
    c.close();
    //wait for cc to terminate
    bool compileSuccess = runCommand(string("gcc") + " --std=c99 -ffast-math -fassociative-math -o " + exeName + ' ' + cName + " &> /dev/null");
    if(!keep)
    {
      remove(cName.c_str());
    }
    if(!compileSuccess)
    {
      ERR_MSG("C compiler encountered error.");
    }
  }

  void genCommon()
  {
    c << "#include \"stdio.h\"\n";
    c << "#include \"stdlib.h\"\n";
    c << "#include \"math.h\"\n";
    c << "#include \"string.h\"\n";
    c << "#include \"stdint.h\"\n";
    c << "#include \"stdbool.h\"\n";
    c << '\n';
  }

  void genTypeDecls()
  {
    //A list of all singular types (every type except arrays)
    vector<Type*> allTypes;
    for(auto tt : TypeSystem::tuples)
    {
      allTypes.push_back(tt);
    }
    for(auto prim : TypeSystem::primitives)
    {
      allTypes.push_back(prim);
    }
    walkScopeTree([&] (Scope* s) -> void
      {
        for(auto t : s->types)
        {
          if(!t->isAlias())
          {
            allTypes.push_back(t);
          }
        }
      });
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
    typeDecls << "typedef struct\n";
    typeDecls << "{\n";
    typeDecls << "char* data;\n";
    typeDecls << "unsigned length;\n";
    typeDecls << "} ostring;\n\n";
    //forward-declare all compound types
    for(auto t : allTypes)
    {
      if(!t->isPrimitive() && !t->isAlias())
      {
        //get an identifier for type t
        string ident = getIdentifier();
        types[t] = ident;
        typesImplemented[t] = false;
        //forward-declare the type
        typeDecls << "struct " << ident << "; //" << t->getName() << '\n';
      }
      else if(t->isPrimitive() && !)
      {
        //primitives (including aliases of primitives) are already implemented
        typesImplemented[t] = true;
      }
    }
    typeDecls << '\n';
    //implement all compound types
    for(auto t : allTypes)
    {
      if(!t->isPrimitive() && !typesImplemented[t])
      {
        generateCompoundType(typeDecls, types[t], t);
      }
    }
    typeDecls << '\n';
  }

  void genGlobals()
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
            varDecls << types[v->type] << " " << ident << ";\n";
            numGlobals = 0;
          }
        }
      });
    if(numGlobals)
    {
      varDecls << '\n';
    }
  }

  void genSubroutines()
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
          //all C functions except main are static
          if(ident != "main")
            funcDecls << "static ";
          funcDecls << types[sub->retType] << ' ' << ident << '(';
          for(auto arg : sub->argVars)
          {
            if(arg != sub->argVars.front())
            {
              funcDecls << ", ";
            }
            string argName = getIdentifier();
            vars[arg] = ident;
            funcDecls << types[arg->type] << ' ' << argName;
          }
          funcDecls << ");\n";
        }
      });
    //implement all subroutines
    walkScopeTree([&] (Scope* s) -> void
      {
        for(auto sub : s->subr)
        {
          funcDefs << types[sub->retType] << ' ' << subrs[sub] << '(';
          for(auto arg : sub->argVars)
          {
            if(arg != sub->argVars.front())
            {
              funcDefs << ", ";
            }
            string argName = getIdentifier();
            vars[arg] = argName;
            funcDefs << types[arg->type] << ' ' << argName;
          }
          funcDefs << ")\n";
          generateBlock(funcDefs, sub->body);
        }
      });
  }

  void generateExpression(ostream& c, Block* b, Expression* expr)
  {
    //Expressions in C mostly depend on the subclass of expr
    if(UnaryArith* unary = dynamic_cast<UnaryArith*>(expr))
    {
      c << operatorTable[unary->op];
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
    else if(Indexed* indexed = dynamic_cast<Indexed*>(expr))
    {
      //Indexed expression must be either a tuple or array
      auto indexedType = indexed->group->type;
      if(ArrayType* at = dynamic_cast<ArrayType*>(indexedType))
      {
        auto elementType = at->elem;
        if(ArrayType* subArray = dynamic_cast<ArrayType*>(elementType))
        {
          c << "((" << types[indexedType] << ") {";
          //add all dimensions, except highest
          for(int dim = 1; dim < at->dims; dim++)
          {
            generateExpression(c, b, indexed->group);
            c << ".dim" << dim << ", ";
          }
          //now add the data pointer from expr, but with offset
          generateExpression(c, b, indexed->group);
          c << ".data + ";
          generateExpression(c, b, indexed->index);
          //offset is produce of index and all lesser dimensions
          for(int dim = 1; dim < at->dims; dim++)
          {
            c << " * (";
            generateExpression(c, b, indexed->group);
            c << ".dim" << dim << ")";
          }
          c << "})";
        }
        else
        {
          //just index into data
          c << '(';
          generateExpression(c, b, indexed->group);
          c << ".data[";
          generateExpression(c, b, indexed->index);
          c << "])";
        }
      }
      else if(TupleType* tt = dynamic_cast<TupleType*>(indexedType))
      {
        //tuple: simply reference the requested member
        c << '(';
        generateExpression(c, b, indexed->group);
        //index must be an IntLiteral (has already been checked)
        c << ".mem" << dynamic_cast<IntLiteral*>(indexed)->value << ')';
      }
    }
    else if(CallExpr* call = dynamic_cast<CallExpr*>(expr))
    {
      c << call->subr << '(';
      for(auto arg : call->args)
      {
        generateExpression(c, b, arg);
      }
      c << ')';
    }
    else if(VarExpr* var = dynamic_cast<VarExpr*>(expr))
    {
      c << vars[var->var];
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
      c << "for(";
      generateStatement(c, b, f->init);
      generateExpression(c, b, f->condition);
      generateStatement(c, b, f->increment);
      c << ")\n";
      generateBlock(c, f->loopBlock);
    }
    else if(While* w = dynamic_cast<While*>(stmt))
    {
      c << "while(";
      generateExpression(c, b, w->condition);
      c << ")\n";
      c << "{\n";
      generateBlock(c, w->loopBlock);
      c << "}\n";
    }
    else if(If* i = dynamic_cast<If*>(stmt))
    {
      c << "if(";
      generateExpression(c, b, i->condition);
      c << ")\n";
      generateStatement(c, b, i->body);
    }
    else if(IfElse* ie = dynamic_cast<IfElse*>(stmt))
    {
      c << "if(";
      generateExpression(c, b, ie->condition);
      c << ")\n";
      generateStatement(c, b, ie->trueBody);
      c << "else\n";
      generateStatement(c, b, ie->falseBody);
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
    else if(Assertion* assertion = dynamic_cast<Assertion*>(stmt))
    {
      c << "if(";
      generateExpression(c, b, assertion->asserted);
      c << ")\n";
      c << "{\n";
      c << "puts(\"Assertion failed.\");\n";
      c << "exit(1);\n";
      c << "}\n";
    }
  }

  void generateAssignment(ostream& c, Block* b, Expression* lhs, Expression* rhs)
  {
  }

  string getPrintFunction(Type* t)
  {
    //lazily look up or create print function
    {
      auto it = printFuncs.find(t);
      if(it != printFuncs.end())
      {
        return it->second;
      }
    }
    //otherwise, add decl/def for print function and return that
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
      c << "putchar('[');\n";
      //generate nested for loops to iterate over each dimension
      for(int dim = 0; dim < arrayType->dims; dim++)
      {
        string counter = getIdentifier();
        c << "for(uint64_t " << counter << " = 0; " << counter << " < ";
        generateExpression(c, b, expr);
        c << ".dim" << dim << "; " << counter << "++)\n{\n";
        //generate print for single element
        c << "}\n";
      }
      c << "putchar(']');\n";
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

  void generateCompoundType(ostream& c, string cName, TypeSystem::Type* t)
  {
    string indexType = "uint64_t";
    auto at = dynamic_cast<ArrayType*>(t);
    auto st = dynamic_cast<StructType*>(t);
    auto ut = dynamic_cast<UnionType*>(t);
    auto tt = dynamic_cast<TupleType*>(t);
    auto et = dynamic_cast<EnumType*>(t);
    //C type to use for array types
    if(at)
    {
      if(!typesImplemented[at->elem])
      {
        generateCompoundType(c, types[at->elem], at->elem);
      }
      c << "struct " << cName << "\n{\n";
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
      //  since there is no possible name collision among the member names, don't
      //  need to replace them with mangled identifiers
      //first make sure all members are already implemented
      for(auto mem : st->members)
      {
        if(!typesImplemented[mem])
        {
          generateCompoundType(c, types[mem], mem);
        }
      }
      //then add the members to the actual struct definition
      c << "struct " << cName << "\n{\n";
      for(size_t i = 0; i < st->members.size(); i++)
      {
        c << types[st->members[i]] << ' ' << st->memberNames[i] << ";\n";
      }
    }
    else if(ut)
    {
      c << "struct " << cName << "\n{\n";
      c << "void* data;\n";
      c << "int option;\n";
    }
    else if(tt)
    {
      for(auto mem : tt->members)
      {
        if(!typesImplemented[mem])
        {
          generateCompoundType(c, types[mem], mem);
        }
      }
      c << "struct " << cName << "\n{\n";
      for(size_t i = 0; i < tt->members.size(); i++)
      {
        //tuple members are anonymous so just use memN as the name
        c << types[tt->members[i]] << " mem" << i << ";\n";
      }
    }
    c << "};\n";
    typesImplemented[t] = true;
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

