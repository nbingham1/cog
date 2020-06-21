#pragma once

#include "Type.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

#include <vector>
#include <unordered_map>

namespace Cog
{

struct Value
{
	typedef std::unordered_map<Symbol*, llvm::Value*> Scope;
	typedef std::vector<Scope> Table;

	static Table table;

	Value(Type *type, std::string name);
	~Value();

	llvm::Value *llvmValue;

	static Value *find(std::string name);
};

}
