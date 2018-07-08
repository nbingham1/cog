#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

#include <vector>
#include <list>
#include <string>

namespace Cog
{

struct BaseType;

struct Typename
{
	Typename();
	~Typename();

	llvm::Type *prim;
	BaseType *base;

	int pointerCount;

	bool isBool;
	bool isUnsigned;
	int bitwidth;
	int fpExp;

	std::string getName();
};

bool operator==(const Typename &t1, const Typename &t2);
bool operator!=(const Typename &t1, const Typename &t2);

/*

Primitives: prim, name
Functions: prim, retType, name, argTypes
Member Functions: prim, retType, name, recvType, argTypes
Structures: prim, name, argTypes
Interfaces: name, argTypes

*/
struct BaseType
{
	BaseType();
	~BaseType();

	llvm::Type *prim;

	Typename retType;
	std::string name;
	Typename recvType;
	std::vector<Typename> argTypes;
};

bool operator==(const BaseType &t1, const BaseType &t2);

struct BaseValue
{
	Typename type;
	llvm::Value *value;
	llvm::AllocaInst *inst;
};

struct Symbol
{
	Symbol(std::string name, Typename type);
	Symbol(const Symbol &copy);
	~Symbol();

	Typename type;
	std::string name;
	llvm::AllocaInst *inst;

	std::list<llvm::Value*> values;
	std::list<llvm::Value*>::iterator curr;

	void nextValue();
	void popValue();
	void setValue(llvm::Value *value);
	llvm::Value *getValue();
};

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
	Symbol *createSymbol(std::string name, Typename type);
};

struct Info
{
	Info();
	~Info();

	Typename type;
	llvm::Value *value;
	Symbol *symbol;
	Info *next;
};

}
