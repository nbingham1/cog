#include "Lang.h"
#include "Compiler.h"
#include "Intrinsic.h"
#include "Expression.h"
#include "Parser.y.h"

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <llvm/ADT/Twine.h>

extern Cog::Compiler cog;

using std::endl;

namespace Cog
{

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
		result->type = Typename::getVoid();
	} else if (token == BOOL_PRIMITIVE) {
		result->type = Typename::getBool();
	} else if (token == INT_PRIMITIVE && txt[0] == 'u') {
		result->type = Typename::getUInt(atoi(txt+4));
	} else if (token == INT_PRIMITIVE && txt[0] != 'u') {
		result->type = Typename::getInt(atoi(txt+3));
	} else if (token == FIXED_PRIMITIVE && txt[0] == 'u') {
		unsigned int bitwidth = atoi(txt+6);
		result->type = Typename::getUFixed(bitwidth, bitwidth/4);
	} else if (token == FIXED_PRIMITIVE && txt[0] != 'u') {
		unsigned int bitwidth = atoi(txt+5);
		result->type = Typename::getFixed(bitwidth, bitwidth/4);
	} else if (token == FLOAT_PRIMITIVE) {
		result->type = Typename::getFloat(atoi(txt+5));
	} else if (token == IDENTIFIER) {
	
	} else {
		error() << "undefined type." << endl;
	}

	delete txt;
	if (result->type.isSet())
		return result;
	else {
		delete result;
		return NULL;
	}
}

Info *getStaticArrayTypename(Info *name, Info *cnst)
{
	if (name)
		delete name;
	if (cnst)
		delete cnst;

	return NULL;
}

Info *getDynamicArrayTypename(Info *name)
{
	if (name)
		delete name;

	return NULL;
}

Info *getPointerTypename(Info *name)
{
	if (name)
		delete name;

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

	if (cnst < 0)
		result->type = Typename::getFixed(value.getBitWidth(), exponent);
	else
		result->type = Typename::getUFixed(value.getBitWidth(), exponent);
	result->value = ConstantInt::get(result->type.getLlvm(), value);
	
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
		result->type = Typename::getBool();
		if (*curr == 't' || *curr == 'T' || *curr == 'y' || *curr == 'Y' || *curr == '1')
			result->value = ConstantInt::get(result->type.getLlvm(), 1);
		else
			result->value = ConstantInt::get(result->type.getLlvm(), 0);
		delete txt;
		return result;
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
		delete result;
		return NULL;
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

	if (isNegative)
		result->type = Typename::getFixed(value.getBitWidth(), exponent);
	else
		result->type = Typename::getUFixed(value.getBitWidth(), exponent);
	result->value = ConstantInt::get(result->type.getLlvm(), value);

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

Info *unaryOperator(int op, Info *arg)
{
	if (arg != NULL) {
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
					unaryTypecheck(arg, Typename::getBool());
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
		if (left->type.prim && right->type.prim) {
			switch (op) {
			case OR:   getBooleanOr(left, right); break;
			case XOR:  getBooleanXor(left, right); break;
			case AND:  getBooleanAnd(left, right); break;
			case '<':  getCmpLT(left, right); break;
			case '>':  getCmpGT(left, right); break;
			case LE:   getCmpLE(left, right); break;
			case GE:   getCmpGE(left, right); break;
			case EQ:   getCmpEQ(left, right); break;
			case NE:   getCmpNE(left, right); break;
			case '|':  getBitwiseOr(left, right); break;
			case '^':  getBitwiseXor(left, right); break;
			case '&':  getBitwiseAnd(left, right); break;
			case SHL:  getShl(left, right); break;
			case ASHR: getAshr(left, right); break;
			case LSHR: getLshr(left, right); break;
			case ROR:  getRor(left, right); break;
			case ROL:  getRol(left, right); break;
			case '+':  getAdd(left, right); break;
			case '-':  getSub(left, right); break;
			case '*':  getMult(left, right); break;
			case '/':  getDiv(left, right); break;
			case '%':  getRem(left, right); break;
			default:
				error() << "unrecognized binary operator '" << (char)op << "'." << endl;
				delete left;
				delete right;
				return NULL;
			}
		}
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


void declareSymbols(Info *type, Info *names)
{
	if (type && names) {
		Info *curr = names;
		while (curr != NULL) {
			printf("%s\n", curr->text.c_str());
			if (cog.getScope()->findSymbol(curr->text) != NULL) {
				error() << "variable '" << curr->text << "' already defined." << endl;
			}

			Symbol *symbol = cog.getScope()->createSymbol(curr->text, type->type);

			if (curr->value) {
				unaryTypecheck(curr, symbol->type);
				symbol->setValue(curr->value);
				curr->value->setName(symbol->name);
			}
			curr = curr->next;
		}

		delete type;
		delete names;
	} else {
		error() << "here" << endl;
		if (type)
			delete type;
		if (names)
			delete names;
		// something is broken
	}
}

void assignSymbol(Info *left, int op, Info *right)
{
	if (left && right) {
		Symbol *symbol = left->symbol;

		switch (op) {
		case '=': break;
		case ASSIGN_MUL: getMult(left, right); break;
		case ASSIGN_DIV: getDiv(left, right); break;
		case ASSIGN_REM: getRem(left, right); break;
		case ASSIGN_ADD: getAdd(left, right); break;
		case ASSIGN_SUB: getSub(left, right); break;
		case ASSIGN_SHL: getShl(left, right); break;
		case ASSIGN_ASHR: getAshr(left, right); break;
		case ASSIGN_LSHR: getLshr(left, right); break;
		case ASSIGN_ROL: getRol(left, right); break;
		case ASSIGN_ROR: getRor(left, right); break;
		case ASSIGN_AND: getBitwiseAnd(left, right); break;
		case ASSIGN_XOR: getBitwiseXor(left, right); break;
		case ASSIGN_OR: getBitwiseOr(left, right); break;
		case ASSIGN_BAND: getBooleanAnd(left, right); break;
		case ASSIGN_BXOR: getBooleanXor(left, right); break;
		case ASSIGN_BOR: getBooleanOr(left, right); break;
		}

		unaryTypecheck(left, symbol->type);
		symbol->setValue(left->value);
		left->value->setName(symbol->name);

		if (left)
			delete left;
		if (right)
			delete right;
	} else {
		error() << "here" << endl;
		if (left)
			delete left;
		if (right)
			delete right;
		// something is broken
	}
}

Info *variableDeclarationName(char *name, Info *expr)
{
	if (expr == NULL)
		expr = new Info();

	expr->text = name;
	delete name;
	return expr;
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

	unaryTypecheck(cond, Typename::getBool());

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

	unaryTypecheck(cond, Typename::getBool());

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

