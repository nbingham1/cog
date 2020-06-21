#include "Type.h"
#include "Symbol.h"
#include "Compiler.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>

#include <sstream>

extern Cog::Compiler compile;

namespace Cog
{

Type::Table Type::table;

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

Type *Type::define(Type *t)
{
	std::pair<Type::Table::iterator, bool> result = Type::table.insert(std::pair<std::string, Type*>(t->name(), t));
	return result.first->second;
}

Type *Type::find(std::string name)
{
	Type::Table::iterator result = table.find(name);
	if (result != table.end()) {
		return result->second;
	// Check base types before giving up
	} else if (name == "void") {
		return define(new Void());
	} else if (name == "bool") {
		return define(new Boolean());
	} else {
		// set default values
		bool isSigned = true;
		int bitwidth = 0;
		int exponent = 0;
		bool isFloat = false;

		int idx = 0;

		// check unsigned or not
		if (1 <= (int)name.size() && name[idx] == 'u') {
			isSigned = false;
			++idx;
		}
		
		// parse keyword (int/fixed/float)
		if (idx+3 <= (int)name.size() && strncmp(name.c_str()+idx, "int", 3) == 0) {
			idx += 3;
		} else if (idx+5 <= (int)name.size() && strncmp(name.c_str()+idx, "fixed", 5) == 0) {
			idx += 5;
		} else if (idx+5 <= (int)name.size() && strncmp(name.c_str()+idx, "float", 5) == 0) {
			isFloat = true;
			idx += 5;
		} else {
			return NULL;
		}

		// parse bitwidth
		while (idx < (int)name.size() &&
					 name[idx] >= '0' && name[idx] <= '9') {
			bitwidth = bitwidth*10 + name[idx] - '0';
			++idx;
		}

		// parse exponent
		if (!isFloat && idx < (int)name.size() && name[idx] == 'e') {
			++idx;
			bool negative = false;
			if (idx < (int)name.size() && name[idx] == '-') {
				negative = true;
				++idx;
			}

			while (idx < (int)name.size() &&
						 name[idx] >= '0' && name[idx] <= '9') {
				exponent = exponent*10 + name[idx] - '0';
				++idx;
			}

			if (negative)
				exponent = -exponent;
		}

		// check no dangling characters
		if (idx < (int)name.size()) {
			return NULL;
		} else if (isFloat) {
			return define(new Float(bitwidth));
		} else {
			return define(new Fixed(isSigned, bitwidth, exponent));
		}
	}
}

Void::Void()
{
	llvmType = llvm::Type::getVoidTy(compile.context);
}

Void::~Void()
{
}

std::string Void::name() const
{
	return "void";
}

int Void::id() const
{
	return typeId;
}

bool Void::eq(Type *other) const
{
	return other->id() == typeId;
}

Boolean::Boolean()
{
	llvmType = llvm::Type::getInt1Ty(compile.context);
}

Boolean::~Boolean()
{
}

std::string Boolean::name() const
{
	return "bool";
}

int Boolean::id() const
{
	return typeId;
}

bool Boolean::eq(Type *other) const
{
	return other->id() == typeId;
}

Fixed::Fixed(bool isSigned, int bitwidth, int exponent)
{
	this->isSigned = isSigned;
	this->bitwidth = bitwidth;
	this->exponent = exponent;
	this->llvmType = llvm::Type::getIntNTy(compile.context, bitwidth);
}

Fixed::~Fixed()
{
}

std::string Fixed::name() const
{
	std::stringstream result;
	if (isSigned)
		result << "u";

	if (exponent == 0)
		result << "int" << bitwidth;
	else
		result << "fixed" << bitwidth << "e" << exponent;
	return result.str();
}

int Fixed::id() const
{
	return typeId;
}

bool Fixed::eq(Type *other) const
{
	if (other->id() != typeId)
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
		this->llvmType = llvm::Type::getHalfTy(compile.context);
		break;
	case 32:
		this->sigwidth = 24;
		this->expmin = -127;
		this->expmax = 128;
		this->llvmType = llvm::Type::getFloatTy(compile.context);
		break;
	case 64:
		this->sigwidth = 53;
		this->expmin = -1023;
		this->expmax = 1024;
		this->llvmType = llvm::Type::getDoubleTy(compile.context);
		break;
	case 80:
		this->sigwidth = 64;
		this->expmin = -16383;
		this->expmax = 16384;
		this->llvmType = llvm::Type::getX86_FP80Ty(compile.context);
		break;
	case 128:
		this->sigwidth = 113;
		this->expmin = -16383;
		this->expmax = 16384;
		this->llvmType = llvm::Type::getFP128Ty(compile.context);
		break;
	default:
		this->llvmType = NULL;
		this->bitwidth = 0;
		this->sigwidth = 0;
		this->expmin = 0;
		this->expmax = 0;
		//std::cout << (parse::fail(lexer, token) << "floating point types must be 16, 32, 64, or 128 bits.");
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

int Float::id() const
{
	return typeId;
}

bool Float::eq(Type *other) const
{
	if (other->id() != typeId)
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

int StaticArray::id() const
{
	return typeId;
}

bool StaticArray::eq(Type *other) const
{
	if (other->id() != typeId)
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
		result << ptrType->name() << "#";
	else
		result << "undefined#";
	return result.str();
}

int Pointer::id() const
{
	return typeId;
}

bool Pointer::eq(Type *other) const
{
	if (other->id() != typeId)
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
	this->llvmType = llvm::StructType::create(compile.context, typeName);
}

Struct::Struct(std::string typeName, std::vector<Symbol> members)
{
	std::vector<llvm::Type*> memberTypes;
	for (int i = 0; i < (int)members.size(); i++) {
		memberTypes.push_back(members[i].type->llvmType);
	}

	this->typeName = typeName;
	this->members = members;
	this->llvmType = llvm::StructType::create(compile.context, memberTypes, typeName);
}

Struct::~Struct()
{
}

std::string Struct::name() const
{
	return typeName;
}

int Struct::id() const
{
	return typeId;
}

bool Struct::eq(Type *other) const
{
	if (other->id() != typeId)
		return false;

	const Struct *otherStruct = other->get(this);
	return otherStruct->typeName == typeName;
}

Tuple::Tuple()
{
	this->llvmType = NULL;
}

Tuple::Tuple(std::vector<Type*> members)
{
	std::vector<llvm::Type*> memberTypes;
	for (int i = 0; i < (int)members.size(); i++) {
		memberTypes.push_back(members[i]->llvmType);
	}

	this->members = members;
	this->llvmType = llvm::StructType::get(compile.context, memberTypes);
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

		if (members[i] != NULL)
			result << members[i]->name();
		else
			result << "undefined";
	}
	result << "}";
	return result.str();
}

int Tuple::id() const
{
	return typeId;
}

bool Tuple::eq(Type *other) const
{
	if (other->id() != typeId)
		return false;

	const Tuple *otherTuple = other->get(this);
	if (members.size() != otherTuple->members.size())
		return false;

	for (int i = 0; i < (int)members.size(); i++) {
		if (!members[i]->eq(otherTuple->members[i]))
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

Function::Function(Type *retType, Type *thisType, std::vector<Symbol*> args)
{
	std::vector<llvm::Type*> argTypes;
	if (thisType != NULL)
		argTypes.push_back(thisType->llvmType);

	for (auto i = args.begin(); i != args.end(); i++) {
		argTypes.push_back((*i)->type->llvmType);
		this->args.push_back(**i);
	}

	this->retType = retType;
	this->thisType = thisType;
	this->llvmType = llvm::FunctionType::get(retType->llvmType, argTypes, false);
}

Function::~Function()
{
}

std::string Function::name() const
{
	std::stringstream result;
	result << thisType->name() << "::";
	result << "(";
	for (int i = 0; i < (int)args.size(); i++) {
		if (i != 0)
			result << ",";
		result << args[i].type->name();
	}
	result << ")";
	return result.str();
}

int Function::id() const
{
	return typeId;
}

bool Function::eq(Type *other) const
{
	if (other->id() != typeId)
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

