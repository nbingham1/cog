#pragma once

#include "CogSymbol.h"

namespace Cog
{

struct Typename
{
	/*bool canWrite;
	bool canRead;
	Typename retType;
	Typename recvType;*/
	string id;
	/*vector<Typename> argList;
	int ptrCount;*/
};

bool operator<(Typename t1, Typename t2);
bool operator>(Typename t1, Typename t2);
bool operator<=(Typename t1, Typename t2);
bool operator>=(Typename t1, Typename t2);
bool operator==(Typename t1, Typename t2);
bool operator!=(Typename t1, Typename t2);

struct Symbol
{
	int scope;
	Typename type;
	string name;

	Value *value;
};

struct Type
{
	vector<Symbol> symbols;
};

struct Info
{
	Info();
	~Info();

	Typename type;
	Value *constant;
	int symbol;
};

}
