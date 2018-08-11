#include "Type.h"
#include "Compiler.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

#include <sstream>

extern Compiler cog;

namespace Cog
{

Type::Type()
{
	llvmType = NULL;
}

Type::~Type()
{
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
		result << "int";
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
		this->llvmType = llvm::Type::getFP80Ty(cog.context);
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
	this->arrType = NULL;
	this->size = -1;
}

StaticArray::StaticArray(Type *arrType, int size)
{
	this->arrType = arrType;
	this->size = size;
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
}

Pointer::Pointer(Type *ptrType)
{
	this->ptrType = ptrType;
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
}

Struct::Struct(std::string typeName)
{
	this->typeName = typeName;
}

Struct::~Struct()
{
}

std::string Struct::name() const
{
	if (typeName.size() > 0)
		return typeName;
	else {
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
}

bool Struct::eq(Type *other) const
{
	if (other->id != id)
		return false;

	const Struct *otherStruct = other->get(this);
	if (members.size() != otherStruct->members.size())
		return false;

	for (int i = 0; i < (int)members.size(); i++) {
		if (!members[i].type->eq(otherStruct->members[i].type))
			return false;
	}

	return true;
}

Function::Function()
{
	retType = NULL;
	recvType = NULL;
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
	if ((retType == NULL) != (otherFunc->retType == NULL) ||
	    (thisType == NULL) != (otherFunc->thisType == NULL))
		return false;

	if (retType != NULL && !retType->eq(otherFunc->retType))
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

