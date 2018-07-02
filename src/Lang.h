#pragma once

#include "Info.h"

namespace Cog
{

Info *getConstant(int token, char *txt);
Info *getPrimitive(int token, char *txt);
Info *getIdentifier(char *txt);
Info *unaryOperator(int op, Info *arg);
Info *binaryOperator(Info *left, int op, Info *right);
void declareSymbol(Info *type, char *name);
void assignSymbol(Info *symbol, Info *value);
void callFunction(char *txt, Info *args);
void returnValue(Info *value);
void returnVoid();

void ifCondition(Info *cond);
void elseifCondition();
void elseCondition();
void ifStatement();

void whileKeyword();
void whileCondition(Info *cond);
void whileStatement();
void functionPrototype(Info *retType, char *name); 
void functionDeclaration();

Info *instanceList(Info *lst, Info *elem);

}

