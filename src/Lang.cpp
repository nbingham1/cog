#include "Lang.h"
#include "Compiler.h"
#include "Parser.y.h"

#include <stdio.h>
#include <string.h>
#include <llvm/ADT/Twine.h>

extern Cog::Compiler cog;
extern int line;
extern int column;

namespace Cog
{

Info *getTypename(int token, char *txt)
{
	// TODO distinction between signed & unsigned integers
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
			printf("error: %d:%d floating point types must be 16, 32, 64, or 128 bits.\n", line, column); 
		}
	} else if (token == IDENTIFIER) {
		
	} else {
		printf("error: %d:%d undefined type\n", line, column); 
	}

	delete result;
	delete txt;
	return NULL;
}

Info *getConstant(int token, char *txt)
{
	Info *result = new Info();
	//result->value = ConstantFP::get(Type::getDoubleTy(cog.context), atof(txt));
	result->value = ConstantInt::get(Type::getInt32Ty(cog.context), atoi(txt));
	result->type.prim = result->value->getType();
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

	printf("error: %d:%d undefined variable\n", line, column); 
	delete txt;
	return NULL;
}

Info *castType(Info *from, Info *to)
{
	llvm::Type *ft = from->type.prim;
	llvm::Type *tt = to->type.prim;

	if (ft == NULL || ft->isVoidTy() || ft->isLabelTy()
	 || ft->isMetadataTy() || ft->isTokenTy()) {
		printf("internal: %d:%d found non-valid type '%s'\n", line, column, from->type.getName().c_str());
	} else if (tt == NULL || tt->isVoidTy() || tt->isLabelTy()
	 || tt->isMetadataTy() || tt->isTokenTy()) {
		printf("internal: %d:%d found non-valid type '%s'\n", line, column, to->type.getName().c_str());
	} else if (ft != tt && (
	    ft->isStructTy()		|| tt->isStructTy()
	 || ft->isFunctionTy()	|| tt->isFunctionTy()
	 || ft->isArrayTy()			|| tt->isArrayTy()
	 || ft->isPointerTy()		|| tt->isPointerTy()
	 || ft->isVectorTy()		|| tt->isVectorTy())) {
		printf("error: %d:%d typecast '%s' to '%s' not yet supported\n",
		  line, column, from->type.getName().c_str(), to->type.getName().c_str());
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
				from->type.fpExp = to->type.fpExp;
			}

			if (ft->getIntegerBitWidth() > tt->getIntegerBitWidth()) {
				from->value = cog.builder.CreateTrunc(from->value, tt);
				ft = tt;
				from->type.prim = tt;
			}
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
		printf("internal: %d:%d found non-valid type '%s'\n", line, column, arg->type.getName().c_str());
	} else if (at != expect->type.prim && (
	    at->isStructTy()
	 || at->isFunctionTy()
	 || at->isArrayTy()
	 || at->isPointerTy()
	 || at->isVectorTy())) {
		printf("error: %d:%d type promotion '%s', '%s' not yet supported\n",
		  line, column, arg->type.getName().c_str(), expect->type.getName().c_str());
	} else {
		if (canImplicitCast(arg, expect))
			castType(arg, expect);
		else if (arg->type != expect->type)
			printf("error: %d:%d unable to cast '%s' to '%s'\n", line, column, arg->type.getName().c_str(), expect->type.getName().c_str());
	}
}

void binaryTypecheck(Info *left, Info *right, Info *expect)
{
	llvm::Type *lt = left->type.prim;
	llvm::Type *rt = right->type.prim;

	if (lt == NULL || lt->isVoidTy() || lt->isLabelTy()
	 || lt->isMetadataTy() || lt->isTokenTy()) {
		printf("internal: %d:%d found non-valid type '%s'\n", line, column, left->type.getName().c_str());
	} else if (rt == NULL || rt->isVoidTy() || rt->isLabelTy()
	 || rt->isMetadataTy() || rt->isTokenTy()) {
		printf("internal: %d:%d found non-valid type '%s'\n", line, column, right->type.getName().c_str());
	} else if (lt != rt && (
	    lt->isStructTy()		|| rt->isStructTy()
	 || lt->isFunctionTy()	|| rt->isFunctionTy()
	 || lt->isArrayTy()			|| rt->isArrayTy()
	 || lt->isPointerTy()		|| rt->isPointerTy()
	 || lt->isVectorTy()		|| rt->isVectorTy())) {
		printf("error: %d:%d type promotion '%s', '%s' not yet supported\n",
		  line, column, left->type.getName().c_str(), right->type.getName().c_str());
	} else if (expect == NULL) {
		if (canImplicitCast(right, left)) {
			castType(right, left);
		} else if (canImplicitCast(left, right)) {
			castType(left, right);
		}

		if (left->type != right->type)
			printf("error: %d:%d type mismatch '%s' != '%s'\n", line, column, left->type.getName().c_str(), right->type.getName().c_str());
	} else {
		if (canImplicitCast(left, expect))
			castType(left, expect);
		else
			printf("error: %d:%d unable to cast '%s' to '%s'\n", line, column, left->type.getName().c_str(), expect->type.getName().c_str());
	
		if (canImplicitCast(right, expect))
			castType(right, expect);
		else
			printf("error: %d:%d unable to cast '%s' to '%s'\n", line, column, right->type.getName().c_str(), expect->type.getName().c_str());
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
				printf("error: %d:%d unrecognized operator\n", line, column); 
				delete arg;
				return NULL;
		}
		arg->type.prim = arg->value->getType();
		arg->symbol = NULL;
		return arg;
	} else {
		printf("error: %d:%d previous failure\n", line, column); 
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
			left->value = cog.builder.CreateOr(left->value, right->value);
			break;
		case '^':
			left->value = cog.builder.CreateXor(left->value, right->value);
			break;
		case '&':
			left->value = cog.builder.CreateAnd(left->value, right->value);
			break;
		case SHL:
			left->value = cog.builder.CreateShl(left->value, right->value);
			break;
		case ASHR:
			left->value = cog.builder.CreateAShr(left->value, right->value);
			break;
		case LSHR:
			left->value = cog.builder.CreateLShr(left->value, right->value);
			break;
		case ROR:
			width = ConstantInt::get(cog.context, APInt(32, (int)left->value->getType()->getIntegerBitWidth(), true));
			inv = cog.builder.CreateSub(width, right->value);
			lsb = cog.builder.CreateLShr(left->value, right->value);
			msb = cog.builder.CreateShl(left->value, inv);
			left->value = cog.builder.CreateOr(msb, lsb);
			break;
		case ROL:
			width = ConstantInt::get(cog.context, APInt(32, (int)left->value->getType()->getIntegerBitWidth(), true));
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
			printf("error: %d:%d unrecognized operator\n", line, column); 
			delete left;
			delete right;
			return NULL;
		}
		left->type.prim = left->value->getType();
		left->symbol = NULL;
		delete right;
		return left;
	} else {
		printf("error: %d:%d previous failure\n", line, column); 
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
			printf("error: %d:%d variable '%s' already defined\n", line, column, name); 
			// already defined
		}

		cog.getScope()->createSymbol(name, type->type);
		delete type;
		delete name;
	} else {
		printf("error: %d:%d previous failure\n", line, column); 
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
		printf("error: %d:%d previous failure\n", line, column); 
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
		printf("error: %d:%d previous failure\n", line, column); 
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
	Info *curr = lst;
	while (curr->next != NULL)
		curr = curr->next;
	curr->next = elem;
	return lst;
}

}

