#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <string>

namespace Cog
{

struct Symbol
{
	Symbol();
	~Symbol();

	int scope;
	std::string name;

	llvm::Value *value;
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
	int symbol;
};

}
