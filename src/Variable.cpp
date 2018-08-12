#include "Variable.h"


namespace Cog
{

Symbol::Symbol(std::string name, Typename type)
{
	this->type = type;
	this->name = name;
	this->inst = NULL;
	llvm::Value *value = type.undefValue();
	if (value) {
		value->setName(this->name);
		this->values.push_back(value);
	}
	this->curr = this->values.begin();
}

Symbol::Symbol(const Symbol &copy)
{
	this->type = copy.type;
	this->name = copy.name;
	this->inst = copy.inst;
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

