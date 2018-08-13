#include "Type.h"
#include "Compiler.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>

#include <sstream>

extern Cog::Compiler cog;

namespace Cog
{

Type::Type()
{
	llvmType = NULL;
}

Type::~Type()
{
}

llvm::Value *Type::undefValue() const
{
	return llvm::UndefValue::get(llvmType);
}

Declaration::Declaration()
{
}

Declaration::Declaration(Type *type, std::string name)
{
	this->type = type;
	this->name = name;
}

Declaration::~Declaration()
{
}

Void::Void()
{
	llvmType = llvm::Type::getVoidTy(cog.context);
}

Void::~Void()
{
}

std::string Void::name() const
{
	return "void";
}

bool Void::eq(Type *other) const
{
	return other->id == id;
}

Boolean::Boolean()
{
	llvmType = llvm::Type::getInt1Ty(cog.context);
}

Boolean::~Boolean()
{
}

std::string Boolean::name() const
{
	return "bool";
}

bool Boolean::eq(Type *other) const
{
	return other->id == id;
}

Fixed::Fixed(bool isSigned, int bitwidth, int exponent)
{
	this->isSigned = isSigned;
	this->bitwidth = bitwidth;
	this->exponent = exponent;
	this->llvmType = llvm::Type::getIntNTy(cog.context, bitwidth);
}

Fixed::~Fixed()
{
}

std::string Fixed::name() const
{
	std::stringstream result;
	if (isSigned)
		result << "u";

	// TODO exponent needs to be base 10
	if (exponent == 0)
		result << "int" << bitwidth;
	else
		result << "fixed" << bitwidth << "e" << exponent;
	return result.str();
}

bool Fixed::eq(Type *other) const
{
	if (other->id != id)
		return false;

	const Fixed *otherFixed = other->get(this);
	return otherFixed->bitwidth == bitwidth
	    && otherFixed->exponent == exponent
	    && otherFixed->isSigned == isSigned;
}

Float::Float(int bitwidth)
{
	this->bitwidth = bitwidth;
	switch (bitwidth) {
	case 16:
		this->sigwidth = 11;
		this->expmin = -15;
		this->expmax = 15;
		this->llvmType = llvm::Type::getHalfTy(cog.context);
		break;
	case 32:
		this->sigwidth = 24;
		this->expmin = -127;
		this->expmax = 128;
		this->llvmType = llvm::Type::getFloatTy(cog.context);
		break;
	case 64:
		this->sigwidth = 53;
		this->expmin = -1023;
		this->expmax = 1024;
		this->llvmType = llvm::Type::getDoubleTy(cog.context);
		break;
	case 80:
		this->sigwidth = 64;
		this->expmin = -16383;
		this->expmax = 16384;
		this->llvmType = llvm::Type::getX86_FP80Ty(cog.context);
		break;
	case 128:
		this->sigwidth = 113;
		this->expmin = -16383;
		this->expmax = 16384;
		this->llvmType = llvm::Type::getFP128Ty(cog.context);
		break;
	default:
		this->llvmType = NULL;
		this->bitwidth = 0;
		this->sigwidth = 0;
		this->expmin = 0;
		this->expmax = 0;
		error() << "floating point types must be 16, 32, 64, or 128 bits." << std::endl;
	}
}

Float::~Float()
{
}

std::string Float::name() const
{
	std::stringstream result;
	result << "float" << bitwidth;
	return result.str();
}

bool Float::eq(Type *other) const
{
	if (other->id != id)
		return false;

	const Float *otherFloat = other->get(this);
	return otherFloat->bitwidth == bitwidth;
}

StaticArray::StaticArray()
{
	this->llvmType = NULL;
	this->arrType = NULL;
	this->size = -1;
}

StaticArray::StaticArray(Type *arrType, int size)
{
	this->arrType = arrType;
	this->size = size;
	this->llvmType = llvm::ArrayType::get(arrType->llvmType, size);
}

StaticArray::~StaticArray()
{
}

std::string StaticArray::name() const
{
	std::stringstream result;
	if (arrType != NULL)
		result << arrType->name() << "[" << size << "]";
	else
		result << "undefined[" << size << "]";
	return result.str();
}

bool StaticArray::eq(Type *other) const
{
	if (other->id != id)
		return false;

	const StaticArray *otherArray = other->get(this);
	if (otherArray->size != size)
		return false;

	return arrType->eq(otherArray->arrType);
}

Pointer::Pointer()
{
	this->ptrType = NULL;
	this->llvmType = NULL;
}

Pointer::Pointer(Type *ptrType)
{
	this->ptrType = ptrType;
	this->llvmType = llvm::PointerType::get(ptrType->llvmType, 0);
}

Pointer::~Pointer()
{
}

std::string Pointer::name() const
{
	std::stringstream result;
	if (ptrType != NULL)
		result << ptrType->name() << "@";
	else
		result << "undefined@";
	return result.str();
}

bool Pointer::eq(Type *other) const
{
	if (other->id != id)
		return false;

	const Pointer *otherPointer = other->get(this);
	return ptrType->eq(otherPointer->ptrType);
}

Struct::Struct()
{
	this->llvmType = NULL;
}

Struct::Struct(std::string typeName)
{
	this->typeName = typeName;
	this->llvmType = llvm::StructType::create(cog.context, typeName);
}

Struct::Struct(std::string typeName, std::vector<Declaration> members)
{
	std::vector<llvm::Type*> memberTypes;
	for (int i = 0; i < (int)members.size(); i++) {
		memberTypes.push_back(members[i].type->llvmType);
	}

	this->typeName = typeName;
	this->members = members;
	this->llvmType = llvm::StructType::create(cog.context, memberTypes, typeName);
}

Struct::~Struct()
{
}

std::string Struct::name() const
{
	return typeName;
}

bool Struct::eq(Type *other) const
{
	if (other->id != id)
		return false;

	const Struct *otherStruct = other->get(this);
	return otherStruct->typeName == typeName;
}

Tuple::Tuple()
{
	this->llvmType = NULL;
}

Tuple::Tuple(std::vector<Declaration> members)
{
	std::vector<llvm::Type*> memberTypes;
	for (int i = 0; i < (int)members.size(); i++) {
		memberTypes.push_back(members[i].type->llvmType);
	}

	this->members = members;
	this->llvmType = llvm::StructType::get(cog.context, memberTypes);
}

Tuple::~Tuple()
{
}

std::string Tuple::name() const
{
	std::stringstream result;
	result << "{";
	for (int i = 0; i < (int)members.size(); i++) {
		if (i != 0)
			result << ",";

		if (members[i].type != NULL)
			result << members[i].type->name();
		else
			result << "undefined";
	}
	result << "}";
	return result.str();
}

bool Tuple::eq(Type *other) const
{
	if (other->id != id)
		return false;

	const Tuple *otherTuple = other->get(this);
	if (members.size() != otherTuple->members.size())
		return false;

	for (int i = 0; i < (int)members.size(); i++) {
		if (!members[i].type->eq(otherTuple->members[i].type))
			return false;
	}

	return true;
}

Function::Function()
{
	llvmType = NULL;
	retType = NULL;
	thisType = NULL;
}

Function::Function(Type *retType, Type *thisType, std::string typeName, std::vector<Declaration> args)
{
	std::vector<llvm::Type*> argTypes;
	if (thisType != NULL)
		argTypes.push_back(thisType->llvmType);

	for (int i = 0; i < (int)args.size(); i++) {
		argTypes.push_back(args[i].type->llvmType);
	}

	this->retType = retType;
	this->thisType = thisType;
	this->typeName = typeName;
	this->args = args;
	this->llvmType = llvm::FunctionType::get(retType->llvmType, argTypes, false);
}

Function::~Function()
{
}

std::string Function::name() const
{
	std::stringstream result;
	result << thisType->name() << "::";
	result << typeName << "(";
	for (int i = 0; i < (int)args.size(); i++) {
		if (i != 0)
			result << ",";
		result << args[i].type->name();
	}
	result << ")";
	return result.str();
}

bool Function::eq(Type *other) const
{
	if (other->id != id)
		return false;

	Function *otherFunc = other->get(this);
	if ((thisType == NULL) != (otherFunc->thisType == NULL))
		return false;

	if (thisType != NULL && !thisType->eq(otherFunc->thisType))
		return false;

	if (args.size() != otherFunc->args.size())
		return false;

	for (int i = 0; i < (int)args.size(); i++) {
		if (!args[i].type->eq(otherFunc->args[i].type))
			return false;
	}

	return true;
}

}

