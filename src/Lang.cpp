#include "Lang.h"
#include "Compiler.h"
#include "Parser.y.h"

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <llvm/ADT/Twine.h>

extern Cog::Compiler cog;
extern int line;
extern int column;
extern const char *str;

using std::endl;

namespace Cog
{

std::ostream &error_(const char *dfile, int dline)
{
	std::cout << str << endl;
	std::cout << dfile << ":" << dline << " error " << line << ":" << column << ": ";
	return std::cout;
}

#define error() error_(__FILE__, __LINE__)

APInt APInt_pow(APInt base, unsigned int exp)
{
    APInt result(1, 1);
    for (;;)
    {
        if (exp & 1) {
					unsigned int newWidth = result.getBitWidth() + base.getBitWidth();
          result = result.zext(newWidth) * base.zext(newWidth);
					result = result.trunc(result.ceilLogBase2());
				}
        exp >>= 1;
        if (!exp)
            break;
				base = base.zext(2*base.getBitWidth());
        base *= base;
				base = base.trunc(base.ceilLogBase2());
    }

    return result;
}

Info *getTypename(int token, char *txt)
{
	// TODO fixed point integer math set precision
	// TODO pointer types
	// TODO structure types
	// TODO interface types
	Info *result = new Info();
	if (token == VOID_PRIMITIVE) {
		result->type.prim = Type::getVoidTy(cog.context);
		return result;
	} else if (token == BOOL_PRIMITIVE) {
		result->type.prim = Type::getInt1Ty(cog.context);
		result->type.isBool = true;
		return result;
	} else if (token == INT_PRIMITIVE && txt[0] == 'u') {
		unsigned int bitwidth = atoi(txt+4);
		result->type.prim = Type::getIntNTy(cog.context, bitwidth);
		result->type.isUnsigned = true;
		return result;
	} else if (token == INT_PRIMITIVE && txt[0] != 'u') {
		unsigned int bitwidth = atoi(txt+3);
		result->type.prim = Type::getIntNTy(cog.context, bitwidth);
		return result;
	} else if (token == FIXED_PRIMITIVE && txt[0] == 'u') {
		unsigned int bitwidth = atoi(txt+6);
		result->type.prim = Type::getIntNTy(cog.context, bitwidth);
		result->type.isUnsigned = true;
		result->type.fpExp = bitwidth/4;
		return result;
	} else if (token == FIXED_PRIMITIVE && txt[0] != 'u') {
		unsigned int bitwidth = atoi(txt+5);
		result->type.prim = Type::getIntNTy(cog.context, bitwidth);
		result->type.fpExp = bitwidth/4;
		return result;
	} else if (token == FLOAT_PRIMITIVE) {
		unsigned int bitwidth = atof(txt+5);
		switch (bitwidth) {
		case 16:
			result->type.prim = Type::getHalfTy(cog.context);
			return result;
		case 32:
			result->type.prim = Type::getFloatTy(cog.context);
			return result;
		case 64:
			result->type.prim = Type::getDoubleTy(cog.context);
			return result;
		//case 80:
		//	result->type.prim = Type::getFP80Ty(cog.context);
		//	return result;
		case 128:
			result->type.prim = Type::getFP128Ty(cog.context);
			return result;
		default:
			error() << "floating point types must be 16, 32, 64, or 128 bits." << endl;
		}
	} else if (token == IDENTIFIER) {
		
	} else {
		error() << "undefined type." << endl;
	}

	delete result;
	delete txt;
	return NULL;
}

Info *getConstant(int64_t cnst)
{
	Info *result = new Info();
	
	APInt value(64, (uint64_t)cnst, cnst < 0);
	int valueShift = value.countTrailingZeros();
	value = value.lshr(valueShift);
	value = value.trunc(value.ceilLogBase2());
	int exponent = valueShift;

	result->value = ConstantInt::get(Type::getIntNTy(cog.context, value.getBitWidth()), value);
	result->type.prim = result->value->getType();
	result->type.fpExp = exponent;
	result->type.isUnsigned = cnst >= 0;
	
	return result;
}

Info *getConstant(int token, char *txt)
{
	Info *result = new Info();
	
	char *curr = txt;
	bool isFractional = false;
	bool isNegative = false;
	
	std::string valueStr = "";
	unsigned int radix = 10;
	int exponent = 0;

	if (*curr == '-') {
		isNegative = true;
		++curr;
	}
	
	switch (token) {
	case BOOL_CONSTANT:
		if (*curr == 't' || *curr == 'T' || *curr == 'y' || *curr == 'Y' || *curr == '1')
			result->value = ConstantInt::get(Type::getInt1Ty(cog.context), 1);
		else
			result->value = ConstantInt::get(Type::getInt1Ty(cog.context), 0);
		result->type.prim = result->value->getType();
		result->type.isBool = true;
		break;
	case HEX_CONSTANT:
		radix = 16;
		curr += 2;
		valueStr = curr;
		break;
	case BIN_CONSTANT:
		radix = 2;
		curr += 2;
		valueStr = curr;
		break;
	case DEC_CONSTANT:
		while ((*curr >= '0' && *curr <= '9') || *curr == '.') {
			if (*curr == '.')
				isFractional = true;
			else {
				exponent -= isFractional;
				valueStr.push_back(*curr);
			}

			++curr;
		}

		if (*curr == 'e' || *curr == 'E')
			exponent += atoi(curr+1);
		break;
	default:
		error() << "unrecognized constant token." << endl;
		delete txt;
		return result;
	}
	
	int bitwidth = APInt::getBitsNeeded(valueStr, radix);
	APInt value(bitwidth, valueStr, radix);
	int valueShift = -value.countTrailingZeros();
	// this assumes that radix > 0
	int radixShift = __builtin_ctz(radix);

	if (exponent >= 0) {
		radix = radix >> radixShift;
		APInt valueCoeff(32 - __builtin_clz(radix), radix);
		valueCoeff = APInt_pow(valueCoeff, exponent);
		value = value.lshr(-valueShift);
		value = value.zextOrTrunc(value.getBitWidth() + valueShift + valueCoeff.getBitWidth());
		valueCoeff = valueCoeff.zext(value.getBitWidth());
		value *= valueCoeff;
	} else {
		radix = radix >> radixShift;
		APInt valueCoeff(32 - __builtin_clz(radix), radix);
		valueCoeff = APInt_pow(valueCoeff, -exponent);
		valueShift += valueCoeff.getBitWidth();
		if (valueShift < 0) {
			value = value.lshr(-valueShift);
			value = value.trunc(value.getBitWidth() + valueShift);
		} else if (valueShift > 0) {
			value = value.zext(value.getBitWidth() + valueShift);
			value = value.shl(valueShift);
		}

		value = value.udiv(valueCoeff.zext(value.getBitWidth()));
		value = value.trunc(value.ceilLogBase2());
	}

	radix = 2;
	exponent -= valueShift;

	valueShift = value.countTrailingZeros();
	value = value.lshr(valueShift);
	value = value.trunc(value.getBitWidth() - valueShift);
	if (isNegative)
		value = -value;
	exponent += valueShift;

	result->value = ConstantInt::get(Type::getIntNTy(cog.context, value.getBitWidth()), value);
	result->type.prim = result->value->getType();
	result->type.fpExp = exponent;
	result->type.isUnsigned = !isNegative;

	printf("%s: %s*2^%d width: %d\n", result->type.getName().c_str(), value.toString(10, false).c_str(), result->type.fpExp, value.getBitWidth());
	
	delete txt;
	return result;
}

Info *getIdentifier(char *txt)
{
	Symbol *symbol = cog.getScope()->findSymbol(txt);
	if (symbol) {
		Info *result = new Info();
		result->symbol = symbol;
		result->value = symbol->getValue();
		result->type = result->symbol->type;
		delete txt;
		return result;
	}

	error() << "undefined variable '" << txt << "'." << endl;
	delete txt;
	return NULL;
}

Info *castType(Info *from, Info *to)
{
	llvm::Type *ft = from->type.prim;
	llvm::Type *tt = to->type.prim;

	if (ft == NULL || ft->isVoidTy() || ft->isLabelTy()
	 || ft->isMetadataTy() || ft->isTokenTy()) {
		error() << "found non-valid type '" << from->type.getName() << "'." << endl;
	} else if (tt == NULL || tt->isVoidTy() || tt->isLabelTy()
	 || tt->isMetadataTy() || tt->isTokenTy()) {
		error() << "found non-valid type '" << from->type.getName() << "'." << endl;
	} else if (ft != tt && (
	    ft->isStructTy()		|| tt->isStructTy()
	 || ft->isFunctionTy()	|| tt->isFunctionTy()
	 || ft->isArrayTy()			|| tt->isArrayTy()
	 || ft->isPointerTy()		|| tt->isPointerTy()
	 || ft->isVectorTy()		|| tt->isVectorTy())) {
		error() << "typecast '" << from->type.getName() << "' to '" << to->type.getName() << "' not yet supported." << endl;
	} else if (ft->isFloatingPointTy() && tt->isFloatingPointTy()) {
		if (ft->getPrimitiveSizeInBits() < tt->getPrimitiveSizeInBits()) {
			from->value = cog.builder.CreateFPExt(from->value, tt);
			from->type = to->type;
			from->symbol = NULL;
		} else if (ft->getPrimitiveSizeInBits() > tt->getPrimitiveSizeInBits()) {
			from->value = cog.builder.CreateFPTrunc(from->value, tt);
			from->type = to->type;
			from->symbol = NULL;
		}
	} else if (ft->isIntegerTy() && tt->isFloatingPointTy()) {
		if (from->type.isBool) {
			from->value = cog.builder.CreateSelect(from->value, ConstantFP::get(tt, 0.0), ConstantFP::getNaN(tt));
		} else {
			if (from->type.isUnsigned)
				from->value = cog.builder.CreateUIToFP(from->value, tt);
			else
				from->value = cog.builder.CreateSIToFP(from->value, tt);

			if (from->type.fpExp != 0)
				from->value = cog.builder.CreateFMul(from->value, ConstantFP::get(tt, pow(2.0, (double)from->type.fpExp)));
		}

		from->type = to->type;
		from->symbol = NULL;
	} else if (ft->isFloatingPointTy() && tt->isIntegerTy()) {
		if (to->type.isBool) {
			from->value = cog.builder.CreateFCmpORD(from->value, from->value);
		} else {
			if (to->type.fpExp != 0)
				from->value = cog.builder.CreateFMul(from->value, ConstantFP::get(ft, pow(2.0, (double)to->type.fpExp)));

			if (to->type.isUnsigned)
				from->value = cog.builder.CreateFPToUI(from->value, tt);
			else
				from->value = cog.builder.CreateFPToSI(from->value, tt);
		}

		from->type = to->type;
		from->symbol = NULL;
	} else if (ft->isIntegerTy() && tt->isIntegerTy()) {
		if (!from->type.isBool && !to->type.isBool) {
			if (!from->type.isUnsigned && to->type.isUnsigned) {
				llvm::Value *flt0 = cog.builder.CreateICmpSLT(from->value, ConstantInt::get(ft, 0));
				from->value = cog.builder.CreateSelect(flt0, cog.builder.CreateNeg(from->value), from->value);
				from->type.isUnsigned = true;
			}

			if (ft->getIntegerBitWidth() < tt->getIntegerBitWidth()) {
				if (from->type.isUnsigned)
					from->value = cog.builder.CreateZExt(from->value, tt);
				else
					from->value = cog.builder.CreateSExt(from->value, tt);
				ft = tt;
				from->type.prim = tt;
			}

			if (from->type.fpExp > to->type.fpExp) {
				from->value = cog.builder.CreateShl(from->value, from->type.fpExp - to->type.fpExp);
			} else if (from->type.fpExp < to->type.fpExp) {
				if (from->type.isUnsigned)
					from->value = cog.builder.CreateLShr(from->value, to->type.fpExp - from->type.fpExp);	
				else {
					/*
					if (from < 0) then (from >> (toExp-fromExp)) will truncate to -1
					This means that 1e6 + -1e0 = 0
					So we need to round toward zero instead of truncate.
					
					Assuming constants:
					width = sizeof(from)
					shift = (toExp - fromExp)
					mask = ((1 << shift)-1)

					Two possible implementations:
					from = (((from >> width) & mask) + from) >> shift
					from = (from < 0 ? from + mask : from) >> shift

					Both are four instructions, but the second is more parallel:
					ashr -> and -> add -> ashr
					(cmplz, add) -> cmov -> ashr 
					*/
					int shift = (to->type.fpExp - from->type.fpExp);
					int mask = ((1 << shift)-1);

					llvm::Value *flt0 = cog.builder.CreateICmpSLT(from->value, ConstantInt::get(ft, 0));
					llvm::Value *add = cog.builder.CreateAdd(from->value, ConstantInt::get(ft, mask));
					from->value = cog.builder.CreateSelect(flt0, add, from->value);
					from->value = cog.builder.CreateAShr(from->value, shift);
				}
			}
			from->type.fpExp = to->type.fpExp;

			if (ft->getIntegerBitWidth() > tt->getIntegerBitWidth()) {
				from->value = cog.builder.CreateTrunc(from->value, tt);
				ft = tt;
				from->type.prim = tt;
			}

			from->type.isUnsigned = to->type.isUnsigned;
		}
	}

	return from;
}

bool canImplicitCast(Info *from, Info *to)
{
	return (
	  (
	    to->type.isBool &&
	    from->type.prim->isFloatingPointTy()
	  ) || (
	    to->type.prim->isFloatingPointTy() && 
	    (
	      (from->type.prim->isFloatingPointTy() &&
	       to->type.prim->getPrimitiveSizeInBits() >
	       from->type.prim->getPrimitiveSizeInBits()
	      ) ||
	      from->type.prim->isIntegerTy()
	    )
	  ) || (
	    to->type.prim->isIntegerTy() &&
	    from->type.prim->isIntegerTy() &&
	    from->type.prim->getIntegerBitWidth() < to->type.prim->getIntegerBitWidth() &&
	    (
	      from->type.isUnsigned == to->type.isUnsigned ||
	      (from->type.isUnsigned && !to->type.isUnsigned)
	    )
		)
	);
}

void unaryTypecheck(Info *arg, Info *expect)
{
	llvm::Type *at = arg->type.prim;
	if (at == NULL || at->isVoidTy() || at->isLabelTy()
	 || at->isMetadataTy() || at->isTokenTy()) {
		error() << "foun non-valid type '" << arg->type.getName() << "'." << endl;
	} else if (at != expect->type.prim && (
	    at->isStructTy()
	 || at->isFunctionTy()
	 || at->isArrayTy()
	 || at->isPointerTy()
	 || at->isVectorTy())) {
		error() << "type promotion '" << arg->type.getName() << "' to '" << expect->type.getName() << "' not yet supported." << endl;
	} else {
		if (canImplicitCast(arg, expect))
			castType(arg, expect);
		else if (arg->type != expect->type)
			error() << "unable to cast '" << arg->type.getName() << "' to '" << expect->type.getName() << "'." << endl;
	}
}

void binaryTypecheck(Info *left, Info *right, Info *expect)
{
	llvm::Type *lt = left->type.prim;
	llvm::Type *rt = right->type.prim;

	if (lt == NULL || lt->isVoidTy() || lt->isLabelTy()
	 || lt->isMetadataTy() || lt->isTokenTy()) {
		error() << "found non-valid type '" << left->type.getName() << "'." << endl;
	} else if (rt == NULL || rt->isVoidTy() || rt->isLabelTy()
	 || rt->isMetadataTy() || rt->isTokenTy()) {
		error() << "found non-valid type '" << right->type.getName() << "'." << endl;
	} else if (lt != rt && (
	    lt->isStructTy()		|| rt->isStructTy()
	 || lt->isFunctionTy()	|| rt->isFunctionTy()
	 || lt->isArrayTy()			|| rt->isArrayTy()
	 || lt->isPointerTy()		|| rt->isPointerTy()
	 || lt->isVectorTy()		|| rt->isVectorTy())) {
		error() << "type promotion '" << left->type.getName() << "', '" << right->type.getName() << "' not yet supported." << endl;
	} else if (expect == NULL) {
		if (canImplicitCast(right, left)) {
			castType(right, left);
		} else if (canImplicitCast(left, right)) {
			castType(left, right);
		}

		if (left->type != right->type)
			error() << "type mismatch '" << left->type.getName() << "' != '" << right->type.getName() << "'." << endl;
	} else {
		if (canImplicitCast(left, expect))
			castType(left, expect);
		else
			error() << "unable to cast '" << left->type.getName() << "' to '" << expect->type.getName() << "'." << endl;
	
		if (canImplicitCast(right, expect))
			castType(right, expect);
		else
			error() << "unable to cast '" << right->type.getName() << "' to '" << expect->type.getName() << "'." << endl;
	}
}

Info *unaryOperator(int op, Info *arg)
{
	Info booleanType;
	booleanType.type.prim = Type::getInt1Ty(cog.context);
	booleanType.type.isBool = true;

	if (arg) {
		switch (op) {
			case '-':
				arg->value = cog.builder.CreateNeg(arg->value);
				break;
			case '~':
				arg->value = cog.builder.CreateNot(arg->value);
				break;
			case NOT:
				unaryTypecheck(arg, &booleanType);
				arg->value = cog.builder.CreateNot(arg->value);
				break;
			default:
				error() << "unrecognized unary operator '" << (char)op << "'." << endl;
				delete arg;
				return NULL;
		}
		arg->type.prim = arg->value->getType();
		arg->symbol = NULL;
		return arg;
	} else {
		error() << "here" << endl;
		return NULL;
	}
}

Info *binaryOperator(Info *left, int op, Info *right)
{
	if (left && right) {
		Value *width = NULL;
		Value *inv = NULL;
		Value *lsb = NULL;
		Value *msb = NULL;

		Info booleanType;
		booleanType.type.prim = Type::getInt1Ty(cog.context);
		booleanType.type.isBool = true;

		switch (op) {
		case OR:
			binaryTypecheck(left, right, &booleanType);
			left->value = cog.builder.CreateOr(left->value, right->value);
			break;
		case XOR:
			binaryTypecheck(left, right, &booleanType);
			left->value = cog.builder.CreateXor(left->value, right->value);
			break;
		case AND:
			binaryTypecheck(left, right, &booleanType);
			left->value = cog.builder.CreateAnd(left->value, right->value);
			break;
		case '<':
			binaryTypecheck(left, right);
			if (left->type.prim->isFloatingPointTy())
				left->value = cog.builder.CreateFCmpOLT(left->value, right->value);
			else if (left->type.isUnsigned)
				left->value = cog.builder.CreateICmpULT(left->value, right->value);
			else
				left->value = cog.builder.CreateICmpSLT(left->value, right->value);
			left->type = booleanType.type;
			break;
		case '>':
			binaryTypecheck(left, right);
			if (left->type.prim->isFloatingPointTy())
				left->value = cog.builder.CreateFCmpOGT(left->value, right->value);
			else if (left->type.isUnsigned)
				left->value = cog.builder.CreateICmpUGT(left->value, right->value);
			else 
				left->value = cog.builder.CreateICmpSGT(left->value, right->value);
			left->type = booleanType.type;
			break;
		case LE:
			binaryTypecheck(left, right);
			if (left->type.prim->isFloatingPointTy())
				left->value = cog.builder.CreateFCmpOLE(left->value, right->value);
			else if (left->type.isUnsigned)
				left->value = cog.builder.CreateICmpULE(left->value, right->value);
			else 
				left->value = cog.builder.CreateICmpSLE(left->value, right->value);
			left->type = booleanType.type;
			break;
		case GE:
			binaryTypecheck(left, right);
			if (left->type.prim->isFloatingPointTy())
				left->value = cog.builder.CreateFCmpOGE(left->value, right->value);
			else if (left->type.isUnsigned)
				left->value = cog.builder.CreateICmpUGE(left->value, right->value);
			else 
				left->value = cog.builder.CreateICmpSGE(left->value, right->value);
			left->type = booleanType.type;
			break;
		case EQ:
			binaryTypecheck(left, right);
			if (left->type.prim->isFloatingPointTy())
				left->value = cog.builder.CreateFCmpOEQ(left->value, right->value);
			else 
				left->value = cog.builder.CreateICmpEQ(left->value, right->value);
			left->type = booleanType.type;
			break;
		case NE:
			binaryTypecheck(left, right);
			if (left->type.prim->isFloatingPointTy())
				left->value = cog.builder.CreateFCmpUNE(left->value, right->value);
			else 
				left->value = cog.builder.CreateICmpNE(left->value, right->value);
			left->type = booleanType.type;
			break;
		case '|':
			binaryTypecheck(left, right);
			left->value = cog.builder.CreateOr(left->value, right->value);
			break;
		case '^':
			binaryTypecheck(left, right);
			left->value = cog.builder.CreateXor(left->value, right->value);
			break;
		case '&':
			binaryTypecheck(left, right);
			left->value = cog.builder.CreateAnd(left->value, right->value);
			break;
		case SHL:
			binaryTypecheck(left, right);
			left->value = cog.builder.CreateShl(left->value, right->value);
			break;
		case ASHR:
			binaryTypecheck(left, right);
			left->value = cog.builder.CreateAShr(left->value, right->value);
			break;
		case LSHR:
			binaryTypecheck(left, right);
			left->value = cog.builder.CreateLShr(left->value, right->value);
			break;
		case ROR:
			binaryTypecheck(left, right);
			width = ConstantInt::get(left->type.prim, left->type.prim->getIntegerBitWidth());
			inv = cog.builder.CreateSub(width, right->value);
			lsb = cog.builder.CreateLShr(left->value, right->value);
			msb = cog.builder.CreateShl(left->value, inv);
			left->value = cog.builder.CreateOr(msb, lsb);
			break;
		case ROL:
			binaryTypecheck(left, right);
			width = ConstantInt::get(left->type.prim, left->type.prim->getIntegerBitWidth());
			inv = cog.builder.CreateSub(width, right->value);
			lsb = cog.builder.CreateLShr(left->value, inv);
			msb = cog.builder.CreateShl(left->value, right->value);
			left->value = cog.builder.CreateOr(msb, lsb);
			break;
		case '+':
			binaryTypecheck(left, right);
			if (left->type.prim->isFloatingPointTy())
				left->value = cog.builder.CreateFAdd(left->value, right->value);
			else 
				left->value = cog.builder.CreateAdd(left->value, right->value);
			break;
		case '-':
			binaryTypecheck(left, right);
			if (left->type.prim->isFloatingPointTy())
				left->value = cog.builder.CreateFSub(left->value, right->value);
			else 
				left->value = cog.builder.CreateSub(left->value, right->value);
			break;
		case '*':
			binaryTypecheck(left, right);
			if (left->type.prim->isFloatingPointTy())
				left->value = cog.builder.CreateFMul(left->value, right->value);
			else
				left->value = cog.builder.CreateMul(left->value, right->value);
			break;
		case '/':
			binaryTypecheck(left, right);
			if (left->type.prim->isFloatingPointTy())
				left->value = cog.builder.CreateFDiv(left->value, right->value);
			else if (left->type.isUnsigned)
				left->value = cog.builder.CreateUDiv(left->value, right->value);
			else
				left->value = cog.builder.CreateSDiv(left->value, right->value);
			break;
		case '%':
			binaryTypecheck(left, right);
			if (left->type.prim->isFloatingPointTy())
				left->value = cog.builder.CreateFRem(left->value, right->value);
			else if (left->type.isUnsigned)
				left->value = cog.builder.CreateURem(left->value, right->value);
			else
				left->value = cog.builder.CreateSRem(left->value, right->value);
			break;
		default:
			error() << "unrecognized binary operator '" << (char)op << "'." << endl;
			delete left;
			delete right;
			return NULL;
		}
		left->type.prim = left->value->getType();
		left->symbol = NULL;
		delete right;
		return left;
	} else {
		error() << "here" << endl;
		if (left)
			delete left;
		if (right)
			delete right;
		return NULL;
	}
}

void declareSymbol(Info *type, char *name)
{
	if (type && name) {
		if (cog.getScope()->findSymbol(name) != NULL) {
			error() << "variable '" << name << "' already defined." << endl;
		}

		cog.getScope()->createSymbol(name, type->type);
		delete type;
		delete name;
	} else {
		error() << "here" << endl;
		if (type)
			delete type;
		if (name)
			delete name;
		// something is broken
	}
}

void assignSymbol(Info *symbol, Info *value)
{
	if (symbol && value) {
		unaryTypecheck(value, symbol);
		symbol->symbol->setValue(value->value);
		value->value->setName(symbol->symbol->name + "_");
	} else {
		error() << "here" << endl;
		if (symbol)
			delete symbol;
		if (value)
			delete value;
		// something is broken
	}
}

void structureDefinition(char *name)
{
	std::vector<Type*> members;
	for (int i = 0; i < (int)cog.getScope()->symbols.size(); i++) {
		members.push_back(cog.getScope()->symbols[i].type.prim);
	}

	llvm::StructType::create(cog.context, members, name, false);
	cog.scopes.pop_back();
	cog.scopes.push_back(Scope());
}

void functionPrototype(Info *retType, char *name, Info *typeList)
{
	std::vector<Type*> args;
	Info *curr = typeList;
	while (curr != NULL)	
		args.push_back(curr->type.prim);

  FunctionType *FT = FunctionType::get(retType->type.prim, args, false);
  Function::Create(FT, Function::ExternalLinkage, name, cog.module);
}

void functionDeclaration(Info *retType, char *name)
{
	std::vector<Type*> args;
	args.reserve(cog.getScope()->symbols.size());
	for (int i = 0; i < (int)cog.getScope()->symbols.size(); i++)
		args.push_back(cog.getScope()->symbols[i].type.prim);

  FunctionType *FT = FunctionType::get(retType->type.prim, args, false);
	Function *F = cog.module->getFunction(name);
	if (F == NULL)
  	F = Function::Create(FT, Function::ExternalLinkage, name, cog.module);
	
	int i = 0;
	for (auto &arg : F->args()) {
		arg.setName(cog.getScope()->symbols[i].name);
		cog.getScope()->symbols[i].setValue(&arg);
		++i;
	}

	BasicBlock *body = BasicBlock::Create(cog.context, "entry", F, 0);
	cog.getScope()->blocks.push_back(body);
	cog.getScope()->curr = cog.getScope()->blocks.begin();
	cog.builder.SetInsertPoint(body);
}

void functionDefinition()
{
	cog.scopes.pop_back();
	cog.scopes.push_back(Scope());
}

void returnValue(Info *value)
{
	if (value) {
		// TODO cast to function return type
		// unaryTypecheck(cond, retType);

		cog.builder.CreateRet(value->value);
	} else {
		error() << "here" << endl;
	}
}

void returnVoid()
{
	cog.builder.CreateRetVoid();
}

void callFunction(char *txt, Info *args)
{
	// TODO search for implicit Casts

	std::vector<Value*> argValues;
	Info *curr = args;
	while (curr != NULL) {
		argValues.push_back(curr->value);
		curr = curr->next;
	}

	if (args != NULL)
		delete args;

	cog.builder.CreateCall(cog.module->getFunction(txt), argValues);
}

void ifCondition(Info *cond)
{
	Function *func = cog.builder.GetInsertBlock()->getParent();
	
	BasicBlock *thenBlock = BasicBlock::Create(cog.context, "then", func);
	BasicBlock *elseBlock = BasicBlock::Create(cog.context, "else", func);
	cog.getScope()->appendBlock(thenBlock);
	cog.getScope()->appendBlock(elseBlock);

	Info booleanType;
	booleanType.type.prim = Type::getInt1Ty(cog.context);
	booleanType.type.isBool = true;

	unaryTypecheck(cond, &booleanType);

	cog.builder.CreateCondBr(cond->value, thenBlock, elseBlock);
	cog.getScope()->popBlock();
	cog.pushScope();
	cog.builder.SetInsertPoint(cog.getScope()->getBlock());
}

void elseifCondition()
{
	cog.popScope();
	cog.getScope()->nextBlock();	
	cog.builder.SetInsertPoint(cog.getScope()->getBlock());
}

void elseCondition()
{
	cog.popScope();
	cog.getScope()->nextBlock();
	cog.pushScope();
	cog.builder.SetInsertPoint(cog.getScope()->getBlock());
}

void ifStatement()
{
	Function *func = cog.builder.GetInsertBlock()->getParent();
	
	cog.popScope();
	Scope *scope = cog.getScope();
	// if this block has an else statement we can't use the remaining else block
	if (std::next(scope->curr) == scope->blocks.end()) {
		BasicBlock *mergeBlock = BasicBlock::Create(cog.context, "merge", func);
		scope->appendBlock(mergeBlock);
	}
	
	scope->nextBlock();
	cog.builder.SetInsertPoint(scope->getBlock());

	std::vector<llvm::PHINode*> phi;
	phi.reserve(scope->symbols.size());
	for (int i = 0; i < (int)scope->symbols.size(); i++) {
		phi.push_back(cog.builder.CreatePHI(scope->symbols[i].type.prim, scope->blocks.size()-1));
		phi.back()->setName(scope->symbols[i].name + "_");
		scope->symbols[i].values.back() = phi.back();
	}

	while (scope->blocks.size() > 1) {
		cog.builder.SetInsertPoint(scope->blocks.front());
		cog.builder.CreateBr(scope->getBlock());
		for (int i = 0; i < (int)scope->symbols.size(); i++)
			phi[i]->addIncoming(scope->symbols[i].values.front(), scope->blocks.front());
		scope->dropBlock();
	}
	cog.builder.SetInsertPoint(scope->getBlock());
}

void whileKeyword()
{
	Function *func = cog.builder.GetInsertBlock()->getParent();
	BasicBlock *fromBlock = cog.getScope()->getBlock();
	
	BasicBlock *condBlock = BasicBlock::Create(cog.context, "cond", func);
	cog.getScope()->appendBlock(condBlock);
	
	cog.builder.CreateBr(condBlock);
	cog.getScope()->popBlock();
	cog.builder.SetInsertPoint(cog.getScope()->getBlock());

	Scope *scope = cog.getScope();
	for (int i = 0; i < (int)scope->symbols.size(); i++) {
		PHINode *value = cog.builder.CreatePHI(scope->symbols[i].type.prim, scope->blocks.size()-1);
		value->addIncoming(scope->symbols[i].getValue(), fromBlock);
		value->setName(scope->symbols[i].name + "_");
		scope->symbols[i].setValue(value);
	}
}

void whileCondition(Info *cond)
{
	Function *func = cog.builder.GetInsertBlock()->getParent();
	
	BasicBlock *thenBlock = BasicBlock::Create(cog.context, "then", func);
	BasicBlock *elseBlock = BasicBlock::Create(cog.context, "else", func);
	cog.getScope()->appendBlock(thenBlock);
	cog.getScope()->appendBlock(elseBlock);

	Info booleanType;
	booleanType.type.prim = Type::getInt1Ty(cog.context);
	booleanType.type.isBool = true;

	unaryTypecheck(cond, &booleanType);

	cog.builder.CreateCondBr(cond->value, thenBlock, elseBlock);
	cog.getScope()->nextBlock();
	cog.pushScope();
	cog.builder.SetInsertPoint(cog.getScope()->getBlock());
}

void whileStatement()
{
	Scope *prev = &cog.scopes[cog.scopes.size()-2];
	Scope *curr = cog.getScope();
	for (int i = 0; i < (int)prev->symbols.size(); i++) {
		((PHINode*)prev->symbols[i].getValue())->addIncoming(curr->symbols[i].getValue(), curr->getBlock());
		curr->symbols[i].setValue(prev->symbols[i].getValue());
	}

	cog.popScope();
	cog.builder.CreateBr(cog.getScope()->blocks.front());
	cog.getScope()->nextBlock();
	cog.builder.SetInsertPoint(cog.getScope()->getBlock());

	while (cog.getScope()->blocks.size() > 1)
		cog.getScope()->dropBlock();
}

Info *infoList(Info *lst, Info *elem)
{
	if (lst) {
		Info *curr = lst;
		while (curr->next != NULL)
			curr = curr->next;

		curr->next = elem;
		return lst;
	} else
		return elem;
}

Info *asmRegister(char *txt)
{
	Info *result = new Info();
	result->text = txt;
	delete txt;
	return result;
}

Info *asmConstant(char *txt)
{
	Info *result = new Info();
	if (txt[0] == '$')
		result->text = std::string("$") + txt;
	else
		result->text = txt;
	delete txt;
	return result;
}

Info *asmFunction(char *txt, Info *args)
{
	Info *result = new Info();
	result->text = txt;
	result->next = args;
	delete txt;
	return result;
}

void asmFunctionDefinition()
{
	returnVoid();
	cog.scopes.pop_back();
	cog.scopes.push_back(Scope());
}

Info *asmMemoryArg(char *seg, char *cnst, Info *args)
{
	Info *result = new Info();
	if (seg) {
		result->text += seg;
		delete seg;
	}

	if (cnst) {
		if (result->text.size() > 0)
			result->text += ":";
		result->text += cnst;
		delete cnst;
	}

	result->args = args;
	return result;
}

Info *asmAddress(Info *base, Info *offset, char *scale)
{
	Info *result = NULL;
	if (scale) {
		result = new Info();
		result->text = scale;
		delete scale;
	}

	if (offset) {
		offset->next = result;
		result = offset;
	} else if (result) {
		offset = new Info();
		offset->next = result;
		result = offset;
	}

	if (base) {
		base->next = result;
		result = base;
	} else if (result) {
		base = new Info();
		base->next = result;
		result = base;
	}

	return result;
}

Info *asmInstruction(char *instr, Info *args)
{
	Info *result = new Info();
	result->text = instr;
	result->args = args;
	delete instr;
	return result;
}

Info *asmStatement(char *label, Info *instr)
{
	Info *result = instr;
	if (label) {
		result = new Info();
		result->args = instr;
		result->text = label;
		delete label;
	}
	return result;
}

void asmBlock(Info *stmts)
{
	std::vector<Symbol*> outs;
	std::vector<Type*> outTypes;
	std::vector<Type*> inTypes;
	std::vector<Value*> inValues;
	std::string outCnsts;
	std::string inCnsts;

	// figure out constraints and register mapping
	for (Info *stmt = stmts; stmt; stmt = stmt->next) {
		// strip labels
		Info *instr = NULL;
		if (stmt->text.size() > 0 && stmt->text.back() == ':')
			instr = stmt->args;
		else
			instr = stmt;

		for (Info *arg = instr->args; arg; arg = arg->next) {
			// handle memory arguments
			for (Info *addr = arg->args; addr; addr = addr->next) {
				if (addr->symbol) {
					if (arg->next) {
						if (inCnsts.size() > 0)
							inCnsts += ",";
						inCnsts += "r";
						inTypes.push_back(addr->type.prim);
						inValues.push_back(addr->value);
					} else {
						if (outCnsts.size() > 0)
							outCnsts += ",";
						outCnsts += "=r";
						outs.push_back(addr->symbol);
						outTypes.push_back(addr->type.prim);
					}
				}
			}

			if (arg->symbol) {
				if (arg->next) {
					if (inCnsts.size() > 0)
						inCnsts += ",";
					inCnsts += "r";
					inTypes.push_back(arg->type.prim);
					inValues.push_back(arg->value);
				} else {
					if (outCnsts.size() > 0)
						outCnsts += ",";
					outCnsts += "=r";
					outs.push_back(arg->symbol);
					outTypes.push_back(arg->type.prim);
				}
			}
		}
	}

	// build the assembly string
	std::stringstream assembly;
	for (Info *stmt = stmts; stmt; stmt = stmt->next) {
		if (stmt != stmts)
			assembly << ";\n";

		// strip labels
		Info *instr = NULL;
		if (stmt->text.size() > 0 && stmt->text.back() == ':') {
			assembly << stmt->text << " ";
			instr = stmt->args;
		} else
			instr = stmt;

		assembly << instr->text;
		for (Info *arg = instr->args; arg; arg = arg->next) {
			if (arg != instr->args)
				assembly << ", ";

			// handle memory arguments
			if (arg->args) {
				assembly << arg->text << "(";
				for (Info *addr = arg->args; addr; addr = addr->next) {
					if (addr != arg->args)
						assembly << ",";

					if (addr->symbol) {
						bool found = false;
						for (int i = 0; !found && i < (int)outs.size(); i++)
							if (addr->symbol == outs[i]) {
								assembly << "$" << i;
								found = true;
							}
						for (int i = 0; !found && i < (int)inValues.size(); i++)
							if (addr->value == inValues[i]) {
								assembly << "$" << ((int)outs.size() + i);
								found = true;
							}

						if (!found)
							error() << "This shouldn't happen..." << endl;
					} else {
						assembly << addr->text;
					}
				}
				assembly << ")";
			} else if (arg->symbol) {
				bool found = false;
				for (int i = 0; !found && i < (int)outs.size(); i++)
					if (arg->symbol == outs[i]) {
						assembly << "$" << i;
						found = true;
					}
				for (int i = 0; !found && i < (int)inValues.size(); i++)
					if (arg->value == inValues[i]) {
						assembly << "$" << ((int)outs.size() + i);
						found = true;
					}

				if (!found)
					error() << "This shouldn't happen..." << endl;	
			} else {
				assembly << arg->text;
			}
		}
	}

	std::string constraints;
	constraints += outCnsts;
	if (inCnsts.size() > 0 && outCnsts.size() > 0)
		constraints += ",";
	constraints += inCnsts;

	Type *retType = Type::getVoidTy(cog.context);
	if (outTypes.size() > 0)
		retType = outTypes[0];

	FunctionType *returnType = FunctionType::get(retType, inTypes, false);	
	InlineAsm *asmIns = InlineAsm::get(returnType, assembly.str(), constraints, false);
	Value *ret = cog.builder.CreateCall(asmIns, inValues);

	for (int i = 0; i < (int)outs.size(); i++)
		outs[i]->setValue(ret);

	delete stmts;
}

}

