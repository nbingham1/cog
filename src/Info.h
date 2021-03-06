#pragma once

#include "Type.h"
#include "Variable.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

#include <vector>
#include <list>
#include <string>

namespace Cog
{

struct Scope
{
	Scope();
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

	Symbol *findSymbol(std::string name);
	Symbol *createSymbol(Type *type, std::string name);
};

struct Info
{
	Info();
	~Info();

	std::string text;
	Type *type;
	llvm::Value *value;
	Declaration *variable;
	
	Info *args;
	Info *next;
};

}
