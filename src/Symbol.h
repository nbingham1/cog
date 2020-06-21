#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

#include <vector>
#include <unordered_map>

namespace Cog
{

struct Type;

struct Symbol
{
	typedef std::unordered_map<std::string, Symbol*> Scope;
	typedef std::vector<Scope> Table;

	static Table table;

	Symbol();
	Symbol(Type *type, std::string name);
	virtual ~Symbol();

	Type *type;	
	std::string name;

	llvm::Value *undefValue();

	static Symbol *declare(Symbol *s);
	static Symbol *find(std::string name);
	static void pushScope();
	static void dropScope();
};

}
