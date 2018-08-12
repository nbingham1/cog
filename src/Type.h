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

struct Type
{
	static const int id = __COUNTER__;

	Type();
	~Type();

	llvm::Type *llvmType;

	template <typename ToType>
	ToType *get(const ToType *to = NULL)
	{
		return (ToType*)this;
	}

	template <typename ToType>
	const ToType *get(const ToType *to = NULL) const
	{
		return (const ToType*)this;
	}

	template <typename cmpType>
	bool is()
	{
		return id == cmpType::id;
	}

	template <typename cmpType>
	bool is() const
	{
		return id == cmpType::id;
	}

	virtual std::string name() const = 0;
	virtual bool eq(Type *other) const = 0;
};

struct Declaration
{
	static const int id = __COUNTER__;

	Declaration();
	Declaration(Type *type, std::string name);
	~Declaration();

	Type *type;
	std::string name;
};

struct Void : Type
{
	static const int id = __COUNTER__;
	
	Void();
	~Void();

	std::string name() const;
	bool eq(Type *other) const;
};

struct Boolean : Type
{
	static const int id = __COUNTER__;
	
	Boolean();
	~Boolean();

	std::string name() const;
	bool eq(Type *other) const;
};

struct Fixed : Type
{
	static const int id = __COUNTER__;
	
	Fixed(bool isSigned, int bitwidth, int exponent = 0);
	~Fixed();

	bool isSigned;
	int bitwidth;
	int exponent; // base 2

	std::string name() const;
	bool eq(Type *other) const;
};

struct Float : Type
{
	static const int id = __COUNTER__;
	
	Float(int bitwidth);
	~Float();

	int bitwidth;
	int sigwidth;
	int expmin;
	int expmax;

	std::string name() const;
	bool eq(Type *other) const;
};

struct StaticArray : Type
{
	static const int id = __COUNTER__;
	
	StaticArray();
	StaticArray(Type *arrType, int size);
	~StaticArray();

	Type *arrType;
	int size;

	std::string name() const;
	bool eq(Type *other) const;
};

struct Pointer : Type
{
	static const int id = __COUNTER__;
	
	Pointer();
	Pointer(Type *ptrType);
	~Pointer();

	Type *ptrType;

	std::string name() const;
	bool eq(Type *other) const;
};

struct Struct : Type
{
	static const int id = __COUNTER__;
	
	Struct();
	Struct(std::string typeName);
	Struct(std::string typeName, std::vector<Declaration> members);
	~Struct();

	std::string typeName;
	std::vector<Declaration> members;

	std::string name() const;
	bool eq(Type *other) const;
};

struct Tuple : Type
{
	static const int id = __COUNTER__;
	
	Tuple();
	Tuple(std::vector<Declaration> members);
	~Tuple();

	std::vector<Declaration> members;

	std::string name() const;
	bool eq(Type *other) const;
};

struct Function : Type
{
	static const int id = __COUNTER__;
	
	Function();
	Function(Type *retType, Type *thisType, std::string typeName, std::vector<Declaration> args);
	~Function();

	std::string typeName;
	Type *retType;
	Type *thisType;
	std::vector<Declaration> args;

	std::string name() const;
	bool eq(Type *other) const;
};

}

