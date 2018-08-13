#pragma once

#include "Type.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

#include <list>

namespace Cog
{

struct Instance : Declaration
{
	static const int id = __COUNTER__;

	Instance(Type *type, std::string name);
	Instance(const Declaration &decl);
	~Instance();

	Instance *ref;

	llvm::Value *value;
};

struct Symbol : Declaration
{
	static const int id = __COUNTER__;

	Symbol(Type *type, std::string name);
	Symbol(const Declaration &decl);
	Symbol(const Instance &inst);
	~Symbol();

	Instance *ref;

	std::list<llvm::Value*> values;
	std::list<llvm::Value*>::iterator curr;

	void nextValue();
	void popValue();
	void setValue(llvm::Value *value);
	llvm::Value *getValue();
};

}
