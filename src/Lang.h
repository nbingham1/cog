#pragma once

#include "Info.h"

namespace Cog
{

Info *getTypename(int token, char *txt);
Info *getConstant(int token, char *txt);
Info *getIdentifier(char *txt);

Info *unaryOperator(int op, Info *arg);
Info *binaryOperator(Info *left, int op, Info *right);

void declareSymbol(Info *type, char *name);
void assignSymbol(Info *left, int op, Info *right);

void structureDefinition(char *name);

Info *functionPrototype(Info *retType, char *name);
void functionDeclaration(Info *retType, char *name); 
void functionDefinition();
void returnValue(Info *value);
void returnVoid();

void callFunction(char *txt, Info *argList);

void ifCondition(Info *cond);
void elseifCondition();
void elseCondition();
void ifStatement();

void whileKeyword();
void whileCondition(Info *cond);
void whileStatement();

Info *infoList(Info *lst, Info *elem);

Info *asmRegister(char *txt);
Info *asmConstant(char *txt);
Info *asmIdentifier(char *txt);
Info *asmFunction(char *txt, Info *args);
void asmFunctionDefinition();
Info *asmMemoryArg(char *seg, char *cnst, Info *args);
Info *asmAddress(Info *base, Info *offset, char *scale);
Info *asmInstruction(char *instr, Info *args);
Info *asmStatement(char *label, Info *instr);
void asmBlock(Info *instrs);


}

