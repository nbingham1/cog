#include "Info.h"

#include <llvm/IR/Constants.h>
#include <sstream>

namespace Cog
{

Typename::Typename()
{
	prim = NULL;
	base = NULL;
	pointerCount = 0;
	isBool = false;
	isUnsigned = false;
	fpExp = 0;
}

Typename::~Typename()
{
}

std::string Typename::getName()
{
	std::ostringstream result;
	if (prim && prim->isVoidTy()) {
		result << "void";
	} if (prim && prim->isFloatingPointTy()) {
		result << "float" << prim->getPrimitiveSizeInBits();
	} else if (prim && prim->isIntegerTy()) {
		if (isBool)
			result << "bool";
		else {
			if (isUnsigned)
				result << "u";

			if (fpExp == 0)
				result << "int";
			else
				result << "fixed";

			result << prim->getIntegerBitWidth();

			if (fpExp != 0)
				result << "e" << fpExp;
		}
	} else if (base) {
		result << base->name;
	}
	return result.str();
}

bool operator==(const Typename &t1, const Typename &t2)
{
	return t1.prim == t2.prim
			&& t1.base == t2.base
			&& t1.pointerCount == t2.pointerCount
			&& t1.isBool == t2.isBool
			&& t1.isUnsigned == t2.isUnsigned
			&& t1.fpExp == t2.fpExp;
}

bool operator!=(const Typename &t1, const Typename &t2)
{
	return t1.prim != t2.prim
			|| t1.base != t2.base
			|| t1.pointerCount != t2.pointerCount
			|| t1.isBool != t2.isBool
			|| t1.isUnsigned != t2.isUnsigned
			|| t1.fpExp != t2.fpExp;
}

BaseType::BaseType()
{
	prim = NULL;
}

BaseType::~BaseType()
{
}

bool operator==(const BaseType &t1, const BaseType &t2)
{
	return t1.prim == t2.prim
			&& t1.retType == t2.retType
			&& t1.name == t2.name
			&& t1.recvType == t2.recvType
			&& t1.argTypes == t2.argTypes;
}

Symbol::Symbol(std::string name, Typename type)
{
	this->type = type;
	this->name = name;
	this->inst = NULL;
	this->values.push_back(llvm::UndefValue::get(type.prim));
	this->curr = this->values.begin();
	(*(this->curr))->setName(this->name);
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
