#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

#include <functional>
#include <vector>
#include <unordered_map>
#include <string>

namespace Cog
{

struct Symbol;

struct Type
{
	typedef std::unordered_map<std::string, Type*> Table;

	static const int typeId = __COUNTER__;
	static Table table;
	
	Type();
	virtual ~Type();

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
		return id() == cmpType::typeId;
	}

	template <typename cmpType>
	bool is() const
	{
		return id() == cmpType::typeId;
	}

	virtual std::string name() const = 0;
	virtual int id() const = 0;
	virtual bool eq(Type *other) const = 0;
	llvm::Value *undefValue() const;

	static Type *define(Type *t);
	static Type *find(std::string name);
};

struct Void : Type
{
	static const int typeId = __COUNTER__;
	
	Void();
	~Void();

	std::string name() const;
	int id() const;
	bool eq(Type *other) const;
};

struct Boolean : Type
{
	static const int typeId = __COUNTER__;
	
	Boolean();
	~Boolean();

	std::string name() const;
	int id() const;
	bool eq(Type *other) const;
};

struct Fixed : Type
{
	static const int typeId = __COUNTER__;

	Fixed(std::string def);	
	Fixed(bool isSigned, int bitwidth, int exponent = 0);
	~Fixed();

	bool isSigned;
	int bitwidth;
	int exponent; // base 2

	std::string name() const;
	int id() const;
	bool eq(Type *other) const;
};

struct Float : Type
{
	static const int typeId = __COUNTER__;

	Float(std::string def);	
	Float(int bitwidth);
	~Float();

	int bitwidth;
	int sigwidth;
	int expmin;
	int expmax;

	std::string name() const;
	int id() const;
	bool eq(Type *other) const;
};

struct StaticArray : Type
{
	static const int typeId = __COUNTER__;
	
	StaticArray();
	StaticArray(Type *arrType, int size);
	~StaticArray();

	Type *arrType;
	int size;

	std::string name() const;
	int id() const;
	bool eq(Type *other) const;
};

struct Pointer : Type
{
	static const int typeId = __COUNTER__;
	
	Pointer();
	Pointer(Type *ptrType);
	~Pointer();

	Type *ptrType;

	std::string name() const;
	int id() const;
	bool eq(Type *other) const;
};


struct Struct : Type
{
	static const int typeId = __COUNTER__;
	
	Struct();
	Struct(std::string typeName);
	Struct(std::string typeName, std::vector<Symbol> members);
	~Struct();

	std::string typeName;
	std::vector<Symbol> members;

	std::string name() const;
	int id() const;
	bool eq(Type *other) const;
};

struct Tuple : Type
{
	static const int typeId = __COUNTER__;
	
	Tuple();
	Tuple(std::vector<Type*> members);
	~Tuple();

	std::vector<Type*> members;

	std::string name() const;
	int id() const;
	bool eq(Type *other) const;
};

struct Function : Type
{
	static const int typeId = __COUNTER__;
	
	Function();
	Function(Type *retType, Type *thisType, std::vector<Symbol*> args);
	~Function();

	Type *retType;
	Type *thisType;
	std::vector<Symbol> args;

	std::string name() const;
	int id() const;
	bool eq(Type *other) const;
};


}

