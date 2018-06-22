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

struct Symbol;

struct SymbolTable
{
	vector<Symbol> symbols;
};

struct Symbol
{
	Typename type;
	string name;
	
	SymbolTable table;
};

struct ScopeStack
{
	vector<SymbolTable> tables;
};

struct Type
{
	SymbolTable table;
};

struct TypeTable
{
	map<Typename, Type> types;
};

}
