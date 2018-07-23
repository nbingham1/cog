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
struct PrimType;
struct Symbol;

struct Typename
{
	Typename();
	Typename(const Typename &copy);
	~Typename();

	PrimType *prim;
	BaseType *base;

	int pointerCount;

	llvm::Type *getLlvm() const;
	std::string getName() const;
	llvm::Value *undefValue() const;
	bool isSet() const;

	void setPrimType(llvm::Type *llvmType, int kind, int bitwidth = 0, int exponent = 0, int pointerCount = 0);

	Typename &operator=(const Typename &copy);
};

bool operator==(const Typename &t1, const Typename &t2);
bool operator!=(const Typename &t1, const Typename &t2);

struct Declaration
{
	Declaration();
	Declaration(Typename type, std::string name);
	Declaration(const Symbol *symbol);
	~Declaration();

	Typename type;
	std::string name;
};

bool operator==(const Declaration &left, const Declaration &right);
bool operator!=(const Declaration &left, const Declaration &right);

struct PrimType
{
	PrimType();
	PrimType(const PrimType &copy);
	PrimType(llvm::Type *llvmType, int kind, int bitwidth = 0, int exponent = 0);
	~PrimType();

	llvm::Type *llvmType;

	enum {
		Void = 0,
		Boolean = 1,
		Unsigned = 2,
		Signed = 3,
		Float = 4
	};

	int kind;
	int bitwidth;
	int exponent;

	std::string getName() const;
	llvm::Value *undefValue() const;
};

bool operator==(const PrimType &t1, const PrimType &t2);
bool operator!=(const PrimType &t1, const PrimType &t2);

/*

Functions: prim, retType, name, argTypes
Member Functions: prim, retType, name, recvType, argTypes
Structures: prim, name, argTypes
Interfaces: name, argTypes

*/
struct BaseType
{
	BaseType();
	~BaseType();

	llvm::Type *llvmType;

	Typename retType;
	std::string name;
	Typename thisType;
	std::vector<Declaration> args;

	std::string getName(bool withRet = true) const;
	llvm::Value *undefValue() const;
};

bool operator==(const BaseType &t1, const BaseType &t2);
bool operator!=(const BaseType &t1, const BaseType &t2);

struct BaseValue
{
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

	std::string text;
	Typename type;
	llvm::Value *value;
	Symbol *symbol;
	
	Info *args;
	Info *next;
};

}
