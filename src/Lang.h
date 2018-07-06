#pragma once

#include "Info.h"

namespace Cog
{

Info *getTypename(int token, char *txt);
Info *getConstant(int token, char *txt);
Info *getIdentifier(char *txt);

Info *castType(Info *from, Info *to);
bool canImplicitCast(Info *from, Info *to);
void handleTypecheck(Info *left, Info *right, Info *expected = NULL);

Info *unaryOperator(int op, Info *arg);
Info *binaryOperator(Info *left, int op, Info *right);

void declareSymbol(Info *type, char *name);
void assignSymbol(Info *symbol, Info *value);

void structureDefinition(char *name);

void functionPrototype(Info *retType, char *name, Info *typeList);
void functionDeclaration(Info *retType, char *name); 
void functionDefinition();
void returnValue(Info *value);
void returnVoid();

void callFunction(char *txt, Info *args);

void ifCondition(Info *cond);
void elseifCondition();
void elseCondition();
void ifStatement();

void whileKeyword();
void whileCondition(Info *cond);
void whileStatement();

Info *infoList(Info *lst, Info *elem);

}

