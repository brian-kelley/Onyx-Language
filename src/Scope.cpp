#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Subroutine.hpp"

int BlockScope::nextBlockIndex = 0;

/*******************************
*   Scope & subclasses impl    *
*******************************/

#define ADD_NAME(T, tname, tenum) \
  void Scope::addName(T* item) \
  { \
    if(names.find(item->name) != names.end()) \
    { \
      ERR_MSG(tname << ' ' << item->name << " causes scope name conflict"); \
    } \
    shadowCheck(item->name); \
    names[item->name] = Name(item, this); \
  }

ADD_NAME(ModuleScope,            "module",      Name::MODULE);
ADD_NAME(TypeSystem::StructType, "struct",      Name::STRUCT);
ADD_NAME(TypeSystem::EnumType,   "enum",        Name::ENUM);
ADD_NAME(TypeSystem::AliasType,  "typedef",     Name::TYPEDEF);
ADD_NAME(TypeSystem::BoundedType,"bounded type",Name::BOUNDED_TYPE);
ADD_NAME(TypeSystem::Trait,      "trait",       Name::TRAIT);
ADD_NAME(Subroutine,             "subroutine",  Name::SUBROUTINE);
ADD_NAME(Variable,               "variable",    Name::VARIABLE);

Scope::Scope(Scope* parentIn)
{
  parent = parentIn;
  if(parent)
  {
    parent->children.push_back(this);
  }
}

string Scope::getFullPath()
{
  if(parent)
    return parent->getFullPath() + '_' + getLocalName();
  else
    return getLocalName();
}

Name Scope::lookup(string name)
{
  auto it = names.find(name);
  if(it == names.end())
    return Name();
  return it->second;
}

Name Scope::findName(Parser::Member* mem)
{
  //scope is the scope that actually contains name mem->tail
  Scope* scope = this;
  for(size_t i = 0; i < mem->names.size(); i++)
  {
    Name it = scope->lookup(mem->names[i]);
    if(it.item && i == mem->names.size() - 1)
    {
      return it;
    }
    else if(it.item)
    {
      //make sure that it is actually a scope of some kind
      //(MODULE and STRUCT are the only named scopes for this purpose)
      if(it.kind == Name::MODULE)
      {
        //module is already scope
        scope = (Scope*) it.item;
        continue;
      }
      else if(it.kind == Name::STRUCT)
      {
        scope = ((TypeSystem::StructType*) it.item)->structScope;
        continue;
      }
    }
    else
    {
      scope = nullptr;
      break;
    }
  }
  //try search again in parent scope
  if(parent)
    return parent->findName(mem);
  //failure
  return Name();
}

Name Scope::findName(string name)
{
  Parser::Member m;
  m.names.push_back(name);
  return findName(&m);
}

void Scope::shadowCheck(string name)
{
  for(Scope* iter = this; iter; iter = iter->parent)
  {
    Name n = iter->lookup(name);
    if(n.item)
    {
      ERR_MSG("name " << name << " shadows a previous declaration");
    }
  }
}

/* ModuleScope */

ModuleScope::ModuleScope(string nameIn, Scope* par, Parser::Module* astIn) : Scope(par)
{
  name = nameIn;
}

string ModuleScope::getLocalName()
{
  return name;
}

/* StructScope */

StructScope::StructScope(string nameIn, Scope* par, Parser::StructDecl* astIn) : Scope(par), name(nameIn) {}

string StructScope::getLocalName()
{
  return name;
}

/* SubroutineScope */

string SubroutineScope::getLocalName()
{
  return subr->name;
}

/* BlockScope */

BlockScope::BlockScope(Scope* par, Parser::Block* astIn) : Scope(par), index(nextBlockIndex++) {}

BlockScope::BlockScope(Scope* par) : Scope(par), index(nextBlockIndex++) {}

string BlockScope::getLocalName()
{
  //note: Onyx identifiers can't begin with underscore, so if it ever
  //matters this local name can't conflict with any other scope name
  return string("_B") + to_string(index);
}

/* TraitScope */

TraitScope::TraitScope(Scope* par, string n) : Scope(par), ttype(nullptr), name(n) {}

string TraitScope::getLocalName()
{
  return name;
}

