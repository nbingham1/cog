#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>

#include <vector>
#include <list>
#include <string>

namespace Cog
{

struct Symbol
{
	Symbol(std::string name, llvm::Type *type);
	Symbol(std::string name, llvm::Value *value);
	Symbol(const Symbol &copy);
	~Symbol();

	llvm::Type *type;
	std::string name;

	std::list<llvm::Value*> values;
	std::list<llvm::Value*>::iterator curr;

	void nextValue();
	void popValue();
	void setValue(llvm::Value *value);
	llvm::Value *getValue();
};

struct Scope
{
	Scope(llvm::BasicBlock *block);
	Scope(Scope *from);
	Scope(const Scope &copy);
	~Scope();

	std::vector<Symbol> symbols;
	std::list<llvm::BasicBlock*> blocks;
	std::list<llvm::BasicBlock*>::iterator curr;

	void nextBlock();
	void appendBlock(llvm::BasicBlock *block);
	void dropBlock();
	void popBlock();
	void setBlock(llvm::BasicBlock *block);
	llvm::BasicBlock *getBlock();

	void merge(Scope *from);	
};

struct Typename
{
	Typename();
	~Typename();

	std::string name;
	llvm::Type *type;
};

struct Info
{
	Info();
	~Info();

	llvm::Type *type;
	llvm::Value *value;
	Symbol *symbol;
};

}
