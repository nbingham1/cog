#include "Symbol.h"
#include "Type.h"

#include <iostream>
#include <parse/message.h>

namespace Cog
{

Symbol::Table Symbol::table;

Symbol::Symbol()
{
}

Symbol::Symbol(Type *type, std::string name)
{
	this->type = type;
	this->name = name;
}

Symbol::~Symbol()
{
}

llvm::Value *Symbol::undefValue()
{
	llvm::Value *result = type->undefValue();
	result->setName(name);
	return result;
}

Symbol* Symbol::declare(Symbol *s)
{
	std::pair<Symbol::Scope::iterator, bool> result = Symbol::table.back().insert(std::pair<std::string, Symbol*>(s->name, s));
	return result.first->second;
}

Symbol *Symbol::find(std::string name)
{
	Symbol::Scope::iterator result;
	int i = (int)table.size();
	if (i > 0) {
		do {
			--i;
			result = table[i].find(name);
		} while (i > 0 && result == table[i].end());
	}

	if (i > 0 || ((int)table.size() > 0 && result != table[0].end())) {
		return result->second;
	} else {
		return NULL;
	}
}

void Symbol::pushScope()
{
	table.push_back(Scope());
}

void Symbol::dropScope()
{
	if ((int)table.size() > 0) {
		table.pop_back();
	}
}

}

