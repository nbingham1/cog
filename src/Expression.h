#pragma once

#include "Info.h"

namespace Cog
{

llvm::Value *castType(llvm::Value *value, Typename from, Typename to);
Info *castType(Info *from, const Typename &to);
int implicitCastDistance(const Typename &from, const Typename &to);
void unaryTypecheck(Info *left, const Typename &expect);
void binaryTypecheck(Info *left, Info *right);

Info *getBooleanOr(Info *left, Info *right);
Info *getBooleanXor(Info *left, Info *right);
Info *getBooleanAnd(Info *left, Info *right);

Info *getCmpLT(Info *left, Info *right);
Info *getCmpGT(Info *left, Info *right);
Info *getCmpLE(Info *left, Info *right);
Info *getCmpGE(Info *left, Info *right);
Info *getCmpEQ(Info *left, Info *right);
Info *getCmpNE(Info *left, Info *right);

Info *getBitwiseOr(Info *left, Info *right);
Info *getBitwiseXor(Info *left, Info *right);
Info *getBitwiseAnd(Info *left, Info *right);

Info *getShl(Info *left, Info *right);
Info *getAshr(Info *left, Info *right);
Info *getLshr(Info *left, Info *right);
Info *getRor(Info *left, Info *right);
Info *getRol(Info *left, Info *right);

Info *getAdd(Info *left, Info *right);
Info *getSub(Info *left, Info *right);
Info *getMult(Info *left, Info *right);
Info *getDiv(Info *left, Info *right);
Info *getRem(Info *left, Info *right);

}

