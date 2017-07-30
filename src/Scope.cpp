#include "Scope.hpp"

int BlockScope::nextBlockIndex = 0;

/*******************************
*   Scope & subclasses impl    *
*******************************/

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

void Scope::findSubImpl(vector<string>& names, vector<Scope*>& matches)
{
  if(names.size() == 0)
  {
    //search this scope and then all parents
    for(Scope* iter = this; iter; iter = iter->parent)
      matches.push_back(iter);
    return;
  }
  //does this contain a scope chain with path given by names?
  Scope* next = this;
  bool found = false;
  for(auto n : names)
  {
    for(auto child : next->children)
    {
      if(child->getLocalName() == n)
      {
        found = true;
        next = child;
        break;
      }
    }
    if(!found)
      break;
  }
  //if out of the loop with found == true, next is the requested child scope
  if(found)
    matches.push_back(next);
  if(!parent)
  {
    //no more scopes to seach, so done
    return;
  }
  else
  {
    //append any matches found in parent scopes
    parent->findSubImpl(names, matches);
  }
}

vector<Scope*> Scope::findSub(vector<string>& names)
{
  vector<Scope*> matches;
  findSubImpl(names, matches);
  return matches;
}

/* ModuleScope */

ModuleScope::ModuleScope(string nameIn, Scope* parent, Parser::Module* astIn) : Scope(parent)
{
  name = nameIn;
  ast = astIn;
}

string ModuleScope::getLocalName()
{
  return name;
}

/* StructScope */

StructScope::StructScope(string nameIn, Scope* parent, Parser::StructDecl* astIn) : Scope(parent), ast(astIn), name(nameIn) {}

string StructScope::getLocalName()
{
  return name;
}

/* BlockScope */

BlockScope::BlockScope(Scope* parent, Parser::Block* astIn) : Scope(parent), ast(astIn), index(nextBlockIndex++), statementCounter(0) {}

string BlockScope::getLocalName()
{
  //TODO: prevent all other identifiers from having a name which could be confused as a block name
  return string("_B") + to_string(index);
}

