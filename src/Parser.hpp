#ifndef PARSER_H
#define PARSER_H

#include "Common.hpp"
#include "AST.hpp"
#include "Token.hpp"
#include "Scope.hpp"

struct Statement;
struct Expression;
struct Block;
struct Assign;
struct ForC;
struct ForRange;
struct ForArray;
struct If;
struct While;
struct Switch;
struct Match;
struct Subroutine;
struct ExternalSubroutine;
struct Type;
struct StructType;
struct UnresolvedType;

void parseProgram(SourceFile* sf);
//Parse program from stdin
void parseProgram();
//Parse the whole program into the global AST
void parseProgram(string mainSourcePath);

namespace Parser
{
  struct Stream
  {
    Stream(SourceFile* file);
    Stream(const Stream& s) = delete;
    size_t pos;
    vector<Token*>* tokens;
    bool emitErrors;
    Stream& operator=(const Stream& other);
    bool operator==(const Stream& s);
    bool operator!=(const Stream& s);
    bool operator<(const Stream& s);

    void accept();                //accept (and discard) any token
    bool accept(Token& t);
    Token* accept(TokenTypeEnum tokType);   //return NULL if tokType doesn't match next
    bool acceptKeyword(KeywordEnum type);
    bool acceptOper(OperatorEnum type);
    bool acceptPunct(PunctEnum type);
    void expect(Token& t);
    Token* expect(TokenTypeEnum tokType);
    void expectKeyword(KeywordEnum type);
    void expectOper(OperatorEnum type);
    void expectPunct(PunctEnum type);
    string expectIdent();
    Token* lookAhead(int n = 0);  //get the next token without advancing pos
    void err(string msg = "");

    Statement* parseDecl(Scope* s, bool semicolon);
    //parse a statement, but don't add it to block
    Statement* parseStatement(Block* b, bool semicolon);
    //parse a statement or declaration
    //if statement, return it but don't add to block
    //if decl, add it to block's scope
    Statement* parseStatementOrDecl(Block* b, bool semicolon);
    If* parseIf(Block* b);
    While* parseWhile(Block* b);
    //parse a variable declaration, and add the variable to scope
    //if s belongs to a block and the variable is initialized, return the assignment
    Assign* parseVarDecl(Scope* s);
    Expression* parseExpression(Scope* s, int prec = 0);
    Expression* parseLambdaExpr(Scope* s);
    void parseSubroutineDecl(Scope* s);
    void parseSubroutine(SubroutineDecl* sd);
    void parseExternalSubroutine(SubroutineDecl* sd);
    void parseModule(Scope* s);
    void parseStruct(Scope* s);
    void parseAlias(Scope* s);
    void parseSimpleType(Scope* s);
    void parseEnum(Scope* s);
    void parseUsing(Scope* s);
    ForC* parseForC(Block* b);
    ForArray* parseForArray(Block* b);
    ForRange* parseForRange(Block* b);
    Switch* parseSwitch(Block* b);
    Match* parseMatch(Block* b);
    Member* parseMember();
    //Parse a block (which has already been constructed)
    void parseBlock(Block* b);
    void parseTest(Scope* s);
    Type* parseType(Scope* s);
    void parseLambdaType(UnresolvedType* ut);

    //Metaprogramming
    //Starting at offset from current stream position,
    //parse and substitute #<stmt>
    void processMetaStmt(int offset);
  };
}

//Utils
ostream& operator<<(ostream& os, const Member& mem);

#endif

