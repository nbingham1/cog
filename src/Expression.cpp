#include "Expression.h"
#include "Compiler.h"
#include "Parser.y.h"
#include "Intrinsic.h"

extern Cog::Compiler cog;

using std::endl;

namespace Cog
{

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
						value = fn_div2(value, tp->exponent - fp->exponent);
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

void binaryTypecheck(Info *left, Info *right)
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
	} else {
		int ltor = implicitCastDistance(left->type, right->type);
		int rtol = implicitCastDistance(right->type, left->type);

		if (ltor >= 0 && (rtol < 0 || ltor <= rtol))
			castType(left, right->type);
		else if (rtol >= 0 && (ltor < 0 || rtol <= ltor))
			castType(right, left->type);

		if (left->type != right->type)
			error() << "type mismatch '" << left->type.getName() << "' != '" << right->type.getName() << "'." << endl;
	}
}

Info *getBooleanOr(Info *left, Info *right)
{
	Typename booleanType = Typename::getBool();
	unaryTypecheck(left, booleanType);
	unaryTypecheck(right, booleanType);
	left->value = cog.builder.CreateOr(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getBooleanXor(Info *left, Info *right)
{
	Typename booleanType = Typename::getBool();
	unaryTypecheck(left, booleanType);
	unaryTypecheck(right, booleanType);
	left->value = cog.builder.CreateXor(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getBooleanAnd(Info *left, Info *right)
{
	Typename booleanType = Typename::getBool();
	unaryTypecheck(left, booleanType);
	unaryTypecheck(right, booleanType);
	left->value = cog.builder.CreateAnd(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getCmpLT(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	if (lt->isFloatingPointTy())
		left->value = cog.builder.CreateFCmpOLT(left->value, right->value);
	else if (lp->kind == PrimType::Unsigned)
		left->value = cog.builder.CreateICmpULT(left->value, right->value);
	else
		left->value = cog.builder.CreateICmpSLT(left->value, right->value);
	left->type = Typename::getBool();
	left->symbol = NULL;
	return left;
}

Info *getCmpGT(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	if (lt->isFloatingPointTy())
		left->value = cog.builder.CreateFCmpOGT(left->value, right->value);
	else if (lp->kind == PrimType::Unsigned)
		left->value = cog.builder.CreateICmpUGT(left->value, right->value);
	else 
		left->value = cog.builder.CreateICmpSGT(left->value, right->value);
	left->type = Typename::getBool();
	left->symbol = NULL;
	return left;
}

Info *getCmpLE(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	if (lt->isFloatingPointTy())
		left->value = cog.builder.CreateFCmpOLE(left->value, right->value);
	else if (lp->kind == PrimType::Unsigned)
		left->value = cog.builder.CreateICmpULE(left->value, right->value);
	else 
		left->value = cog.builder.CreateICmpSLE(left->value, right->value);
	left->type = Typename::getBool();
	left->symbol = NULL;
	return left;
}

Info *getCmpGE(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	if (lt->isFloatingPointTy())
		left->value = cog.builder.CreateFCmpOGE(left->value, right->value);
	else if (lp->kind == PrimType::Unsigned)
		left->value = cog.builder.CreateICmpUGE(left->value, right->value);
	else 
		left->value = cog.builder.CreateICmpSGE(left->value, right->value);
	left->type = Typename::getBool();
	left->symbol = NULL;
	return left;
}

Info *getCmpEQ(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	if (lt->isFloatingPointTy())
		left->value = cog.builder.CreateFCmpOEQ(left->value, right->value);
	else 
		left->value = cog.builder.CreateICmpEQ(left->value, right->value);
	left->type = Typename::getBool();
	left->symbol = NULL;
	return left;
}

Info *getCmpNE(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	if (lt->isFloatingPointTy())
		left->value = cog.builder.CreateFCmpUNE(left->value, right->value);
	else 
		left->value = cog.builder.CreateICmpNE(left->value, right->value);
	left->type = Typename::getBool();
	left->symbol = NULL;
	return left;
}

Info *getBitwiseOr(Info *left, Info *right)
{
	binaryTypecheck(left, right);
	left->value = cog.builder.CreateOr(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getBitwiseXor(Info *left, Info *right)
{
	binaryTypecheck(left, right);
	left->value = cog.builder.CreateXor(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getBitwiseAnd(Info *left, Info *right)
{
	binaryTypecheck(left, right);
	left->value = cog.builder.CreateAnd(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getShl(Info *left, Info *right)
{
	binaryTypecheck(left, right);
	left->value = cog.builder.CreateShl(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getAshr(Info *left, Info *right)
{
	binaryTypecheck(left, right);
	left->value = cog.builder.CreateAShr(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getLshr(Info *left, Info *right)
{
	binaryTypecheck(left, right);
	left->value = cog.builder.CreateLShr(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getRor(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	llvm::Value *width = ConstantInt::get(lt, lt->getIntegerBitWidth());
	llvm::Value *inv = cog.builder.CreateSub(width, right->value);
	llvm::Value *lsb = cog.builder.CreateLShr(left->value, right->value);
	llvm::Value *msb = cog.builder.CreateShl(left->value, inv);
	left->value = cog.builder.CreateOr(msb, lsb);
	left->symbol = NULL;
	return left;
}

Info *getRol(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	llvm::Value *width = ConstantInt::get(lt, lt->getIntegerBitWidth());
	llvm::Value *inv = cog.builder.CreateSub(width, right->value);
	llvm::Value *lsb = cog.builder.CreateLShr(left->value, inv);
	llvm::Value *msb = cog.builder.CreateShl(left->value, right->value);
	left->value = cog.builder.CreateOr(msb, lsb);
	left->symbol = NULL;
	return left;
}

Info *getAdd(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	if (lt->isFloatingPointTy())
		left->value = cog.builder.CreateFAdd(left->value, right->value);
	else 
		left->value = cog.builder.CreateAdd(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getSub(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	if (lt->isFloatingPointTy())
		left->value = cog.builder.CreateFSub(left->value, right->value);
	else 
		left->value = cog.builder.CreateSub(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getMult(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	if (lt->isFloatingPointTy())
		left->value = cog.builder.CreateFMul(left->value, right->value);
	else
		left->value = cog.builder.CreateMul(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getDiv(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	if (lt->isFloatingPointTy())
		left->value = cog.builder.CreateFDiv(left->value, right->value);
	else if (lp->kind == PrimType::Unsigned)
		left->value = cog.builder.CreateUDiv(left->value, right->value);
	else
		left->value = cog.builder.CreateSDiv(left->value, right->value);
	left->symbol = NULL;
	return left;
}

Info *getRem(Info *left, Info *right)
{
	PrimType *lp = left->type.prim;
	llvm::Type *lt = lp->llvmType;

	binaryTypecheck(left, right);
	if (lt->isFloatingPointTy())
		left->value = cog.builder.CreateFRem(left->value, right->value);
	else if (lp->kind == PrimType::Unsigned)
		left->value = cog.builder.CreateURem(left->value, right->value);
	else
		left->value = cog.builder.CreateSRem(left->value, right->value);
	left->symbol = NULL;
	return left;
}

}

