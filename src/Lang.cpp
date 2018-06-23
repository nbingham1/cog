#include "Lang.h"
#include "Compiler.h"

#include <stdio.h>
#include <string.h>

namespace Cog
{

extern Compiler cog;

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

	delete txt;
	return NULL;
}

Info *getIdentifier(char *txt)
{
	for (int i = (int)cog.symbols.size()-1; i >= 0; i--) {
		if (strcmp(cog.symbols[i].name.c_str(), txt) == 0) {
			Info *result = new Info();
			result->symbol = i;
			result->value = cog.symbols[i].value;
			result->type = result->value->getType();
			delete txt;
			return result;
		}
	}

	delete txt;
	return NULL;
}

Info *unaryOperator(int op, Info *arg)
{
	if (arg) {
		switch (op) {
			case '-':
				arg->value = cog.builder.CreateNeg(arg->value);
				arg->type = arg->value->getType();
				arg->symbol = -1;
				return arg;
			default:
				return arg;
		}
	} else {
		return NULL;
	}
}

Info *binaryOperator(Info *left, int op, Info *right)
{
	if (left && right) {
		switch (op) {
		case '+':
			left->value = cog.builder.CreateAdd(left->value, right->value);
			left->type = left->value->getType();
			delete right;
			return left;
		case '-':
			left->value = cog.builder.CreateSub(left->value, right->value);
			left->type = left->value->getType();
			delete right;
			return left;
		case '*':
			left->value = cog.builder.CreateMul(left->value, right->value);
			left->type = left->value->getType();
			delete right;
			return left;
		case '/':
			left->value = cog.builder.CreateSDiv(left->value, right->value);
			left->type = left->value->getType();
			delete right;
			return left;
		default:
			delete left;
			delete right;
			return NULL;
		}
	} else {
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
		for (int i = (int)cog.symbols.size()-1; i >= 0; i--) {
			if (strcmp(cog.symbols[i].name.c_str(), txt) == 0) {
				// already defined
			}
		}

		cog.symbols.push_back(Cog::Symbol());
		cog.symbols.back().name = txt;
		cog.symbols.back().value = UndefValue::get(type->type);
		delete type;
		delete txt;
	} else {
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
			cog.symbols[symbol->symbol].value = value->value;
		} else {
			// typecheck failed
		}
	} else {
		if (symbol)
			delete symbol;
		if (value)
			delete value;
		// something is broken
	}
}

}
