#include "Info.h"

#include <llvm/IR/Constants.h>

namespace Cog
{

Symbol::Symbol(std::string name, llvm::Value *value)
{
	this->type = value->getType();
	this->name = name;
	this->values.push_back(value);
	this->curr = this->values.begin();
}

Symbol::Symbol(std::string name, llvm::Type *type)
{
	this->type = type;
	this->name = name;
	this->values.push_back(llvm::UndefValue::get(type));
	this->curr = this->values.begin();
}

Symbol::Symbol(const Symbol &copy)
{
	this->type = copy.type;
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

Scope::Scope()
{
}

Scope::Scope(llvm::BasicBlock *block)
{
	blocks.push_back(block);
	curr = blocks.begin();
}

Scope::Scope(Scope *from)
{
	blocks.push_back(from->getBlock());
	curr = blocks.begin();
	for (std::vector<Symbol>::iterator i = from->symbols.begin(); i != from->symbols.end(); i++) {
		symbols.push_back(Symbol(i->name, i->getValue()));
	}
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
	for (int i = 0; i < (int)symbols.size(); i++)
		symbols[i].nextValue();
	++curr;
}

void Scope::appendBlock(llvm::BasicBlock *block)
{
	for (int i = 0; i < (int)symbols.size(); i++)
		symbols[i].values.push_back(symbols[i].getValue());
	blocks.push_back(block);
}

void Scope::dropBlock()
{
	for (int i = 0; i < (int)symbols.size(); i++)
		symbols[i].values.pop_front();
	blocks.pop_front();
}

void Scope::popBlock()
{
	for (int i = 0; i < (int)symbols.size(); i++)
		symbols[i].popValue();
	blocks.erase(curr++);
}

void Scope::setBlock(llvm::BasicBlock *block)
{
	if (blocks.size() == 0) {
		blocks.push_back(block);
		curr = blocks.begin();
	} else {
		*curr = block;
	}
}

llvm::BasicBlock *Scope::getBlock()
{
	return *curr;
}

void Scope::merge(Scope *from)
{
	setBlock(from->getBlock());
	for (int i = 0; i < (int)symbols.size() && i < (int)from->symbols.size(); i++) {
		symbols[i].setValue(from->symbols[i].getValue());
	}
}	

Symbol* Scope::findSymbol(std::string name)
{
	for (auto symbol = symbols.rbegin(); symbol != symbols.rend(); ++symbol) {
		if (symbol->name == name) {
			return &(*symbol);
		}
	}

	return NULL;
}

Symbol *Scope::createSymbol(std::string name, llvm::Type *type)
{
	symbols.push_back(Cog::Symbol(name, type));
	return &symbols.back();
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
	next = NULL;
}

Info::~Info()
{
	if (next != NULL)
		delete next;
	next = NULL;
}

}
