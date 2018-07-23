#include "Lang.h"
#include "Compiler.h"
#include "Intrinsic.h"
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
		result->type.setPrimType(Type::getVoidTy(cog.context), PrimType::Void); 
		return result;
	} else if (token == BOOL_PRIMITIVE) {
		result->type.setPrimType(Type::getInt1Ty(cog.context), PrimType::Boolean);
		return result;
	} else if (token == INT_PRIMITIVE && txt[0] == 'u') {
		unsigned int bitwidth = atoi(txt+4);
		result->type.setPrimType(Type::getIntNTy(cog.context, bitwidth), PrimType::Unsigned, bitwidth);
		return result;
	} else if (token == INT_PRIMITIVE && txt[0] != 'u') {
		unsigned int bitwidth = atoi(txt+3);
		result->type.setPrimType(Type::getIntNTy(cog.context, bitwidth), PrimType::Signed, bitwidth);
		return result;
	} else if (token == FIXED_PRIMITIVE && txt[0] == 'u') {
		unsigned int bitwidth = atoi(txt+6);
		result->type.setPrimType(Type::getIntNTy(cog.context, bitwidth), PrimType::Unsigned, bitwidth, bitwidth/4);
		return result;
	} else if (token == FIXED_PRIMITIVE && txt[0] != 'u') {
		unsigned int bitwidth = atoi(txt+5);
		result->type.setPrimType(Type::getIntNTy(cog.context, bitwidth), PrimType::Signed, bitwidth, bitwidth/4);;
		return result;
	} else if (token == FLOAT_PRIMITIVE) {
		unsigned int bitwidth = atof(txt+5);
		switch (bitwidth) {
		case 16:
			result->type.setPrimType(Type::getHalfTy(cog.context), PrimType::Float, 11);
			return result;
		case 32:
			result->type.setPrimType(Type::getFloatTy(cog.context), PrimType::Float, 24);
			return result;
		case 64:
			result->type.setPrimType(Type::getDoubleTy(cog.context), PrimType::Float, 53);
			return result;
		//case 80:
		//	result->type.setPrimType(Type::getFP80Ty(cog.context), PrimType::Float, 64);
		//	return result;
		case 128:
			result->type.setPrimType(Type::getFP128Ty(cog.context), PrimType::Float, 113);
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
	result->type.setPrimType(
		result->value->getType(),
		cnst < 0 ? PrimType::Signed : PrimType::Unsigned,
		value.getBitWidth(),
		exponent);
	
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
		result->type.setPrimType(result->value->getType(), PrimType::Boolean);
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
	result->type.setPrimType(
		result->value->getType(),
		isNegative ? PrimType::Signed : PrimType::Unsigned,
		value.getBitWidth(),
		exponent);

	//printf("%s: %s\n", result->type.getName().c_str(), value.toString(10, false).c_str());
	
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

llvm::Value *castType(llvm::Value* value, Typename from, Typename to)
{
	llvm::Type *ft = from.getLlvm();
	llvm::Type *tt = to.getLlvm();

	if (ft == NULL || ft->isLabelTy()
	 || ft->isMetadataTy() || ft->isTokenTy()) {
		error() << "found non-valid type '" << from.getName() << "'." << endl;
	} else if (tt == NULL || tt->isLabelTy()
	 || tt->isMetadataTy() || tt->isTokenTy()) {
		error() << "found non-valid type '" << from.getName() << "'." << endl;
	} else if (ft != tt && (
	    ft->isStructTy()		|| tt->isStructTy()
	 || ft->isFunctionTy()	|| tt->isFunctionTy()
	 || ft->isArrayTy()			|| tt->isArrayTy()
	 || ft->isPointerTy()		|| tt->isPointerTy()
	 || ft->isVectorTy()		|| tt->isVectorTy())) {
		error() << "typecast '" << from.getName() << "' to '" << to.getName() << "' not yet supported." << endl;
	} else if (from.prim && to.prim) {
		PrimType *fp = from.prim;
		PrimType *tp = to.prim;
	
		if ((fp->kind == PrimType::Void
		  || tp->kind == PrimType::Void)
		  && fp->kind != tp->kind) {
			error() << "void not castable" << endl;
		} else if (fp->kind == PrimType::Float && tp->kind == PrimType::Float) {
			if (fp->bitwidth < tp->bitwidth) {
				value = cog.builder.CreateFPExt(value, tt);
			} else if (fp->bitwidth > tp->bitwidth) {
				value = cog.builder.CreateFPTrunc(value, tt);
			}
		} else if (ft->isIntegerTy() && tt->isFloatingPointTy()) {
			if (fp->kind == PrimType::Boolean) {
				value = cog.builder.CreateSelect(value, ConstantFP::get(tt, 0.0), ConstantFP::getNaN(tt));
			} else {
				if (fp->kind == PrimType::Unsigned)
					value = cog.builder.CreateUIToFP(value, tt);
				else
					value = cog.builder.CreateSIToFP(value, tt);

				if (fp->exponent != 0)
					value = cog.builder.CreateFMul(value, ConstantFP::get(tt, pow(2.0, (double)fp->exponent)));
			}
		} else if (ft->isFloatingPointTy() && tt->isIntegerTy()) {
			if (tp->kind == PrimType::Boolean) {
				value = cog.builder.CreateFCmpORD(value, value);
			} else {
				if (tp->exponent != 0)
					value = cog.builder.CreateFMul(value, ConstantFP::get(ft, pow(2.0, (double)tp->exponent)));

				if (tp->kind == PrimType::Unsigned)
					value = cog.builder.CreateFPToUI(value, tt);
				else
					value = cog.builder.CreateFPToSI(value, tt);
			}
		} else if (ft->isIntegerTy() && tt->isIntegerTy()) {
			if (fp->kind != PrimType::Boolean && tp->kind != PrimType::Boolean) {
				if (fp->kind == PrimType::Signed && tp->kind == PrimType::Unsigned) {
					value = fn_abs(value);
					fp->kind = PrimType::Unsigned;
				}

				if (fp->bitwidth < tp->bitwidth) {
					if (fp->kind == PrimType::Unsigned)
						value = cog.builder.CreateZExt(value, tt);
					else
						value = cog.builder.CreateSExt(value, tt);
					fp->bitwidth = tp->bitwidth;
					ft = tt;
					fp->llvmType = tt;
				}

				if (fp->exponent > tp->exponent) {
					value = cog.builder.CreateShl(value, fp->exponent - fp->exponent);
				} else if (fp->exponent < tp->exponent) {
					if (fp->kind == PrimType::Unsigned)
						value = cog.builder.CreateLShr(value, tp->exponent - fp->exponent);	
					else
						value = fn_roundedAShr(value, tp->exponent - fp->exponent);
				}
				fp->exponent = tp->exponent;

				if (fp->bitwidth > tp->bitwidth) {
					value = cog.builder.CreateTrunc(value, tt);
					fp->bitwidth = tp->bitwidth;
					ft = tt;
					fp->llvmType = tt;
				}

				fp->kind = tp->kind;
			}
		}
	}

	return value;
}

Info *castType(Info *from, const Typename &to)
{
	from->value = castType(from->value, from->type, to);
	from->symbol = NULL;
	from->type = to;

	return from;
}

int implicitCastDistance(const Typename &from, const Typename &to)
{
	// This is a measure of the number of precision bits lost
	if (from.prim && to.prim) {
		PrimType *fp = from.prim;
		PrimType *tp = to.prim;

		if (fp->kind == PrimType::Void || tp->kind == PrimType::Void)
			return -1;

		// do not implicitly cast signed to unsigned values
		if (fp->kind == PrimType::Signed && tp->kind == PrimType::Unsigned)
			return -1;
		// float cannot cast to fixed
		else if (fp->kind == PrimType::Float
		     && (tp->kind == PrimType::Signed || tp->kind == PrimType::Unsigned))
			return -1;

		int widthDiff = tp->bitwidth - fp->bitwidth;

		// make space for from's implicit sign bit
		if (fp->kind == PrimType::Unsigned && tp->kind == PrimType::Signed)
			--widthDiff;

		if ((fp->kind == PrimType::Signed && tp->kind == PrimType::Signed) ||
		    (fp->kind == PrimType::Unsigned &&
				(tp->kind == PrimType::Signed || tp->kind == PrimType::Unsigned))) {
			int expDiff = tp->exponent - fp->exponent;
			
			// We'll loose significant bits, no can do
			if (widthDiff + expDiff < 0)
				return -1;
			// we'll loose precision
			else if (expDiff > 0)
				return expDiff;
			// otherwise, no data is lost
			else
				return 0;
		}

		if (fp->kind == PrimType::Float && tp->kind == PrimType::Float) {
			if (fp->bitwidth < tp->bitwidth)
				return 0;
			// losing precision is fine, but we can't guarantee the exponent
			else
				return -1;
		}

		if ((fp->kind == PrimType::Signed || fp->kind == PrimType::Unsigned)
		  && tp->kind == PrimType::Float) {
			int expWidth = 0;
			int expMin = 0;
			int expMax = 0;
			switch (tp->bitwidth) {
			case 11: expWidth = 5; expMin = -15; expMax = 16; break;
			case 24: expWidth = 8; expMin = -127; expMax = 128; break;
			case 53: expWidth = 11; expMin = -1023; expMax = 1024; break;
			case 64: expWidth = 15; expMin = -16383; expMax = 16384; break;
			case 113: expWidth = 15; expMin = -16383; expMax = 16384; break;
			default: return -1;
			}

			// this is an estimate. It should be divided by
			// log(10)/log(2) which is about 3.3 but since I'm
			// checking bounds, I don't really want to convert
			// to float.
			int exp10 = (fp->exponent + fp->bitwidth) >> 2;

			// exponent will overflow
			if (exp10 < expMin || exp10 > expMax)
				return -1;
			// loss in precision
			else if (widthDiff < 0)
				return -widthDiff;
			else
				return 0;
		}

		return -1;
	} else if (from.prim && to.base) {
		return -1;
	} else if (from.base && to.prim) {
		return -1;
	} else if (from.base && to.base) {
		BaseType *fb = from.base;
		BaseType *tb = to.base;

		if (fb->args.size() != tb->args.size())
			return -1;

		if (fb->thisType != tb->thisType)
			return -1;

		if (fb->name != tb->name)
			return -1;

		int castDist = 0;
		int tempDist = 0;

		for (int i = 0; i < (int)fb->args.size(); i++) {
			tempDist = implicitCastDistance(fb->args[i].type, tb->args[i].type);
			if (tempDist < 0)
				return -1;
			else
				castDist += tempDist;
		}

		return castDist;
	} else {
		error() << "invalid types '" << from.getName() << "' and '" << to.getName() << "'" << endl;
		return -1;
	}
}

void unaryTypecheck(Info *arg, const Typename &expect)
{
	llvm::Type *at = arg->type.getLlvm();

	if (at == NULL || at->isVoidTy() || at->isLabelTy()
	 || at->isMetadataTy() || at->isTokenTy()) {
		error() << "foun non-valid type '" << arg->type.getName() << "'." << endl;
	} else if (arg->type != expect && (
			at->isStructTy()
	 || at->isFunctionTy()
	 || at->isArrayTy()
	 || at->isPointerTy()
	 || at->isVectorTy())) {
		error() << "type promotion '" << arg->type.getName() << "' to '" << expect.getName() << "' not yet supported." << endl;
	} else {
		if (implicitCastDistance(arg->type, expect) >= 0)
			castType(arg, expect);
		else if (arg->type != expect)
			error() << "unable to cast '" << arg->type.getName() << "' to '" << expect.getName() << "'." << endl;
	}
}

void binaryTypecheck(Info *left, Info *right, Typename *expect)
{
	llvm::Type *lt = left->type.getLlvm();
	llvm::Type *rt = right->type.getLlvm();

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
		int ltor = implicitCastDistance(left->type, right->type);
		int rtol = implicitCastDistance(right->type, left->type);

		if (ltor >= 0 && (rtol < 0 || ltor <= rtol))
			castType(left, right->type);
		else if (rtol >= 0 && (ltor < 0 || rtol <= ltor))
			castType(right, left->type);

		if (left->type != right->type)
			error() << "type mismatch '" << left->type.getName() << "' != '" << right->type.getName() << "'." << endl;
	} else {
		if (implicitCastDistance(left->type, *expect) >= 0)
			castType(left, *expect);
		else
			error() << "unable to cast '" << left->type.getName() << "' to '" << expect->getName() << "'." << endl;
	
		if (implicitCastDistance(right->type, *expect) >= 0)
			castType(right, *expect);
		else
			error() << "unable to cast '" << right->type.getName() << "' to '" << expect->getName() << "'." << endl;
	}
}

Info *unaryOperator(int op, Info *arg)
{
	if (arg != NULL) {
		Typename booleanType;
		booleanType.setPrimType(Type::getInt1Ty(cog.context), PrimType::Boolean);

		if (arg->type.prim) {
			switch (op) {
				case '-':
					arg->value = cog.builder.CreateNeg(arg->value);
					break;
				case '~':
					// TODO should be one of uint, int, ufixed, fixed
					arg->value = cog.builder.CreateNot(arg->value);
					break;
				case NOT:
					unaryTypecheck(arg, booleanType);
					arg->value = cog.builder.CreateNot(arg->value);
					break;
				default:
					error() << "unrecognized unary operator '" << (char)op << "'." << endl;
					delete arg;
					return NULL;
			}
			arg->symbol = NULL;
			return arg;
		} else if (arg->type.base) {
			error() << "operators on structures not yet supported" << endl;
			delete arg;
			return NULL;
		} else {
			error() << "not a valid type" << endl;
			delete arg;
			return NULL;
		}
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

		Typename booleanType;
		booleanType.setPrimType(Type::getInt1Ty(cog.context), PrimType::Boolean);

		if (left->type.prim && right->type.prim) {
			PrimType *lp = left->type.prim;
			PrimType *rp = right->type.prim;
			llvm::Type *lt = lp->llvmType;
			llvm::Type *rt = rp->llvmType;

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
				if (lt->isFloatingPointTy())
					left->value = cog.builder.CreateFCmpOLT(left->value, right->value);
				else if (lp->kind == PrimType::Unsigned)
					left->value = cog.builder.CreateICmpULT(left->value, right->value);
				else
					left->value = cog.builder.CreateICmpSLT(left->value, right->value);
				left->type = booleanType;
				break;
			case '>':
				binaryTypecheck(left, right);
				if (lt->isFloatingPointTy())
					left->value = cog.builder.CreateFCmpOGT(left->value, right->value);
				else if (lp->kind == PrimType::Unsigned)
					left->value = cog.builder.CreateICmpUGT(left->value, right->value);
				else 
					left->value = cog.builder.CreateICmpSGT(left->value, right->value);
				left->type = booleanType;
				break;
			case LE:
				binaryTypecheck(left, right);
				if (lt->isFloatingPointTy())
					left->value = cog.builder.CreateFCmpOLE(left->value, right->value);
				else if (lp->kind == PrimType::Unsigned)
					left->value = cog.builder.CreateICmpULE(left->value, right->value);
				else 
					left->value = cog.builder.CreateICmpSLE(left->value, right->value);
				left->type = booleanType;
				break;
			case GE:
				binaryTypecheck(left, right);
				if (lt->isFloatingPointTy())
					left->value = cog.builder.CreateFCmpOGE(left->value, right->value);
				else if (lp->kind == PrimType::Unsigned)
					left->value = cog.builder.CreateICmpUGE(left->value, right->value);
				else 
					left->value = cog.builder.CreateICmpSGE(left->value, right->value);
				left->type = booleanType;
				break;
			case EQ:
				binaryTypecheck(left, right);
				if (lt->isFloatingPointTy())
					left->value = cog.builder.CreateFCmpOEQ(left->value, right->value);
				else 
					left->value = cog.builder.CreateICmpEQ(left->value, right->value);
				left->type = booleanType;
				break;
			case NE:
				binaryTypecheck(left, right);
				if (lt->isFloatingPointTy())
					left->value = cog.builder.CreateFCmpUNE(left->value, right->value);
				else 
					left->value = cog.builder.CreateICmpNE(left->value, right->value);
				left->type = booleanType;
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
				width = ConstantInt::get(lt, lt->getIntegerBitWidth());
				inv = cog.builder.CreateSub(width, right->value);
				lsb = cog.builder.CreateLShr(left->value, right->value);
				msb = cog.builder.CreateShl(left->value, inv);
				left->value = cog.builder.CreateOr(msb, lsb);
				break;
			case ROL:
				binaryTypecheck(left, right);
				width = ConstantInt::get(lt, lt->getIntegerBitWidth());
				inv = cog.builder.CreateSub(width, right->value);
				lsb = cog.builder.CreateLShr(left->value, inv);
				msb = cog.builder.CreateShl(left->value, right->value);
				left->value = cog.builder.CreateOr(msb, lsb);
				break;
			case '+':
				binaryTypecheck(left, right);
				if (lt->isFloatingPointTy())
					left->value = cog.builder.CreateFAdd(left->value, right->value);
				else 
					left->value = cog.builder.CreateAdd(left->value, right->value);
				break;
			case '-':
				binaryTypecheck(left, right);
				if (lt->isFloatingPointTy())
					left->value = cog.builder.CreateFSub(left->value, right->value);
				else 
					left->value = cog.builder.CreateSub(left->value, right->value);
				break;
			case '*':
				binaryTypecheck(left, right);
				if (lt->isFloatingPointTy())
					left->value = cog.builder.CreateFMul(left->value, right->value);
				else
					left->value = cog.builder.CreateMul(left->value, right->value);
				break;
			case '/':
				printf("%x %x\n", left, left->value);
				printf("%x %x\n", right, right->value);
				binaryTypecheck(left, right);
				printf("%x %x\n", left, left->value);
				printf("%x %x\n", right, right->value);
				if (lt->isFloatingPointTy())
					left->value = cog.builder.CreateFDiv(left->value, right->value);
				else if (lp->kind == PrimType::Unsigned)
					left->value = cog.builder.CreateUDiv(left->value, right->value);
				else
					left->value = cog.builder.CreateSDiv(left->value, right->value);
				break;
			case '%':
				binaryTypecheck(left, right);
				if (lt->isFloatingPointTy())
					left->value = cog.builder.CreateFRem(left->value, right->value);
				else if (lp->kind == PrimType::Unsigned)
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
		}
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
		unaryTypecheck(value, symbol->type);
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
	cog.getStructure(name, cog.getScope()->symbols);
	cog.getScope()->symbols.clear();

	cog.scopes.pop_back();
	cog.scopes.push_back(Scope());
}

Info *functionPrototype(Info *retType, char *name)
{
	Scope *scope = cog.getScope();

	Typename voidType;
	voidType.setPrimType(Type::getVoidTy(cog.context), PrimType::Void);

	BaseType *fType = cog.getFunction(retType->type, voidType, name, scope->symbols);
	delete retType;

	std::string mangled = fType->getName(false);

	Function *func = cog.module->getFunction(mangled.c_str());
	if (func == NULL)
		func = Function::Create((llvm::FunctionType*)fType->llvmType, Function::ExternalLinkage, mangled.c_str(), cog.module);

	scope->symbols.clear();
	
	Info *result = new Info();
	result->type.base = fType;
	return result;
}

void functionDeclaration(Info *retType, char *name)
{
	Scope *scope = cog.getScope();

	Typename voidType;
	voidType.setPrimType(Type::getVoidTy(cog.context), PrimType::Void);

	BaseType *fType = cog.getFunction(retType->type, voidType, name, scope->symbols);
	delete retType;

	std::string mangled = fType->getName(false);
	
	Function *func = cog.module->getFunction(mangled.c_str());
	if (func == NULL)
		func = Function::Create((llvm::FunctionType*)fType->llvmType, Function::ExternalLinkage, mangled.c_str(), cog.module);
	else if (!func->empty()) {
		error() << "function redefined.\n";
		int i = -1;
		while (func != NULL) {
			++i;
			func = cog.module->getFunction((mangled + "_" + char('a' + i)).c_str());
		}

		func = Function::Create((llvm::FunctionType*)fType->llvmType, Function::ExternalLinkage, (mangled + "_" + char('a' + i)).c_str(), cog.module);
	}

	int i = 0;
	for (auto &arg : func->args()) {
		arg.setName(scope->symbols[i].name);
		scope->symbols[i].setValue(&arg);
		++i;
	}

	cog.currFn = fType;
	BasicBlock *body = BasicBlock::Create(cog.context, "entry", func, 0);
	scope->blocks.push_back(body);
	scope->curr = scope->blocks.begin();
	cog.builder.SetInsertPoint(body);
}

void functionDefinition()
{
	cog.scopes.pop_back();
	cog.scopes.push_back(Scope());
	cog.currFn = NULL;
}

void returnValue(Info *value)
{
	if (value) {
		if (cog.currFn != NULL) {
			unaryTypecheck(value, cog.currFn->retType);
			cog.builder.CreateRet(value->value);
		} else {
			error() << "return outside of function" << endl;
		}

		delete value;
	} else {
		error() << "here" << endl;
	}
}

void returnVoid()
{
	if (cog.currFn->retType.prim == NULL || cog.currFn->retType.prim->kind != PrimType::Void) {	
		error() << "unable to cast 'void' to '" << cog.currFn->retType.getName() << "'." << endl;
	}

	cog.builder.CreateRetVoid();
}

void callFunction(char *txt, Info *argList)
{
	std::vector<std::pair<Typename, llvm::Value*> > args;
	Info *curr = argList;
	while (curr != NULL) {
		args.push_back(std::pair<Typename, llvm::Value*>(curr->type, curr->value));
		curr = curr->next;
	}

	Typename thisType;
	thisType.setPrimType(Type::getVoidTy(cog.context), PrimType::Void);
	std::string name = txt;

	int targetDist = -1;
	std::vector<BaseType*> targets;
	for (auto type = cog.types.begin(); type != cog.types.end(); type++) {
		int castDist = 0;
		
		if (type->args.size() != args.size()
		 || type->thisType != thisType
		 || type->name != name)
			castDist = -1;

		int tempDist = 0;
		for (int i = 0; i < (int)args.size() && castDist > 0; i++) {
			if (type->args[i].type != args[i].first) {
				tempDist = implicitCastDistance(type->args[i].type, args[i].first);
				if (tempDist < 0)
					castDist = -1;
				else
					castDist += tempDist;
			}
		}

		if (castDist >= 0 && (targetDist < 0 || castDist < targetDist)) {
			targetDist = castDist;
			targets.clear();
			targets.push_back(&(*type));
		} else if (targetDist >= 0 && castDist == targetDist) {
			targets.push_back(&(*type));
		}
	}

	if (targets.size() != 1) {
		if (targets.size() == 0)
			error() << "undefined reference to ";
		else
			error() << "ambiguous reference to ";

		error() << name << "(";
		for (int i = 0; i < (int)args.size(); i++) {
			if (i != 0)
				error() << ", ";
			error() << args[i].first.getName();
		}
		error() << ")" << endl;
	}
	else {
		std::vector<Value*> argValues;
		for (int i = 0; i < (int)args.size(); i++) {
			if (targets[0]->args[i].type != args[i].first) {
				argValues.push_back(castType(args[i].second, args[i].first, targets[0]->args[i].type));
			} else {
				argValues.push_back(args[i].second);
			}
		}

		std::string mangled_name = targets[0]->getName(false);
		cog.builder.CreateCall(cog.module->getFunction(mangled_name.c_str()), argValues);
	}

	if (argList != NULL)
		delete argList;
}

void ifCondition(Info *cond)
{
	Function *func = cog.builder.GetInsertBlock()->getParent();
	
	BasicBlock *thenBlock = BasicBlock::Create(cog.context, "then", func);
	BasicBlock *elseBlock = BasicBlock::Create(cog.context, "else", func);
	cog.getScope()->appendBlock(thenBlock);
	cog.getScope()->appendBlock(elseBlock);

	Typename booleanType;
	booleanType.setPrimType(Type::getInt1Ty(cog.context), PrimType::Boolean);

	unaryTypecheck(cond, booleanType);

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
		phi.push_back(cog.builder.CreatePHI(scope->symbols[i].type.getLlvm(), scope->blocks.size()-1));
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
		PHINode *value = cog.builder.CreatePHI(scope->symbols[i].type.getLlvm(), scope->blocks.size()-1);
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

	Typename booleanType;
	booleanType.setPrimType(Type::getInt1Ty(cog.context), PrimType::Boolean);

	unaryTypecheck(cond, booleanType);

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
						inTypes.push_back(addr->type.getLlvm());
						inValues.push_back(addr->value);
					} else {
						if (outCnsts.size() > 0)
							outCnsts += ",";
						outCnsts += "=r";
						outs.push_back(addr->symbol);
						outTypes.push_back(addr->type.getLlvm());
					}
				}
			}

			if (arg->symbol) {
				if (arg->next) {
					if (inCnsts.size() > 0)
						inCnsts += ",";
					inCnsts += "r";
					inTypes.push_back(arg->type.getLlvm());
					inValues.push_back(arg->value);
				} else {
					if (outCnsts.size() > 0)
						outCnsts += ",";
					outCnsts += "=r";
					outs.push_back(arg->symbol);
					outTypes.push_back(arg->type.getLlvm());
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

