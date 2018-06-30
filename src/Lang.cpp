#include "Lang.h"
#include "Compiler.h"
#include "Parser.y.h"

#include <stdio.h>
#include <string.h>

extern Cog::Compiler cog;
extern int line;
extern int column;

namespace Cog
{

Info *getConstant(char *txt)
{
	Info *result = new Info();
	result->value = ConstantInt::get(cog.context, APInt(32, atoi(txt), true));
	result->type = result->value->getType();
	delete txt;
	return result;
}

Info *getTypename(char *txt)
{
	for (int i = 0; i < (int)cog.types.size(); i++) {
		if (strcmp(cog.types[i].name.c_str(), txt) == 0) {
			Info *result = new Info();
			result->type = cog.types[i].type;
			delete txt;
			return result;
		}
	}

	printf("error: %d:%d undefined type\n", line, column); 
	delete txt;
	return NULL;
}

Info *getIdentifier(char *txt)
{
	Symbol *symbol = cog.findSymbol(txt);
	if (symbol) {
		Info *result = new Info();
		result->symbol = symbol;
		result->value = symbol->getValue();
		result->type = result->value->getType();
		delete txt;
		return result;
	}

	printf("error: %d:%d undefined variable\n", line, column); 
	delete txt;
	return NULL;
}

Info *unaryOperator(int op, Info *arg)
{
	if (arg) {
		switch (op) {
			case '-':
				arg->value = cog.builder.CreateNeg(arg->value);
				break;
			case '+':
				return arg;
			case '~':
				arg->value = cog.builder.CreateNot(arg->value);
				break;
			case NOT:
				// TODO cast to boolean (int1)
				arg->value = cog.builder.CreateNot(arg->value);
				break;
			default:
				printf("error: %d:%d unrecognized operator\n", line, column); 
				delete arg;
				return NULL;
		}
		arg->type = arg->value->getType();
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

		switch (op) {
		case OR:
			left->value = cog.builder.CreateOr(left->value, right->value);
			break;
		case XOR:
			left->value = cog.builder.CreateXor(left->value, right->value);
			break;
		case AND:
			left->value = cog.builder.CreateAnd(left->value, right->value);
			break;
		case '<':
			left->value = cog.builder.CreateICmpSLT(left->value, right->value);
			break;
		case '>':
			left->value = cog.builder.CreateICmpSGT(left->value, right->value);
			break;
		case LE:
			left->value = cog.builder.CreateICmpSLE(left->value, right->value);
			break;
		case GE:
			left->value = cog.builder.CreateICmpSGE(left->value, right->value);
			break;
		case EQ:
			left->value = cog.builder.CreateICmpEQ(left->value, right->value);
			break;
		case NE:
			left->value = cog.builder.CreateICmpNE(left->value, right->value);
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
			left->value = cog.builder.CreateAdd(left->value, right->value);
			break;
		case '-':
			left->value = cog.builder.CreateSub(left->value, right->value);
			break;
		case '*':
			left->value = cog.builder.CreateMul(left->value, right->value);
			break;
		case '/':
			left->value = cog.builder.CreateSDiv(left->value, right->value);
			break;
		case '%':
			left->value = cog.builder.CreateSRem(left->value, right->value);
			break;
		default:
			printf("error: %d:%d unrecognized operator\n", line, column); 
			delete left;
			delete right;
			return NULL;
		}
		left->type = left->value->getType();
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

void declareSymbol(Info *type, char *txt)
{
	if (type && txt) {
		if (cog.findSymbol(txt) != NULL) {
			printf("error: %d:%d variable already defined\n", line, column); 
			// already defined
		}

		cog.createSymbol(txt, type->type);
		delete type;
		delete txt;
	} else {
		printf("error: %d:%d previous failure\n", line, column); 
		if (type)
			delete type;
		if (txt)
			delete txt;
		// something is broken
	}
}

void assignSymbol(Info *symbol, Info *value)
{
	if (symbol && value) {
		if (symbol->type == value->type) {
			symbol->symbol->setValue(value->value);
		} else {
			printf("error: %d:%d typecheck failed\n", line, column); 
			// typecheck failed
		}
	} else {
		printf("error: %d:%d previous failure\n", line, column); 
		if (symbol)
			delete symbol;
		if (value)
			delete value;
		// something is broken
	}
}

void callFunction(char *txt)
{
	cog.createExit();
}

void returnValue(Info *value)
{
	if (value) {
		cog.builder.CreateRet(value->value);
	} else {
		printf("error: %d:%d previous failure\n", line, column); 
	}
}

void returnVoid()
{
	cog.builder.CreateRetVoid();
}

void pushScope()
{
	cog.pushScope();
}

void popScope()
{
	cog.popScope();
}

void ifCondition(Info *cond)
{
	Function *func = cog.builder.GetInsertBlock()->getParent();
	
	printf("ifCondition()\n");
	BasicBlock *thenBlock = BasicBlock::Create(cog.context, "then", func);
	BasicBlock *elseBlock = BasicBlock::Create(cog.context, "else", func);
	cog.getScope()->blocks.push_back(thenBlock);
	cog.getScope()->blocks.push_back(elseBlock);

	cog.builder.CreateCondBr(cond->value, thenBlock, elseBlock);
	cog.getScope()->popBlock();
	cog.pushScope();
	cog.builder.SetInsertPoint(cog.getScope()->getBlock());
}

void elseifCondition()
{
	printf("elseifCondition()\n");
	cog.popScope();
	cog.getScope()->nextBlock();	
	cog.builder.SetInsertPoint(cog.getScope()->getBlock());
}

void elseCondition()
{
	printf("elseCondition()\n");
	cog.popScope();
	cog.getScope()->nextBlock();
	cog.pushScope();
	cog.builder.SetInsertPoint(cog.getScope()->getBlock());
}

void ifStatement()
{
	Function *func = cog.builder.GetInsertBlock()->getParent();
	
	printf("ifStatement()\n");
	cog.popScope();
	Scope *scope = cog.getScope();
	// if this block has an else statement we can't use the remaining else block
	if (std::next(scope->curr) == scope->blocks.end()) {
		BasicBlock *mergeBlock = BasicBlock::Create(cog.context, "merge", func);
		scope->blocks.push_back(mergeBlock);
	}

	cog.printScope();	
	scope->nextBlock();
	cog.printScope();	
		
	while (scope->blocks.size() > 1) {
		cog.builder.SetInsertPoint(scope->blocks.front());
		cog.builder.CreateBr(scope->getBlock());
		scope->blocks.pop_front();
		cog.printScope();	
	}
	cog.printScope();	
	cog.builder.SetInsertPoint(scope->getBlock());
}

}

