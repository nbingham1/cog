#include "Info.h"
#include "Compiler.h"

#include <llvm/IR/Constants.h>
#include <sstream>

extern Cog::Compiler cog;

namespace Cog
{

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
		symbols.push_back(Symbol(i->name, i->type));
		symbols.back().inst = i->inst;
		if (i->values.size() > 0) {
			symbols.back().values.push_back(i->getValue());
			symbols.back().curr = symbols.back().values.begin();
		}
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

Symbol *Scope::createSymbol(std::string name, Typename type)
{
	symbols.push_back(Cog::Symbol(name, type));
	return &symbols.back();
}

Info::Info()
{
	args = NULL;
	value = NULL;
	symbol = NULL;
	next = NULL;
}

Info::~Info()
{
	if (next != NULL)
		delete next;
	next = NULL;

	if (args != NULL)
		delete args;
	args = NULL;
}

}
