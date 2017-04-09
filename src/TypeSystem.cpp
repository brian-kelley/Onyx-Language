#include "TypeSystem.hpp"

using namespace std;
using namespace Parser;

vector<AP(Type)> Type::table;

/***********************/
/* Type and subclasses */
/***********************/

//Get the type table entry, given the local usage name and current scope
Type* Type::getType(string localName, Scope* usedScope)
{
}

Type* Type::getType(Parser::Member& localName, Scope* usedScope)
{
}

StructType(string name, Scope* enclosingScope)
{
  this->name = name;
  this->enclosing = enclosingScope;
}

string StructType::getCName()
{
}

bool StructType::hasFunc(ProcType& type)
{
}

bool StructType::hasProc(ProcType& type)
{
}

TupleType::TupleType(TupleType& tt)
{
}

string TupleType::getCName()
{
}

AliasType::AliasType(string newName, Type* t)
{
}

AliasType::AliasType(Typedef& td)
{
}

string AliasType::getCName()
{
  return actual->getCName();
}

EnumType::EnumType(Parser::Enum& e)
{
}

string EnumType::getCName()
{
}

IntegerType::IntegerType(string name, int size, bool sign)
{
  this->name = name;
  this->size = size;
  this->isSigned = sign;
}

string IntegerType::getCName()
{
  if(size == 1 && isSigned)
    return "char";
  else if(size == 1)
    return "uchar";
  else if(size == 2 && isSigned)
    return "short";
  else if(size == 2)
    return "ushort";
  else if(size == 4 && isSigned)
    return "int";
  else if(size == 4)
    return "uint";
  else if(size == 8 && isSigned)
    return "long";
  else if(size == 8)
    return "ulong";
  else
  {
    //todo: support larger-precision ints
  }
}

FloatType::FloatType(string name, int size)
{
  this->name = name;
  this->size = size;
}

string FloatType::getCName()
{
  if(size == 4)
  {
    return "float";
  }
  else if(size == 8)
  {
    return "double"
  }
  else
  {
    return "#error undefined type";
  }
}

string StringType::getCName()
{
  return "string";
}

int UndefType::num = 0;

UndefType::UndefType()
{
}

string UndefType::getCName()
{
  return "#error undefined type";
}

