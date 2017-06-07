#include "Scope.hpp"

int BlockScope::nextBlockIndex = 0;

/*******************************
*   Scope & subclasses impl    *
*******************************/

Scope::Scope(Scope* parent)
{
  this->parent = parent;
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

/* ModuleScope */

ModuleScope::ModuleScope(string name, Scope* parent) : Scope(parent)
{
  cout << "Making module scope \"" << name << "\"";
  if(parent)
  {
    cout << " with parent \"" << parent->getLocalName() << "\"\n";
  }
  else
  {
    cout << " with no parent (global)\n";
  }
  this->name = name;
}

string ModuleScope::getLocalName()
{
  return name;
}

/* StructScope */

StructScope::StructScope(string name, Scope* parent) : Scope(parent)
{
  this->name = name;
}

string StructScope::getLocalName()
{
  return name;
}

/* BlockScope */

BlockScope::BlockScope(Scope* parent) : Scope(parent)
{
  index = nextBlockIndex++;
}

string BlockScope::getLocalName()
{
  return string("_B") + to_string(index);
}

