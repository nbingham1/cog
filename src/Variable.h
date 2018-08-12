#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

#include <list>

namespace Cog
{

struct Symbol : Declaration
{
	static const int id = __COUNTER__;

	Symbol(Type *type, std::string name);
	Symbol(const Declaration &decl);
	Symbol(Instance *ref, int index);
	~Symbol();

	std::list<llvm::Value*> values;
	std::list<llvm::Value*>::iterator curr;

	void nextValue();
	void popValue();
	void setValue(llvm::Value *value);
	llvm::Value *getValue();
};

}
