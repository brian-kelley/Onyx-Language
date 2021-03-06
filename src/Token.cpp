#include "Token.hpp"

PastEOF PastEOF::inst;

map<string, KeywordEnum> keywordMap;
vector<string> keywordTable;
map<string, OperatorEnum> operatorMap;
vector<int> operatorPrec;
vector<string> operatorTable;
map<char, PunctEnum> punctMap;
vector<char> punctTable;
//enum values => string
vector<string> tokTypeTable;
vector<bool> operCommutativeTable;

void initTokens()
{
#define SET_KEY(str, val) keywordMap[str] = val;
  SET_KEY("void", VOID)
  SET_KEY("error", ERROR)
  SET_KEY("bool", BOOL)
  SET_KEY("char", CHAR)
  SET_KEY("byte", BYTE)
  SET_KEY("ubyte", UBYTE)
  SET_KEY("short", SHORT)
  SET_KEY("ushort", USHORT)
  SET_KEY("int", INT)
  SET_KEY("uint", UINT)
  SET_KEY("long", LONG)
  SET_KEY("ulong", ULONG)
  SET_KEY("float", FLOAT)
  SET_KEY("double", DOUBLE)
  SET_KEY("print", PRINT)
  SET_KEY("return", RETURN)
  SET_KEY("typedef", TYPEDEF)
  SET_KEY("struct", STRUCT)
  SET_KEY("this", THIS)
  SET_KEY("func", FUNC)
  SET_KEY("proc", PROC)
  SET_KEY("if", IF)
  SET_KEY("else", ELSE)
  SET_KEY("for", FOR)
  SET_KEY("while", WHILE)
  SET_KEY("switch", SWITCH)
  SET_KEY("match", MATCH)
  SET_KEY("case", CASE)
  SET_KEY("default", DEFAULT)
  SET_KEY("break", BREAK)
  SET_KEY("continue", CONTINUE)
  SET_KEY("auto", AUTO)
  SET_KEY("module", MODULE)
  SET_KEY("enum", ENUM)
  SET_KEY("type", TYPE)
  SET_KEY("using", USING)
  SET_KEY("true", TRUE)
  SET_KEY("false", FALSE)
  SET_KEY("is", IS)
  SET_KEY("as", AS)
  SET_KEY("assert", ASSERT)
  SET_KEY("test", TEST)
  SET_KEY("benchmark", BENCHMARK)
  SET_KEY("functype", FUNCTYPE)
  SET_KEY("proctype", PROCTYPE)
  SET_KEY("static", STATIC);
  SET_KEY("array", ARRAY);
  SET_KEY("extern", EXTERN);
  SET_KEY("const", CONST);
  keywordTable.resize(keywordMap.size());
  for(auto& it : keywordMap)
  {
    keywordTable[it.second] = it.first;
  }
#undef SET_KEY
  tokTypeTable.resize(NUM_TOKEN_TYPES);
  tokTypeTable[IDENTIFIER] = "identifier";
  tokTypeTable[STRING_LITERAL] = "string-literal";
  tokTypeTable[CHAR_LITERAL] = "char-literal";
  tokTypeTable[INT_LITERAL] = "int-literal";
  tokTypeTable[FLOAT_LITERAL] = "float-literal";
  tokTypeTable[PUNCTUATION] = "punctuation";
  tokTypeTable[OPERATOR] = "operator";
  tokTypeTable[KEYWORD] = "keyword";
  tokTypeTable[PAST_EOF] = "null-token";
  operatorMap["+"] = PLUS;
  operatorMap["+="] = PLUSEQ;
  operatorMap["-"] = SUB;
  operatorMap["-="] = SUBEQ;
  operatorMap["*"] = MUL;
  operatorMap["*="] = MULEQ;
  operatorMap["/"] = DIV;
  operatorMap["/="] = DIVEQ;
  operatorMap["%"] = MOD;
  operatorMap["%="] = MODEQ;
  operatorMap["||"] = LOR;
  operatorMap["|"] = BOR;
  operatorMap["|="] = BOREQ;
  operatorMap["^"] = BXOR;
  operatorMap["^="] = BXOREQ;
  operatorMap["!"] = LNOT;
  operatorMap["~"] = BNOT;
  operatorMap["&&"] = LAND;
  operatorMap["&"] = BAND;
  operatorMap["&="] = BANDEQ;
  operatorMap["<<"] = SHL;
  operatorMap["<<="] = SHLEQ;
  operatorMap[">>"] = SHR;
  operatorMap[">>="] = SHREQ;
  operatorMap["=="] = CMPEQ;
  operatorMap["!="] = CMPNEQ;
  operatorMap["<"] = CMPL;
  operatorMap["<="] = CMPLE;
  operatorMap[">"] = CMPG;
  operatorMap[">="] = CMPGE;
  operatorMap["="] = ASSIGN;
  operatorMap["++"] = INC;
  operatorMap["--"] = DEC;
  operatorMap["->"] = ARROW;
  operatorTable.resize(operatorMap.size());
  for(auto& it : operatorMap)
  {
    operatorTable[it.second] = it.first;
  }
  operatorPrec.resize(operatorMap.size());
  setOperatorPrec();
  punctMap[';'] = SEMICOLON;
  punctMap[':'] = COLON;
  punctMap['('] = LPAREN;
  punctMap[')'] = RPAREN;
  punctMap['{'] = LBRACE;
  punctMap['}'] = RBRACE;
  punctMap['['] = LBRACKET;
  punctMap[']'] = RBRACKET;
  punctMap['.'] = DOT;
  punctMap[','] = COMMA;
  punctMap['\\'] = BACKSLASH;
  punctMap['$'] = DOLLAR;
  punctMap['?'] = QUESTION;
  punctMap['#'] = HASH;
  punctTable.resize(punctMap.size());
  for(auto& it : punctMap)
  {
    punctTable[it.second] = it.first;
  }
  //set up operator commutativity table
  operCommutativeTable = vector<bool>(33, false);
  operCommutativeTable[PLUS] = true;
  operCommutativeTable[MUL] = true;
  operCommutativeTable[LOR] = true;
  operCommutativeTable[BOR] = true;
  operCommutativeTable[BXOR] = true;
  operCommutativeTable[LAND] = true;
  operCommutativeTable[BAND] = true;
  operCommutativeTable[CMPEQ] = true;
  operCommutativeTable[CMPNEQ] = true;
}

void setOperatorPrec()
{
  for(size_t i = 0; i < operatorPrec.size(); i++)
  {
    operatorPrec[i] = 0;
  }
  //note: lower value means lower precedence
  //only binary operators are given precedence
  operatorPrec[LOR] = 1;
  operatorPrec[LAND] = 2;
  operatorPrec[BOR] = 3;
  operatorPrec[BXOR] = 4;
  operatorPrec[BAND] = 5;
  operatorPrec[CMPEQ] = 6;
  operatorPrec[CMPNEQ] = 6;
  operatorPrec[CMPL] = 7;
  operatorPrec[CMPLE] = 7;
  operatorPrec[CMPG] = 7;
  operatorPrec[CMPGE] = 7;
  operatorPrec[SHL] = 8;
  operatorPrec[SHR] = 8;
  operatorPrec[PLUS] = 9;
  operatorPrec[SUB] = 9;
  operatorPrec[MUL] = 10;
  operatorPrec[DIV] = 10;
  operatorPrec[MOD] = 10;
}

Token::Token()
{
  type = INVALID_TOKEN_TYPE;
}

/* Identifier */
Ident::Ident()
{
  type = IDENTIFIER;
}

Ident::Ident(string n)
{
  type = IDENTIFIER;
  this->name = n;
}

bool Ident::compareTo(Token* rhs)
{
  if(rhs->type == IDENTIFIER && ((Ident*) rhs)->name == name)
    return true;
  return false;
}

bool Ident::operator==(Ident& rhs)
{
  return name == rhs.name;
}

string Ident::getStr()
{
  return string("ident \"") + name + "\"";
}

/* Operator */
Oper::Oper()
{
  type = OPERATOR;
}

Oper::Oper(OperatorEnum o)
{
  type = OPERATOR;
  this->op = o;
}

bool Oper::compareTo(Token* rhs)
{
  return rhs->type == OPERATOR && ((Oper*) rhs)->op == op;
}

bool Oper::operator==(Oper& rhs)
{
  return op == rhs.op;
}

string Oper::getStr()
{
  return operatorTable[op];
}

/* String Literal */
StrLit::StrLit()
{
  type = STRING_LITERAL;
}

StrLit::StrLit(string v)
{
  type = STRING_LITERAL;
  this->val = v;
}

bool StrLit::compareTo(Token* rhs)
{
  return rhs->type == STRING_LITERAL && ((StrLit*) rhs)->val == val;
}

bool StrLit::operator==(StrLit& rhs)
{
  return val == rhs.val;
}

string StrLit::getStr()
{
  string str = "\"";
  for(size_t i = 0; i < val.length(); i++)
  {
    str += generateChar(val[i]);
  }
  str += '\"';
  return str;
}

/* Character Literal */
CharLit::CharLit()
{
  type = CHAR_LITERAL;
}

CharLit::CharLit(char v)
{
  type = CHAR_LITERAL;
  this->val = v;
}

bool CharLit::compareTo(Token* rhs)
{
  return rhs->type == CHAR_LITERAL && ((CharLit*) rhs)->val == val;
}

bool CharLit::operator==(CharLit& rhs)
{
  return val == rhs.val;
}

string CharLit::getStr()
{
  if(isgraph(val))
    return string("'") + val + "'";
  char buf[16];
  sprintf(buf, "%#02hhx", val);
  return buf;
}

/* Integer Literal */
IntLit::IntLit()
{
  type = INT_LITERAL;
}

IntLit::IntLit(uint64_t v)
{
  type = INT_LITERAL;
  this->val = v;
}

bool IntLit::compareTo(Token* rhs)
{
  return rhs->type == INT_LITERAL && ((IntLit*) rhs)->val == val;
}

bool IntLit::operator==(IntLit& rhs)
{
  return val == rhs.val;
}

string IntLit::getStr()
{
  return to_string(val);
}

/* float/double literal */
FloatLit::FloatLit()
{
  type = FLOAT_LITERAL;
}

FloatLit::FloatLit(double v)
{
  type = FLOAT_LITERAL;
  this->val = v;
}

bool FloatLit::compareTo(Token* rhs)
{
  return rhs->type == FLOAT_LITERAL && ((FloatLit*) rhs)->val == val;
}

bool FloatLit::operator==(FloatLit& rhs)
{
  return val == rhs.val;
}

string FloatLit::getStr()
{
  return to_string(val);
}

/* Punctuation */
Punct::Punct()
{
  type = PUNCTUATION;
}

Punct::Punct(PunctEnum v)
{
  type = PUNCTUATION;
  this->val = v;
}

bool Punct::compareTo(Token* rhs)
{
  return rhs->type == PUNCTUATION && ((Punct*) rhs)->val == val;
}

bool Punct::operator==(Punct& rhs)
{
  return val == rhs.val;
}

string Punct::getStr()
{
  return string("") + punctTable[val];
}

/* Keyword */
Keyword::Keyword()
{
  type = KEYWORD;
}

Keyword::Keyword(string text)
{
  type = KEYWORD;
  KeywordEnum val = getKeyword(text);
  if(val == INVALID_KEYWORD)
  {
    INTERNAL_ERROR;
  }
  this->kw = val;
}

Keyword::Keyword(KeywordEnum val)
{
  type = KEYWORD;
  this->kw = val;
}

bool Keyword::compareTo(Token* rhs)
{
  return rhs->type == KEYWORD && ((Keyword*) rhs)->kw == kw;
}

bool Keyword::operator==(Keyword& rhs)
{
  return kw == rhs.kw;
}

string Keyword::getStr()
{
  return keywordTable[kw];
}

PastEOF::PastEOF()
{
  type = PAST_EOF;
}

bool PastEOF::compareTo(Token* t)
{
  return t->type == PAST_EOF;
}

bool PastEOF::operator==(PastEOF& rhs)
{
  return true;
}

string PastEOF::getStr()
{
  return "<INVALID TOKEN>";
}

/* Non-member utility functions */

KeywordEnum getKeyword(const string& str)
{
  auto it = keywordMap.find(str);
  if(it == keywordMap.end())
    return INVALID_KEYWORD;
  else
    return it->second;
}

PunctEnum getPunct(char c)
{
  auto it = punctMap.find(c);
  if(it == punctMap.end())
    return INVALID_PUNCT;
  else
    return it->second;
}

OperatorEnum getOper(const string& str)
{
  auto it = operatorMap.find(str);
  if(it == operatorMap.end())
    return INVALID_OPERATOR;
  else
    return it->second;
}

bool isOperCommutative(OperatorEnum o)
{
  return operCommutativeTable[o];
}

int getOperPrecedence(OperatorEnum o)
{
  return operatorPrec[o];
}

string getTokenTypeDesc(TokenTypeEnum tte)
{
  return tokTypeTable[tte];
}

string getTokenTypeDesc(Token* t)
{
  return getTokenTypeDesc(t->type);
}

