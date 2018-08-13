#include "Variable.h"


namespace Cog
{

Instance::Instance(Type *type, std::string name) : Declaration(type, name)
{
	value = type->undefValue();
	value->setName(name);
}

Instance::Instance(const Declaration &decl) : Declaration(decl)
{
	value = type->undefValue();
	value->setName(name);
}

Instance::~Instance()
{
}

Symbol::Symbol(Type *type, std::string name) : Declaration(type, name)
{
	ref = NULL;

	values.push_back(type->undefValue());
	values.back().setName(name);
	curr = values.begin();
}

Symbol(const Declaration &decl) : Declaration(decl)
{
	ref = NULL;

	values.push_back(type->undefValue());
	values.back().setName(name);
	curr = values.begin();
}

Symbol(const Instance &inst) : Declaration(inst)
{
	this->ref = inst.ref;
	this->values.push_back(inst.value);
	this->curr = values.begin();
}

Symbol::Symbol(const Symbol &copy) : Declaration(copy)
{
	this->ref = copy.ref;
	this->values = copy.values;
	for (curr = values.begin(); curr != values.end() && *curr != *copy.curr; ++curr);
}

Symbol::~Symbol()
{
}

void Symbol::nextValue()
{
	++curr;
}

void Symbol::popValue()
{
	values.erase(curr++);
}

void Symbol::setValue(llvm::Value *value)
{
	*curr = value;
}

llvm::Value *Symbol::getValue()
{
	return *curr;
}

}

