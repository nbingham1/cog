#include "Info.h"

#include <llvm/IR/Constants.h>

namespace Cog
{

Symbol::Symbol(std::string name, llvm::Type *type)
{
	this->name = name;
	this->values.push_back(llvm::UndefValue::get(type));
	this->curr = this->values.begin();
}

Symbol::Symbol(const Symbol &copy)
{
	this->name = copy.name;
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

Scope::Scope(llvm::BasicBlock *block)
{
	blocks.push_back(block);
	curr = blocks.begin();
}

Scope::Scope(const Scope &copy)
{
	symbols = copy.symbols;
	blocks = copy.blocks;
	for (curr = blocks.begin(); curr != blocks.end() && *curr != *copy.curr; ++curr);
}

Scope::~Scope()
{
}

void Scope::nextBlock()
{
	++curr;
}

void Scope::popBlock()
{
	blocks.erase(curr++);
}

void Scope::setBlock(llvm::BasicBlock *block)
{
	*curr = block;
}

llvm::BasicBlock *Scope::getBlock()
{
	return *curr;
}

Typename::Typename()
{
	type = NULL;
}

Typename::~Typename()
{
}

Info::Info()
{
	type = NULL;
	value = NULL;
	symbol = NULL;
}

Info::~Info()
{
}

}
