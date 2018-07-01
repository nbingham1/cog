#pragma once

#include "Info.h"

namespace Cog
{

Info *getConstant(char *txt);
Info *getTypename(char *txt);
Info *getIdentifier(char *txt);
Info *unaryOperator(int op, Info *arg);
Info *binaryOperator(Info *left, int op, Info *right);
void declareSymbol(Info *type, char *name);
void assignSymbol(Info *symbol, Info *value);
void callFunction(char *txt);
void returnValue(Info *value);
void returnVoid();

void ifCondition(Info *cond);
void elseifCondition();
void elseCondition();
void ifStatement();

void whileKeyword();
void whileCondition(Info *cond);
void whileStatement();

}

