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
			value->value->setName(symbol->symbol->name + "_");
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
	std::vector<Value*> argValues;
	cog.builder.CreateCall(cog.module->getFunction(txt), argValues);
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

void ifCondition(Info *cond)
{
	Function *func = cog.builder.GetInsertBlock()->getParent();
	
	printf("ifCondition()\n");
	BasicBlock *thenBlock = BasicBlock::Create(cog.context, "then", func);
	BasicBlock *elseBlock = BasicBlock::Create(cog.context, "else", func);
	cog.getScope()->appendBlock(thenBlock);
	cog.getScope()->appendBlock(elseBlock);

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
		scope->appendBlock(mergeBlock);
	}
	
	scope->nextBlock();
	cog.builder.SetInsertPoint(scope->getBlock());

	std::vector<llvm::PHINode*> phi;
	phi.reserve(scope->symbols.size());
	for (int i = 0; i < (int)scope->symbols.size(); i++) {
		phi.push_back(cog.builder.CreatePHI(scope->symbols[i].type, scope->blocks.size()-1));
		phi.back()->setName(scope->symbols[i].name + "_");
		scope->symbols[i].values.back() = phi.back();
	}

	while (scope->blocks.size() > 1) {
		cog.builder.SetInsertPoint(scope->blocks.front());
		cog.builder.CreateBr(scope->getBlock());
		for (int i = 0; i < (int)scope->symbols.size(); i++)
			phi[i]->addIncoming(scope->symbols[i].values.front(), scope->blocks.front());
		scope->dropBlock();
		cog.printScope();	
	}
	cog.printScope();
	cog.builder.SetInsertPoint(scope->getBlock());
}

void whileKeyword()
{
	Function *func = cog.builder.GetInsertBlock()->getParent();
	BasicBlock *fromBlock = cog.getScope()->getBlock();
	
	printf("whileKeyword()\n");
	BasicBlock *condBlock = BasicBlock::Create(cog.context, "cond", func);
	cog.getScope()->appendBlock(condBlock);
	
	cog.builder.CreateBr(condBlock);
	cog.getScope()->popBlock();
	cog.builder.SetInsertPoint(cog.getScope()->getBlock());

	Scope *scope = cog.getScope();
	for (int i = 0; i < (int)scope->symbols.size(); i++) {
		PHINode *value = cog.builder.CreatePHI(scope->symbols[i].type, scope->blocks.size()-1);
		value->addIncoming(scope->symbols[i].getValue(), fromBlock);
		value->setName(scope->symbols[i].name + "_");
		scope->symbols[i].setValue(value);
	}
}

void whileCondition(Info *cond)
{
	Function *func = cog.builder.GetInsertBlock()->getParent();
	
	printf("whileCondition()\n");
	BasicBlock *thenBlock = BasicBlock::Create(cog.context, "then", func);
	BasicBlock *elseBlock = BasicBlock::Create(cog.context, "else", func);
	cog.getScope()->appendBlock(thenBlock);
	cog.getScope()->appendBlock(elseBlock);

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

void functionPrototype(Info *retType, char *name)
{
	std::vector<Type*> args;
  FunctionType *FT =
    FunctionType::get(retType->type, args, false);

  Function *F =
    Function::Create(FT, Function::ExternalLinkage, name, cog.module);
	BasicBlock *body = BasicBlock::Create(cog.context, "entry", F, 0);
	cog.scopes.push_back(Cog::Scope(body));
	cog.builder.SetInsertPoint(body);

	cog.func = F;
}

void functionDeclaration()
{
	cog.scopes.clear();
	cog.func = NULL;
}

}

