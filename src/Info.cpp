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
}

Typename::Typename(const Typename &copy)
{
	if (copy.prim != NULL)
		prim = new PrimType(*(copy.prim));
	else
		prim = NULL;
	base = copy.base;
	pointerCount = copy.pointerCount;
}

Typename::~Typename()
{
	if (prim)
		delete prim;
	prim = NULL;
}

llvm::Type *Typename::getLlvm() const
{
	if (prim)
		return prim->llvmType;
	else if (base)
		return base->llvmType;
	else
		return NULL;
}

std::string Typename::getName() const
{
	std::string result;
	if (prim)
		result = prim->getName();
	else if (base)
		result = base->getName();
	else
		result = "(undefined)";
	for (int i = 0; i < pointerCount; i++)
		result += "*";
	return result;
}

llvm::Value *Typename::undefValue() const
{
	if (prim)
		return prim->undefValue();
	else if (base)
		return base->undefValue();
	else
		return NULL;
}

bool Typename::isSet() const
{
	return (prim != NULL || base != NULL);
}

void Typename::setPrimType(llvm::Type *llvmType, int kind, int bitwidth, int exponent, int pointerCount)
{
	if (prim)
		delete prim;
	base = NULL;
	
	prim = new PrimType(llvmType, kind, bitwidth, exponent);

	this->pointerCount = pointerCount;
}

Typename &Typename::operator=(const Typename &copy)
{
	if (prim)
		delete prim;

	if (copy.prim)
		prim = new PrimType(*(copy.prim));
	else
		prim = NULL;
	base = copy.base;
	pointerCount = copy.pointerCount;
	return *this;
}

bool operator==(const Typename &t1, const Typename &t2)
{
	if (t1.pointerCount != t2.pointerCount)
		return false;

	if (t1.prim != NULL && t2.prim != NULL) {
		return (*(t1.prim) == *(t2.prim));
	} else if (t1.base != NULL && t2.base != NULL) {
		return (*(t1.base) == *(t2.base));
	} else {
		return true;
	}
}

bool operator!=(const Typename &t1, const Typename &t2)
{
	return !(t1 == t2);
}

Declaration::Declaration()
{
}

Declaration::Declaration(Typename type, std::string name)
{
	this->type = type;
	this->name = name;
}

Declaration::Declaration(const Symbol *symbol)
{
	if (symbol != NULL) {
		this->type = symbol->type;
		this->name = symbol->name;
	}
}

Declaration::~Declaration()
{
}

bool operator==(const Declaration &left, const Declaration &right)
{
	return left.type == right.type;
}

bool operator!=(const Declaration &left, const Declaration &right)
{
	return left.type == right.type;
}

PrimType::PrimType()
{
	llvmType = NULL;
	kind = Void;
	bitwidth = 0;
	exponent = 0;
}

PrimType::PrimType(const PrimType &copy)
{
	this->llvmType = copy.llvmType;
	this->kind = copy.kind;
	this->bitwidth = copy.bitwidth;
	this->exponent = copy.exponent;
}

PrimType::PrimType(llvm::Type *llvmType, int kind, int bitwidth, int exponent)
{
	this->llvmType = llvmType;
	this->kind = kind;
	this->bitwidth = bitwidth;
	this->exponent = exponent;
}

PrimType::~PrimType()
{
}

std::string PrimType::getName() const
{
	std::ostringstream result;
	if (llvmType) {
		if (llvmType->isVoidTy()) {
			result << "void";
		} if (llvmType->isFloatingPointTy()) {
			result << "float" << bitwidth;
		} else if (llvmType->isIntegerTy()) {
			if (kind == Boolean)
				result << "bool";
			else {
				if (kind == Unsigned)
					result << "u";

				if (exponent == 0)
					result << "int";
				else
					result << "fixed";

				result << bitwidth;

				if (exponent != 0)
					result << "e" << exponent;
			}
		}
		return result.str();
	} else {
		return "(undefined)";
	} 
}

llvm::Value *PrimType::undefValue() const
{
	return llvm::UndefValue::get(llvmType);
}

bool operator==(const PrimType &t1, const PrimType &t2)
{
	return t1.llvmType == t2.llvmType
			&& t1.kind == t2.kind
			&& t1.bitwidth == t2.bitwidth
			&& t1.exponent == t2.exponent;
}

bool operator!=(const PrimType &t1, const PrimType &t2)
{
	return t1.llvmType != t2.llvmType
			|| t1.kind != t2.kind
			|| t1.bitwidth != t2.bitwidth
			|| t1.exponent != t2.exponent;
}

BaseType::BaseType()
{
	llvmType = NULL;
}

BaseType::~BaseType()
{
}

std::string BaseType::getName(bool withRet) const
{
	if (llvmType->isStructTy())
		return name;
	else {
		std::string result;
		if (withRet)
			"(" + retType.getName() + ")";
		result += "(" + thisType.getName() + ")";
		result += name;
		for (int i = 0; i < (int)args.size(); i++)
			result += "(" + args[i].type.getName() + ")";
		return result;
	}
}

llvm::Value *BaseType::undefValue() const
{
	if (llvmType)
		return llvm::UndefValue::get(llvmType);
	else
		return NULL;
}

bool operator==(const BaseType &t1, const BaseType &t2)
{
	if (t1.llvmType != t2.llvmType)
		return false;

	if (t1.llvmType->isStructTy())
		return t1.name == t2.name;
	else if (t1.llvmType->isFunctionTy()) {
		if (t1.retType != t2.retType
		 || t1.thisType != t2.thisType
		 || t1.args != t2.args)
			return false;

		return true;
	}
	
	return true;
}

bool operator!=(const BaseType &t1, const BaseType &t2)
{
	return !(t1 == t2);
}

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
